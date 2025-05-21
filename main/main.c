#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // Added
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_sip.h"      // Added
#include "audio_hal.h"    // Added
#include "g711_codec.h" // Added

// WiFi Configuration
#define EXAMPLE_ESP_WIFI_SSID      "YOUR_WIFI_SSID"
#define EXAMPLE_ESP_WIFI_PASS      "YOUR_WIFI_PASSWORD"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

// SIP Configuration - REPLACE WITH YOUR ACTUAL CREDENTIALS
#define EXAMPLE_SIP_URI            "sip:YOUR_SIP_USERNAME@YOUR_SIP_DOMAIN" // e.g. "sip:user1@example.com"
#define EXAMPLE_SIP_SERVER         "YOUR_SIP_SERVER_IP_OR_DOMAIN"       // e.g. "sip.example.com"
#define EXAMPLE_SIP_PORT           5060                                 // Default SIP port
#define EXAMPLE_SIP_USERNAME       "YOUR_SIP_USERNAME"
#define EXAMPLE_SIP_PASSWORD       "YOUR_SIP_PASSWORD"


static const char *TAG = "SIP_PHONE_MAIN";

static int s_retry_num = 0;
static bool s_wifi_connected = false; // Flag to track WiFi connection status

// Audio Queues & Buffers
#define AUDIO_QUEUE_LENGTH 10
#define PCM_BUFFER_SIZE_SAMPLES 320 // 20ms at 16kHz sample rate (as per audio_hal.h I2S_MIC_SAMPLE_RATE)
#define PCM_BUFFER_SIZE_BYTES (PCM_BUFFER_SIZE_SAMPLES * sizeof(int16_t)) // 320 * 2 = 640 bytes
#define G711_BUFFER_SIZE_BYTES PCM_BUFFER_SIZE_SAMPLES // G.711 is 1 byte per sample, so 320 bytes for 320 samples
#define AUDIO_QUEUE_ITEM_SIZE_BYTES G711_BUFFER_SIZE_BYTES


static QueueHandle_t mic_to_sip_queue;
static QueueHandle_t sip_to_speaker_queue;

// Task Handles
static TaskHandle_t audio_capture_task_handle = NULL;
static TaskHandle_t audio_playback_task_handle = NULL;
static TaskHandle_t sip_audio_send_task_handle = NULL; // Added handle

// Call state flag
static volatile bool is_call_active = false;
// Simulation flags (optional, for testing without real SIP server)
// static bool simulated_call_made = false;
// static bool simulated_incoming_triggered = false;


// --- SIP Callback Implementations ---
static void app_incoming_call_handler(const char *caller_uri) {
    ESP_LOGI(TAG, "Incoming call from: %s", caller_uri);
    // In a real app: ring, wait for user, then esp_sip_answer_call()
    // For placeholder: Assume SIP library sets its state to active upon answer.
    // We will poll esp_sip_get_state() to update is_call_active.
    // If esp_sip_answer_call() is available and works:
    // esp_err_t ans_ret = esp_sip_answer_call();
    // if (ans_ret == ESP_OK) {
    //     ESP_LOGI(TAG, "Call answered successfully (simulated by placeholder).");
    // is_call_active = true; // Or set based on state update from SIP lib
    // } else {
    //     ESP_LOGE(TAG, "Failed to answer call (simulated by placeholder).");
    // }
    // For now, we just log. The main loop will check SIP state.
}

static void app_audio_receive_from_sip(const uint8_t *data, size_t len) {
    // ESP_LOGD(TAG, "Received %d bytes of audio from SIP to app_audio_receive_from_sip", len);
    if (is_call_active && sip_to_speaker_queue != NULL) {
         if (len > AUDIO_QUEUE_ITEM_SIZE_BYTES) {
             ESP_LOGW(TAG, "Received audio data (%d bytes) larger than queue item size (%d bytes). Truncating.", (int)len, AUDIO_QUEUE_ITEM_SIZE_BYTES);
            len = AUDIO_QUEUE_ITEM_SIZE_BYTES;
        }
        if (xQueueSend(sip_to_speaker_queue, data, pdMS_TO_TICKS(20)) != pdPASS) { // Increased timeout slightly
            // ESP_LOGW(TAG, "Failed to send to sip_to_speaker_queue");
        }
    }
}

// --- Audio Tasks ---
static void audio_capture_task(void *pvParameters) {
    int16_t *pcm_buffer = (int16_t *)malloc(PCM_BUFFER_SIZE_BYTES);
    uint8_t *g711_buffer = (uint8_t *)malloc(G711_BUFFER_SIZE_BYTES);
    size_t bytes_read;

    if (!pcm_buffer || !g711_buffer) {
        ESP_LOGE(TAG, "Capture task: Failed to allocate audio buffers");
        if(pcm_buffer) free(pcm_buffer);
        if(g711_buffer) free(g711_buffer);
        audio_capture_task_handle = NULL; // Clear task handle
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Audio Capture Task Started (PCM: %d bytes, G711: %d bytes)", PCM_BUFFER_SIZE_BYTES, G711_BUFFER_SIZE_BYTES);
    while (1) {
        if (is_call_active) {
            esp_err_t read_ret = audio_hal_mic_read(pcm_buffer, PCM_BUFFER_SIZE_BYTES, &bytes_read, pdMS_TO_TICKS(100));
            if (read_ret == ESP_OK && bytes_read > 0) {
                // Assuming audio_hal_mic_read provides 16-bit PCM data directly.
                // If I2S_MIC_BITS_PER_SAMPLE in audio_hal.h is 32BIT for reading,
                // then pcm_buffer should be processed to extract 16-bit samples before encoding.
                // For now, assuming direct 16-bit PCM data.
                size_t pcm_samples_read = bytes_read / sizeof(int16_t);
                if (pcm_samples_read > PCM_BUFFER_SIZE_SAMPLES) pcm_samples_read = PCM_BUFFER_SIZE_SAMPLES; // Defensive

                size_t encoded_len = g711_encode_alaw(pcm_buffer, g711_buffer, pcm_samples_read); // Using A-law for consistency with decode
                
                if (encoded_len > 0) {
                    if (xQueueSend(mic_to_sip_queue, g711_buffer, pdMS_TO_TICKS(20)) != pdPASS) {
                        // ESP_LOGW(TAG, "Capture task: Failed to send to mic_to_sip_queue");
                    }
                }
            } else if (read_ret != ESP_OK && read_ret != ESP_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "Mic read error: %s", esp_err_to_name(read_ret));
                vTaskDelay(pdMS_TO_TICKS(20));  // Wait a bit on error before retrying
            } else {
                 vTaskDelay(pdMS_TO_TICKS(5)); // Short delay if timeout or no data
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
    }
    free(pcm_buffer);
    free(g711_buffer);
    audio_capture_task_handle = NULL; // Clear task handle
    vTaskDelete(NULL);
}

static void sip_audio_send_task(void *pvParameters) {
    uint8_t *g711_buffer = (uint8_t *)malloc(G711_BUFFER_SIZE_BYTES);
     if (!g711_buffer) {
        ESP_LOGE(TAG, "Send task: Failed to allocate g711_buffer");
        sip_audio_send_task_handle = NULL; // Clear task handle
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "SIP Audio Send Task Started");
    while(1) {
        if (is_call_active && mic_to_sip_queue != NULL) {
            if (xQueueReceive(mic_to_sip_queue, g711_buffer, portMAX_DELAY) == pdPASS) {
                esp_err_t ret = esp_sip_send_audio(g711_buffer, G711_BUFFER_SIZE_BYTES);
                if (ret != ESP_OK) {
                    // ESP_LOGW(TAG, "esp_sip_send_audio failed (placeholder)");
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    free(g711_buffer);
    sip_audio_send_task_handle = NULL; // Clear task handle
    vTaskDelete(NULL);
}

static void audio_playback_task(void *pvParameters) {
    int16_t *pcm_buffer = (int16_t *)malloc(PCM_BUFFER_SIZE_BYTES);
    uint8_t *g711_buffer = (uint8_t *)malloc(G711_BUFFER_SIZE_BYTES);
    size_t bytes_written;

    if (!pcm_buffer || !g711_buffer) {
        ESP_LOGE(TAG, "Playback task: Failed to allocate audio buffers");
        if(pcm_buffer) free(pcm_buffer);
        if(g711_buffer) free(g711_buffer);
        audio_playback_task_handle = NULL; // Clear task handle
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Audio Playback Task Started");
    while (1) {
        if (is_call_active && sip_to_speaker_queue != NULL) {
            if (xQueueReceive(sip_to_speaker_queue, g711_buffer, portMAX_DELAY) == pdPASS) {
                size_t decoded_samples = g711_decode_alaw(g711_buffer, pcm_buffer, G711_BUFFER_SIZE_BYTES); // Using A-law
                if (decoded_samples > 0) {
                    // Assuming audio_hal_speaker_write expects 16-bit PCM.
                    esp_err_t write_ret = audio_hal_speaker_write(pcm_buffer, decoded_samples * sizeof(int16_t), &bytes_written, pdMS_TO_TICKS(100));
                    if (write_ret != ESP_OK && write_ret != ESP_ERR_TIMEOUT) { // Don't log timeout as error
                       ESP_LOGE(TAG, "Speaker write error: %s", esp_err_to_name(write_ret));
                    }
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
    }
    free(pcm_buffer);
    free(g711_buffer);
    audio_playback_task_handle = NULL; // Clear task handle
    vTaskDelete(NULL);
}


// --- WiFi Event Handler ---
static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_wifi_connected = false; 
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            ESP_LOGE(TAG, "connect to the AP fail");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_wifi_connected = true; 

        // Initialize SIP Client (after WiFi and IP obtained)
        ESP_LOGI(TAG, "Initializing SIP client...");
        esp_sip_config_t sip_config = {
            .uri = EXAMPLE_SIP_URI, // Use macro
            .server_address = EXAMPLE_SIP_SERVER, // Use macro
            .server_port = EXAMPLE_SIP_PORT,       // Use macro
            .username = EXAMPLE_SIP_USERNAME,   // Use macro
            .password = EXAMPLE_SIP_PASSWORD    // Use macro
        };

        if (esp_sip_init(&sip_config) == ESP_OK) {
            ESP_LOGI(TAG, "SIP client initialized successfully.");
            
            // Register SIP callbacks
            esp_sip_set_incoming_call_cb(app_incoming_call_handler);
            esp_sip_set_audio_receive_cb(app_audio_receive_from_sip);
            ESP_LOGI(TAG, "SIP callbacks registered.");

            if (esp_sip_start() == ESP_OK) { // Attempt registration
                ESP_LOGI(TAG, "SIP client registration process started.");
            } else {
                ESP_LOGE(TAG, "Failed to start SIP client registration.");
            }
        } else {
            ESP_LOGE(TAG, "Failed to initialize SIP client.");
        }
    }
}

// --- WiFi Init ---
void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &event_handler,
                                                    NULL,
                                                    &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &event_handler,
                                                    NULL,
                                                    &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.capable = true, .required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

// --- app_main ---
void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 SIP Phone application");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Audio HAL (moved here, before WiFi, to ensure it's ready)
    if (audio_hal_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Audio HAL. SIP Phone cannot function.");
        // Depending on requirements, might want to halt or try recovery, or allow limited functionality.
        // For now, we'll let it continue to see if WiFi/SIP can init, but audio tasks might fail.
    } else {
        ESP_LOGI(TAG, "Audio HAL initialized.");
    }

    // Initialize WiFi (which will then trigger SIP init in event_handler)
    wifi_init_sta();
    
    ESP_LOGI(TAG, "Waiting for WiFi connection to initialize queues and tasks...");
    while (!s_wifi_connected) { // Wait for WiFi connection from event_handler
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
    ESP_LOGI(TAG, "WiFi connected. Proceeding with task creation.");


    // Create Audio Queues
    mic_to_sip_queue = xQueueCreate(AUDIO_QUEUE_LENGTH, AUDIO_QUEUE_ITEM_SIZE_BYTES);
    sip_to_speaker_queue = xQueueCreate(AUDIO_QUEUE_LENGTH, AUDIO_QUEUE_ITEM_SIZE_BYTES);

    if (!mic_to_sip_queue || !sip_to_speaker_queue) {
        ESP_LOGE(TAG, "Failed to create audio queues. Halting.");
        return; // Halt if queues cannot be created
    }
    ESP_LOGI(TAG, "Audio queues created.");

    // Create Audio Tasks
    if (xTaskCreate(audio_capture_task, "audio_capture_task", 4096, NULL, 5, &audio_capture_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio_capture_task.");
    }
    if (xTaskCreate(audio_playback_task, "audio_playback_task", 4096, NULL, 5, &audio_playback_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio_playback_task.");
    }
    if (xTaskCreate(sip_audio_send_task, "sip_audio_send_task", 4096, NULL, 5, &sip_audio_send_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sip_audio_send_task.");
    }
    ESP_LOGI(TAG, "Audio tasks creation attempted.");

    // Main application loop: Monitor SIP state, handle call logic
    while(1) {
        esp_sip_state_t current_sip_state = esp_sip_get_state(); 

        if (current_sip_state == SIP_STATE_CALL_ACTIVE) {
            if (!is_call_active) {
                ESP_LOGI(TAG, "Call is now ACTIVE.");
                is_call_active = true;
                // Potentially unblock audio tasks if they were waiting on this flag explicitly
                // Clear queues at the start of a new call to remove any stale data.
                if(mic_to_sip_queue) xQueueReset(mic_to_sip_queue);
                if(sip_to_speaker_queue) xQueueReset(sip_to_speaker_queue);
                ESP_LOGI(TAG, "Audio queues reset for new call.");
            }
        } else { 
            if (is_call_active) {
                ESP_LOGI(TAG, "Call is no longer active (current SIP state: %d).", current_sip_state);
                is_call_active = false;
            }
        }
        
        // --- For testing purposes (remove or guard with #ifdef for production) ---
        // The following extern declarations are needed if you uncomment the simulation blocks below.
        // extern void esp_sip_simulate_incoming_audio(const uint8_t *data, size_t len); // In sip_client.c
        // extern void esp_sip_simulate_incoming_call(const char *caller_uri); // In sip_client.c
        // extern esp_err_t esp_sip_answer_call(void); // In sip_client.c

        // Example: Simulate an incoming call then auto-answer after registration
        // static bool main_sim_incoming_triggered = false; // Use a static flag for one-time simulation
        // if (current_sip_state == SIP_STATE_REGISTERED && !is_call_active && !main_sim_incoming_triggered) {
        //    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait a bit
        //    ESP_LOGI(TAG, "[SIM_MAIN] Simulating an incoming call from sip:caller@example.com...");
        //    esp_sip_simulate_incoming_call("sip:caller@example.com"); // This function is in sip_client.c
        //    main_sim_incoming_triggered = true;
        // }
        // if (current_sip_state == SIP_STATE_INCOMING_RINGING && main_sim_incoming_triggered) {
        //    vTaskDelay(pdMS_TO_TICKS(1000)); // Simulate user delay before answering
        //    ESP_LOGI(TAG, "[SIM_MAIN] Simulating answering the call...");
        //    esp_sip_answer_call(); // This function is in sip_client.c, should change SIP state to CALL_ACTIVE
        // }

        // Example: Simulate making an outgoing call
        // static bool main_sim_outgoing_call_made = false;
        // if (current_sip_state == SIP_STATE_REGISTERED && !is_call_active && !main_sim_outgoing_call_made && !main_sim_incoming_triggered) {
        //    vTaskDelay(pdMS_TO_TICKS(7000)); // Wait longer
        //    ESP_LOGI(TAG, "[SIM_MAIN] Simulating making an outgoing call to sip:test@example.com...");
        //    esp_sip_make_call("sip:test@example.com");
        //    main_sim_outgoing_call_made = true;
        // }
        
        // Example: Simulate call ending after some time
        // static int call_duration_seconds = 0;
        // if (is_call_active && (main_sim_outgoing_call_made || main_sim_incoming_triggered)) {
        //     if (call_duration_seconds < 15) { // Simulate a 15-second call
        //         ESP_LOGI(TAG, "[SIM_MAIN] Call active, duration: %d s", call_duration_seconds);
        //         call_duration_seconds++;
        //     } else {
        //         ESP_LOGI(TAG, "[SIM_MAIN] Simulating ending the call...");
        //         esp_sip_end_call(); // This should change SIP state
        //         main_sim_outgoing_call_made = false; 
        //         main_sim_incoming_triggered = false;
        //         call_duration_seconds = 0;
        //     }
        // }
        // --- End Simulation Logic ---

        vTaskDelay(pdMS_TO_TICKS(500)); // Check state periodically
    }
}

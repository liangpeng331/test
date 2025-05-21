#include "esp_sip.h"
#include "esp_log.h"

static const char *TAG_SIP = "ESP_SIP";
static esp_sip_config_t current_config;
static esp_sip_state_t current_state = SIP_STATE_UNREGISTERED;
static sip_incoming_call_cb_t incoming_call_callback = NULL;
static sip_audio_receive_cb_t audio_receive_callback = NULL;

esp_err_t esp_sip_init(const esp_sip_config_t *config) {
    if (!config || !config->uri) {
        ESP_LOGE(TAG_SIP, "SIP config or URI is null");
        return ESP_ERR_INVALID_ARG;
    }
    current_config = *config; // Shallow copy, be mindful of string lifetimes
    ESP_LOGI(TAG_SIP, "SIP Client Initialized. URI: %s, Server: %s:%d", current_config.uri, current_config.server_address, current_config.server_port);
    // In a real library, more setup for SIP stack (PJSIP, SofiaSIP etc.) would happen here
    current_state = SIP_STATE_UNREGISTERED;
    return ESP_OK;
}

esp_err_t esp_sip_start(void) {
    ESP_LOGI(TAG_SIP, "Starting SIP registration for %s...", current_config.uri);
    // Simulate registration process
    current_state = SIP_STATE_REGISTERING;
    // In a real scenario, this would involve sending SIP REGISTER messages
    // For now, let's assume it registers successfully after a delay
    // or upon receiving a specific event (which we are not simulating here)
    ESP_LOGI(TAG_SIP, "SIP Client attempting to register.");
    // current_state = SIP_STATE_REGISTERED; // Simulate successful registration for now
    return ESP_OK;
}

esp_sip_state_t esp_sip_get_state(void) {
    return current_state;
}

// Add placeholder implementations for other SIP functions if needed by main.c

esp_err_t esp_sip_make_call(const char *callee_uri) {
    if (!callee_uri) {
        ESP_LOGE(TAG_SIP, "Callee URI is null");
        return ESP_ERR_INVALID_ARG;
    }
    // Allow making call if registered, or if previous call ended, or if not registered yet (library might handle this)
    // This logic might be more restrictive in a real client (e.g. only if registered)
    if (current_state != SIP_STATE_REGISTERED && current_state != SIP_STATE_CALL_ENDED && current_state != SIP_STATE_UNREGISTERED && current_state != SIP_STATE_INCOMING_RINGING) {
        ESP_LOGW(TAG_SIP, "Making call from state: %d. Might not be ideal.", current_state);
    }
    ESP_LOGI(TAG_SIP, "Attempting to make call to: %s", callee_uri);
    // Simulate call initiation
    current_state = SIP_STATE_CALL_INITIATED;
    // In a real client, this would involve sending SIP INVITE messages
    // For simulation, let's assume the call becomes active almost immediately
    // In a real scenario, there would be provisional responses (180 Ringing, 183 Session Progress)
    // and then a 200 OK to actually make the call active.
    // For this placeholder, we'll jump to ACTIVE if originating a call.
    // If it were an incoming call being answered, this transition would be from INCOMING_RINGING.
    // current_state = SIP_STATE_CALL_ACTIVE; // Or could stay in INITIATED until an OK is received
    ESP_LOGI(TAG_SIP, "Call initiated to %s. State: %d", callee_uri, current_state);
    return ESP_OK;
}

esp_err_t esp_sip_set_incoming_call_cb(sip_incoming_call_cb_t cb) {
    incoming_call_callback = cb;
    ESP_LOGI(TAG_SIP, "Incoming call callback set.");
    return ESP_OK;
}

// This function would be called by the underlying SIP stack when an INVITE is received
// For simulation, we might need a way to trigger this from main or a test function
void simulate_incoming_call(const char *caller_uri) {
    ESP_LOGI(TAG_SIP, "Simulating incoming call from: %s", caller_uri);
    current_state = SIP_STATE_INCOMING_RINGING; 
    if (incoming_call_callback) {
        incoming_call_callback(caller_uri);
    } else {
        ESP_LOGW(TAG_SIP, "Incoming call received, but no callback is set.");
    }
}

// Example function to simulate answering a call (could be triggered by user input)
esp_err_t esp_sip_answer_call(void) {
    if (current_state != SIP_STATE_INCOMING_RINGING) {
        ESP_LOGW(TAG_SIP, "No incoming call to answer. Current state: %d", current_state);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_SIP, "Attempting to answer call.");
    current_state = SIP_STATE_CALL_ACTIVE;
    // In a real client, this would involve sending SIP 200 OK messages
    ESP_LOGI(TAG_SIP, "Call answered. State: %d", current_state);
    return ESP_OK;
}


esp_err_t esp_sip_end_call(void) {
    if (current_state != SIP_STATE_CALL_ACTIVE && current_state != SIP_STATE_CALL_INITIATED && current_state != SIP_STATE_INCOMING_RINGING) {
        ESP_LOGW(TAG_SIP, "No active, initiated, or ringing call to end. Current state: %d", current_state);
        return ESP_FAIL; 
    }
    ESP_LOGI(TAG_SIP, "Attempting to end call. Current state: %d", current_state);
    // Simulate call termination
    current_state = SIP_STATE_CALL_ENDED;
    // In a real client, this would involve sending SIP BYE or CANCEL messages
    ESP_LOGI(TAG_SIP, "Call ended. State: %d", current_state);
    return ESP_OK;
}

esp_err_t esp_sip_send_audio(const uint8_t *data, size_t len) {
    if (current_state != SIP_STATE_CALL_ACTIVE) {
        // ESP_LOGW(TAG_SIP, "Cannot send audio, call not active.");
        return ESP_FAIL;
    }
    // In a real library, this would packetize and send audio via RTP
    ESP_LOGD(TAG_SIP, "esp_sip_send_audio: %d bytes (simulated)", (int)len);
    // Simulate sending by perhaps echoing back to receiver if loopback desired for test
    // if (audio_receive_callback) {
    //     audio_receive_callback(data, len); // Loopback for testing
    // }
    return ESP_OK;
}

esp_err_t esp_sip_set_audio_receive_cb(sip_audio_receive_cb_t cb) {
    audio_receive_callback = cb;
    ESP_LOGI(TAG_SIP, "Audio receive callback set.");
    return ESP_OK;
}

// Add a function to simulate receiving audio from network to test the callback
void esp_sip_simulate_incoming_audio(const uint8_t *data, size_t len) {
    if (audio_receive_callback) {
        ESP_LOGD(TAG_SIP, "Simulating incoming audio of %d bytes.", (int)len);
        audio_receive_callback(data, len);
    } else {
        ESP_LOGW(TAG_SIP, "No audio receive callback registered to simulate incoming audio.");
    }
}

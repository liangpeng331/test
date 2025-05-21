#ifndef ESP_SIP_H
#define ESP_SIP_H

#include "esp_err.h"

// Placeholder SIP configuration structure
typedef struct {
    const char *uri;
    const char *username; // If authentication is needed
    const char *password; // If authentication is needed
    // Add other relevant fields like server, port etc.
    // For example:
    const char *server_address;
    int server_port;
} esp_sip_config_t;

// Placeholder for SIP client state
typedef enum {
    SIP_STATE_UNREGISTERED,
    SIP_STATE_REGISTERING,
    SIP_STATE_REGISTERED,
    SIP_STATE_CALL_INITIATED, // Outgoing call attempt
    SIP_STATE_INCOMING_RINGING, // Incoming call received, alerting user
    SIP_STATE_CALL_ACTIVE,
    SIP_STATE_CALL_ENDED
} esp_sip_state_t;


// Placeholder for a SIP initialization function
esp_err_t esp_sip_init(const esp_sip_config_t *config);
// Placeholder for a function to start the SIP client (e.g., registration)
esp_err_t esp_sip_start(void);
// Placeholder for a function to get current SIP state
esp_sip_state_t esp_sip_get_state(void);

// Placeholder for making an outgoing call
esp_err_t esp_sip_make_call(const char *callee_uri);

// Placeholder for answering an incoming call (if applicable, often auto-answered in simple clients)
// Or a function to notify main application of an incoming call
typedef void (*sip_incoming_call_cb_t)(const char *caller_uri);
esp_err_t esp_sip_set_incoming_call_cb(sip_incoming_call_cb_t cb);

// Placeholder for ending the current call
esp_err_t esp_sip_end_call(void);

// Callback type for receiving audio data from the SIP component
typedef void (*sip_audio_receive_cb_t)(const uint8_t *data, size_t len);

// Function for the application to send audio data to the SIP stack
esp_err_t esp_sip_send_audio(const uint8_t *data, size_t len);

// Function for the application to register a callback for receiving audio from SIP
esp_err_t esp_sip_set_audio_receive_cb(sip_audio_receive_cb_t cb);

// Function to simulate receiving audio from network (for testing)
void esp_sip_simulate_incoming_audio(const uint8_t *data, size_t len);


#endif // ESP_SIP_H

# ESP32 SIP Phone Testing Guide

## 1. Overview

Testing is a critical phase for the ESP32 SIP Phone project. This guide outlines strategies for unit testing individual components, integration testing the audio pipeline and SIP functionality, and general debugging techniques.

Currently, several key components (`esp32_sip_voice` SIP client, `codec` G.711 implementation) are placeholders. Therefore, initial testing will focus on verifying the structural integrity and basic data flow. As these placeholders are replaced with functional modules, testing will become more comprehensive.

## 2. Prerequisites for Testing

*   **Hardware:**
    *   ESP32 development board (e.g., ESP32-DevKitC).
    *   Microphone (e.g., I2S INMP441).
    *   Speaker and appropriate amplifier (e.g., I2S MAX98357A).
    *   Connections as detailed in `HARDWARE_SETUP.md`.
*   **Software Environment:**
    *   ESP-IDF (Espressif IoT Development Framework) correctly installed and configured.
    *   Build tools for compiling and flashing the firmware to the ESP32.
*   **Network & SIP Infrastructure (for end-to-end testing):**
    *   A functional Wi-Fi network with internet access.
    *   A SIP server (e.g., Asterisk, FreeSWITCH set up locally, or a cloud-based SIP provider).
    *   Another SIP client (e.g., a softphone like Zoiper, Linphone, MicroSIP, or a physical IP phone) for making and receiving calls to/from the ESP32.

## 3. Unit Testing (Conceptual)

Unit tests should focus on verifying the functionality of individual components in isolation.

### 3.1. `audio_hal` (I2S Audio)

*   **I2S Loopback Test:**
    *   If hardware setup allows, physically connect the `I2S_SPEAKER_DOUT_PIN` to the `I2S_MIC_DIN_PIN`.
    *   Alternatively, some ESP32 I2S configurations or specific DACs/ADCs might support internal loopback modes. This would require consulting ESP-IDF documentation for the specific I2S driver version (e.g., the newer `i2s_std_config_t` in ESP-IDF 5.x vs. older `i2s_config_t`).
    *   Write known data using `audio_hal_speaker_write()` and verify that similar data is read back via `audio_hal_mic_read()`.
*   **Microphone Input Test (`audio_hal_mic_read()`):**
    *   Call `audio_hal_mic_read()` and check if it returns `ESP_OK` and `bytes_read` is greater than zero when sound is present.
    *   Analyze the captured data (e.g., print a small portion) to see if it's non-zero and changes with input sound.
*   **Speaker Output Test (`audio_hal_speaker_write()`):**
    *   Prepare a simple PCM buffer (e.g., a sine wave or a constant value).
    *   Call `audio_hal_speaker_write()` with this buffer.
    *   Verify `ESP_OK` is returned and `bytes_written` matches the input length.
    *   Listen for output on the speaker (though it might be noise or a simple tone without proper PCM data).

### 3.2. `codec` (G.711 Implementation)

*   **Current Placeholder Status:** The current `g711_codec.c` contains highly lossy placeholder functions (simple bit-shifting). These are not real G.711 codecs.
*   **Testing a Real G.711 Codec (Future):**
    *   **Test Vectors:** Obtain standard G.711 test vectors (PCM input and corresponding A-law/u-law output). ITU-T G.191 provides tools and test sequences.
    *   **Encode:** Feed PCM test vectors to `g711_encode_alaw()` or `g711_encode_ulaw()`. Compare the output G.711 data with the expected G.711 test vectors.
    *   **Decode:** Feed G.711 test vectors to `g711_decode_alaw()` or `g711_decode_ulaw()`.
    *   **Round-trip:** Encode PCM to G.711, then decode back to PCM. Compare the resulting PCM with the original. Due to the lossy nature of G.711, an exact match isn't expected, but the difference should be within acceptable limits (Signal-to-Noise Ratio - SNR).

### 3.3. `esp32_sip_voice` (Placeholder SIP Client)

*   **State Transitions:**
    *   Call `esp_sip_init()` and verify initial state is `SIP_STATE_UNREGISTERED`.
    *   Call `esp_sip_start()` and verify state transitions to `SIP_STATE_REGISTERING` (and potentially `SIP_STATE_REGISTERED` if simulated).
    *   Call `esp_sip_make_call()` and check if the state changes to `SIP_STATE_CALL_INITIATED`.
    *   Simulate an incoming call (see below) and then call `esp_sip_answer_call()`, checking for `SIP_STATE_CALL_ACTIVE`.
    *   Call `esp_sip_end_call()` during various active/initiated states and verify transition to `SIP_STATE_CALL_ENDED`.
*   **Callback Invocation:**
    *   **Incoming Call:** Call the internal `esp_sip_simulate_incoming_call()` (if exposed for testing, or trigger the condition that calls it internally) and verify that the callback registered via `esp_sip_set_incoming_call_cb()` (i.e., `app_incoming_call_handler` in `main.c`) is executed.
    *   **Audio Receive:** Call `esp_sip_simulate_incoming_audio()` and verify that the callback registered via `esp_sip_set_audio_receive_cb()` (i.e., `app_audio_receive_from_sip` in `main.c`) is executed with the correct data.

## 4. Integration Testing

### 4.1. Local Audio Loopback Test

This test verifies the entire local audio path: Microphone -> I2S Read -> PCM Buffer -> G.711 Encode -> Queue -> G.711 Decode -> PCM Buffer -> I2S Write -> Speaker.

*   **Modification:**
    *   In `main.c`, inside the `sip_audio_send_task`, instead of calling `esp_sip_send_audio()`, directly send the `g711_buffer` to the `sip_to_speaker_queue`.
    ```c
    // Inside sip_audio_send_task, replace esp_sip_send_audio call with:
    if (xQueueSend(sip_to_speaker_queue, g711_buffer, pdMS_TO_TICKS(10)) != pdPASS) {
        // ESP_LOGW(TAG, "Loopback: Failed to send to sip_to_speaker_queue");
    }
    ```
    *   Alternatively, modify `esp_sip_send_audio()` in `sip_client.c` to call `esp_sip_simulate_incoming_audio()` with the same data for a loopback effect if the audio receive callback is set.
*   **Procedure:**
    1.  Build and flash the modified firmware.
    2.  Ensure `is_call_active` is set to `true` to enable the audio tasks (e.g., by temporarily forcing it in `app_main` or simulating a call that becomes active).
    3.  Speak into the microphone.
*   **Expected Outcome:**
    *   Sound captured by the microphone should be audible on the speaker.
    *   The audio quality will be poor due to the placeholder G.711 codec. The purpose is to verify data flow through the queues and tasks.
    *   Check logs for errors from `audio_hal`, `codec`, and queue operations.

### 4.2. End-to-End SIP Call Testing

This requires a functional SIP server and another SIP client.

*   **Setup:**
    *   In `main/main.c`, replace placeholder values for `EXAMPLE_ESP_WIFI_SSID`, `EXAMPLE_ESP_WIFI_PASS`, `EXAMPLE_SIP_URI`, `EXAMPLE_SIP_SERVER`, `EXAMPLE_SIP_USERNAME`, and `EXAMPLE_SIP_PASSWORD` with actual credentials for your Wi-Fi and SIP server.
*   **Registration:**
    *   Flash and run the application.
    *   Monitor ESP32 logs. You should see attempts to connect to Wi-Fi, obtain an IP, and then initialize and start the SIP client, followed by registration attempts.
    *   Check your SIP server's logs or administration interface to confirm if the ESP32 (using `EXAMPLE_SIP_URI`) has successfully registered.
*   **Outgoing Call Test:**
    1.  **Triggering a Call:** Implement a temporary method to trigger `esp_sip_make_call("sip:target_uri@domain.com")`. This could be:
        *   A simple GPIO button press interrupt.
        *   A timed event in `app_main` after registration (e.g., using `vTaskDelay` and then calling `esp_sip_make_call`).
        *   Uncommenting and adapting one of the simulation blocks in `app_main`.
    2.  Replace `"sip:target_uri@domain.com"` with the SIP URI of your test softphone.
    3.  **Verification:**
        *   Softphone should ring.
        *   Answer the call on the softphone.
        *   Speak into the ESP32's microphone.
        *   Audio should be heard on the softphone's speaker (quality will be poor due to placeholder G.711 codec).
        *   Check ESP32 logs for errors and SIP state changes.
        *   Check SIP server logs for call setup (INVITE, TRYING, RINGING, OK, ACK).
*   **Incoming Call Test:**
    1.  From your softphone, call the ESP32's SIP URI (e.g., `YOUR_SIP_USERNAME@YOUR_SIP_DOMAIN`).
    2.  **Verification:**
        *   ESP32 logs should show `app_incoming_call_handler` being triggered.
        *   The ESP32 SIP client state should change to `SIP_STATE_INCOMING_RINGING`.
        *   **Answering:** Since the placeholder `esp_sip_voice` doesn't have full call control, you might need to manually trigger `esp_sip_answer_call()` or have the `app_incoming_call_handler` set the state to active for testing audio flow. A more complete SIP library would handle this properly.
        *   Speak into the softphone's microphone.
        *   Audio should be heard on the ESP32's speaker (quality will be poor).
        *   Check ESP32 logs and SIP server logs.

## 5. Debugging Techniques

*   **Logging (`ESP_LOGx`):**
    *   Extensively use `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`, and `ESP_LOGD` with component-specific `TAG`s.
    *   Increase log verbosity for specific components in `menuconfig` if needed.
*   **FreeRTOS Task & Queue Debugging:**
    *   `vTaskGetRunTimeStats()`: To check CPU time usage by tasks.
    *   `uxTaskGetSystemState()`: To get detailed info about all tasks.
    *   `xQueuePeek()`: To inspect queue status without removing items.
    *   Check for queue send/receive timeouts or failures (return values of `xQueueSend`/`xQueueReceive`).
*   **Memory Monitoring:**
    *   `esp_get_free_heap_size()`: To check available heap memory at various points.
    *   `esp_get_minimum_free_heap_size()`: To track the lowest heap size reached. This helps identify memory leaks.
*   **JTAG Debugging:**
    *   For complex issues, use a JTAG debugger (e.g., ESP-PROG) with GDB for breakpoints, stepping through code, and inspecting variables.
*   **Network Sniffing (Wireshark):**
    *   Capture network traffic on your Wi-Fi network.
    *   Filter for SIP messages (typically UDP or TCP on port 5060).
    *   Filter for RTP packets (typically UDP, various ports negotiated via SDP in SIP messages) to analyze audio streams. This is crucial for debugging missing audio, one-way audio, or corrupted audio packets when a real SIP stack is used.

## 6. Key Areas for Further Development & Debugging

The current project uses placeholders for critical functionalities. Future testing will need to address:

*   **Real SIP Library Integration:**
    *   Replacing the `esp32_sip_voice` component with a functional SIP stack (e.g., PJSIP, Sofia-SIP, or by fully implementing the `GeorgeBregman/ESP32-SIP-Voice` logic).
    *   Debugging SIP registration, call setup (INVITE, ACK, BYE, CANCEL), SDP negotiation, and NAT traversal (STUN/TURN).
*   **Real G.711 Codec Integration:**
    *   Replacing the placeholder functions in `codec/g711_codec.c` with a standard-compliant G.711 A-law and u-law implementation.
*   **RTP Handling:**
    *   Implementing RTP packetization for outgoing audio and depacketization for incoming audio.
    *   Implementing a jitter buffer to handle network packet delay variations. (Often part of a full SIP/RTP stack).
*   **Audio Quality:**
    *   Fine-tuning I2S clock settings, DMA buffer sizes/counts in `audio_hal.c`.
    *   Adjusting microphone gain (often hardware-dependent or via I2S codec if using one like ES8388).
    *   Ensuring correct data width handling (e.g., INMP441 provides 24-bit data, often read as 32-bit; MAX98357A expects 16-bit). Conversion/scaling might be needed before G.711 encoding.
    *   Addressing electrical noise and grounding issues in the hardware setup.
*   **Acoustic Echo Cancellation (AEC):**
    *   If using a speaker and microphone in close proximity, AEC will be essential for full-duplex audio to prevent the other party from hearing their own voice echoed back. This is a complex DSP task. ESP-IDF includes some AEC capabilities, or third-party libraries might be needed.
*   **Robust Error Handling:**
    *   Implementing comprehensive error checking and recovery mechanisms for network dropouts, SIP transaction timeouts, audio device failures, etc.
*   **DTMF Handling:**
    *   Sending and receiving DTMF tones (e.g., via RFC2833).
*   **User Interface:**
    *   Buttons for dialing, answering, hanging up.
    *   A display for caller ID, status, etc.

This testing guide should evolve as the project progresses and placeholders are replaced with functional components.

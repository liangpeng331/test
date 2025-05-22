# ESP32 SIP Phone

## Overview

This project aims to develop a basic SIP (Session Initiation Protocol) based internet phone using the ESP32 microcontroller. It provides a foundational framework for capturing audio from a microphone, processing it, sending/receiving it via a SIP session, and playing it back on a speaker.

## Current Status

**This is currently a foundational framework and not a fully functional SIP phone.**

Key functionalities are implemented using **placeholder components**:
*   **SIP Client (`esp32_sip_voice` component):** Provides the API for SIP operations but does not contain a full SIP stack implementation.
*   **G.711 Codec (`codec` component):** Includes functions for G.711 A-law encoding/decoding but uses simple bit-shifting placeholders, not a real codec.

The basic project structure, Wi-Fi connectivity, I2S audio input/output (`audio_hal` component), and the FreeRTOS task structure for the audio pipeline are in place.

## Features

*   Wi-Fi Connectivity
*   Basic System Initialization
*   I2S Audio Input (Microphone) and Output (Speaker) via `audio_hal` component.
*   (Placeholder) SIP Client API for:
    *   Registration
    *   Making Outgoing Calls
    *   Receiving Incoming Calls
    *   Sending/Receiving Audio Packets
*   (Placeholder) G.711 A-law Audio Codec.
*   Audio pipeline implemented with FreeRTOS tasks and queues for:
    *   Capturing audio from microphone.
    *   Encoding audio (placeholder G.711).
    *   Sending audio to the SIP stack (placeholder).
    *   Receiving audio from the SIP stack (placeholder).
    *   Decoding audio (placeholder G.711).
    *   Playing audio on the speaker.

## Project Structure

*   `main/`: Contains the main application logic (`main.c`), Wi-Fi setup, SIP initialization, audio task creation, and overall call state management.
*   `components/`:
    *   `audio_hal/`: Hardware Abstraction Layer for I2S audio input/output.
    *   `codec/`: Placeholder G.711 audio codec.
    *   `esp32_sip_voice/`: Placeholder SIP client component.
*   `HARDWARE_SETUP.md`: Details on the required hardware and example pin connections.
*   `TESTING_GUIDE.md`: Guidance on how to test the current system and debug issues.

## Getting Started

### 1. Hardware
Refer to the `HARDWARE_SETUP.md` file for details on the ESP32 board, microphone, speaker, and optional amplifier.

### 2. Software Prerequisites
*   **ESP-IDF:** Espressif IoT Development Framework. This project is intended to be built using ESP-IDF (preferably v4.4 or later, though might work with others with minor adjustments, especially regarding CMake component dependencies like `esp_driver_i2s`). Ensure ESP-IDF is installed and configured correctly.

### 3. Configuration
You need to configure your Wi-Fi and SIP credentials:
*   **Wi-Fi Credentials:**
    Open `main/main.c` and update the following macros:
    ```c
    #define EXAMPLE_ESP_WIFI_SSID      "YOUR_WIFI_SSID"
    #define EXAMPLE_ESP_WIFI_PASS      "YOUR_WIFI_PASSWORD"
    ```
*   **SIP Credentials:**
    In `main/main.c`, update the following macros:
    ```c
    #define EXAMPLE_SIP_URI            "sip:YOUR_SIP_USERNAME@YOUR_SIP_DOMAIN" 
    #define EXAMPLE_SIP_SERVER         "YOUR_SIP_SERVER_IP_OR_DOMAIN"       
    #define EXAMPLE_SIP_PORT           5060                                 
    #define EXAMPLE_SIP_USERNAME       "YOUR_SIP_USERNAME"
    #define EXAMPLE_SIP_PASSWORD       "YOUR_SIP_PASSWORD"
    ```
    These macros are used to populate the `esp_sip_config_t` structure within the `event_handler` function (specifically under `IP_EVENT_STA_GOT_IP`).
    Replace `"YOUR_..."` placeholders with your actual network and SIP account details.

### 4. Build and Flash
Open a terminal with the ESP-IDF environment activated:
```bash
# Set your target ESP32 chip (e.g., esp32, esp32s3)
idf.py set-target esp32

# (Optional) Configure project specific settings (like I2S pins if you changed them from defaults)
idf.py menuconfig

# Build the project
idf.py build

# Flash the project to your ESP32 (replace (PORT) with your serial port, e.g., /dev/ttyUSB0 or COM3)
idf.py -p (PORT) flash

# Monitor the serial output
idf.py -p (PORT) monitor
```

## Testing
Refer to the `TESTING_GUIDE.md` for detailed steps on how to test the current functionalities, including local audio loopback and conceptual end-to-end SIP call tests.

## Next Steps / Future Development
This project provides a starting point. Significant development is required to make it a fully functional SIP phone:
*   **Integrate a Functional SIP Library:** Replace the placeholder `esp32_sip_voice` component with a robust SIP stack (e.g., PJSIP, SofiaSIP, or by completing the `GeorgeBregman/ESP32-SIP-Voice` library).
*   **Implement a Real G.711 Codec:** Replace the placeholder `codec` component with an actual G.711 A-law and/or u-law codec.
*   **RTP Handling:** Implement proper RTP (Real-time Transport Protocol) packetization and depacketization for audio streams, including jitter buffer management. This is usually part of a full SIP library.
*   **Advanced Call Features:** Add support for DTMF tones, call hold, call transfer, multiple calls, etc.
*   **Audio Quality Improvements:**
    *   Implement Acoustic Echo Cancellation (AEC).
    *   Add Noise Suppression.
    *   Fine-tune I2S settings and audio processing.
*   **User Interface:** Add buttons for dialing, answering, hanging up, and a display for status information.
*   **Error Handling & Robustness:** Improve error handling for network issues, SIP failures, and audio device problems.

## Contributing
Contributions are welcome! If you plan to contribute, please fork the repository and submit a pull request with your changes. For major changes, please open an issue first to discuss what you would like to add.
(Note: This is a generic contributing guideline as the project is currently maintained by an AI model.)

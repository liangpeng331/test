# ESP32 SIP Phone Hardware Setup

## 1. Overview

This document outlines the necessary hardware components and typical pin connections for building an ESP32-based SIP (Session Initiation Protocol) phone. This setup allows for voice communication over IP networks using an ESP32 microcontroller. The primary audio interface discussed is I2S for better quality, with a note on using the built-in DAC as a simpler alternative.

## 2. Core Components

The following components are typically required:

*   **ESP32 Module:**
    *   Example: ESP32-DevKitC, ESP32-WROOM-32 module, or similar.
    *   This is the main processor that handles Wi-Fi, networking, SIP communication, and audio processing.
*   **Microphone:**
    *   Example: I2S MEMS Microphone like INMP441.
    *   Used for capturing audio input. An I2S microphone is recommended for better audio quality.
*   **Speaker:**
    *   Example: 8 Ohm, 0.5W to 1W small speaker.
    *   Used for audio output.
*   **Audio Amplifier:**
    *   Example for I2S output: MAX98357A (digital I2S input, direct speaker output).
    *   Example for analog output (if using DAC): PAM8302A, LM386 (requires analog input).
    *   Necessary to drive the speaker. The choice depends on whether you use I2S or the ESP32's DAC for output.

## 3. Pin Connections (Example for I2S Audio)

The following are example pin connections. **Note:** Specific GPIO pins for I2S can often be configured in the ESP-IDF software, but these are common defaults. Always check your ESP32 module's pinout and component datasheets.

### 3.1. Microphone (e.g., I2S INMP441) to ESP32

*   `SCK` (Serial Clock) / `BCLK` (Bit Clock) -> `GPIO14` (Often used as I2S_BCLK)
*   `WS` (Word Select) / `LRC` (Left/Right Clock) -> `GPIO15` (Often used as I2S_LRC / I2S_WS)
*   `SD` (Serial Data) / `DOUT` (Data Out) -> `GPIO13` (Often used as I2S_DIN)
*   `VCC` -> `3.3V`
*   `GND` -> `GND`
*   `L/R` (Channel Select on INMP441):
    *   Connect to `GND` for Left channel.
    *   Connect to `VCC` for Right channel.
    *   (For mono, typically one channel is selected, or firmware handles it).

### 3.2. I2S Amplifier (e.g., MAX98357A) & Speaker to ESP32

This setup assumes you are using an I2S amplifier like the MAX98357A.

*   **Amplifier to ESP32:**
    *   `BCLK` (Bit Clock In) -> `GPIO26` (Can be configured as I2S_BCLK_OUT)
    *   `LRC` (Left/Right Clock In) / `WS` (Word Select) -> `GPIO25` (Can be configured as I2S_LRC_OUT / I2S_WS_OUT)
    *   `DIN` (Data In) -> `GPIO22` (Can be configured as I2S_DOUT)
    *   `GAIN` (Gain Control, MAX98357A specific):
        *   Default (15dB): No connection / Float
        *   12dB: Connect to VCC
        *   9dB: Connect to GND
        *   6dB: Connect to a resistor (see datasheet)
        *   3dB: Connect to a resistor (see datasheet)
    *   `SD` (Shutdown Pin, active LOW on MAX98357A):
        *   Connect to `3.3V` for normal operation.
        *   Connect to `GND` to shutdown (or control via an ESP32 GPIO for power saving).
    *   `VCC` (Amplifier Power) -> `3.3V` or `5V` (MAX98357A supports 2.5V-5.5V. Check your specific module).
    *   `GND` -> `GND`

*   **Speaker to Amplifier:**
    *   Speaker `+` -> Amplifier Speaker Output `+` (e.g., `SPK+` or `AOUT+`)
    *   Speaker `-` -> Amplifier Speaker Output `-` (e.g., `SPK-` or `AOUT-`)

## 4. Alternative Audio Output (ESP32 DAC)

As a simpler, lower-fidelity alternative to an external I2S DAC and amplifier, the ESP32 has two built-in 8-bit Digital-to-Analog Converters (DACs).

*   **DAC Pins:**
    *   `GPIO25` (DAC Channel 1)
    *   `GPIO26` (DAC Channel 2)
*   **Setup:**
    *   Connect one of the DAC pins (e.g., GPIO25) to the input of a simple analog audio amplifier like an LM386 or PAM8302A.
    *   The output of the analog amplifier then drives the speaker.
*   **Considerations:**
    *   Audio quality is significantly lower than I2S due to 8-bit resolution and inherent noise.
    *   Requires an analog amplifier.
    *   Simpler to wire if an external I2S amplifier is not available.

## 5. Notes

*   **GPIO Pin Configuration:** The specific GPIO pins used for I2S (BCLK, LRC/WS, DIN, DOUT) are generally configurable in the ESP-IDF software. The pins mentioned above are common examples but can be changed based on your board layout and other peripheral needs.
*   **Component Datasheets:** Always refer to the datasheets for your specific ESP32 module, microphone, and audio amplifier for accurate pinouts, voltage requirements, and other specifications.
*   **Power Supply:** A stable and clean power supply (typically 3.3V for the ESP32 and some peripherals, 5V for others like some amplifiers) is crucial for good audio quality and overall system stability. Use decoupling capacitors where recommended.
*   **I2S Port:** The ESP32 has two I2S peripherals (I2S0 and I2S1). Ensure your software configuration matches the hardware connections. For simultaneous input and output (full-duplex), both microphone and amplifier might need to be connected to the same I2S peripheral, or configured carefully if using separate peripherals.
*   **Breadboarding:** When prototyping, ensure good connections and minimize wire lengths for audio signals to reduce noise.

This guide provides a starting point. Your specific implementation may vary based on the chosen components and project requirements.

#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

#include "esp_err.h"
#include "driver/i2s.h"

// Configuration for I2S. Can be expanded.
// Using common defaults for INMP441 (mic) & MAX98357A (speaker amp)
// Mic (I2S_NUM_0 for input)
#define I2S_MIC_PORT_NUM      (I2S_NUM_0)
#define I2S_MIC_SAMPLE_RATE   (16000) // Or 8000 if preferred for G.711
#define I2S_MIC_CHANNEL_FMT   (I2S_CHANNEL_FMT_ONLY_LEFT) // INMP441 is mono
#define I2S_MIC_BITS_PER_SAMPLE (I2S_BITS_PER_SAMPLE_32BIT) // Read 32 bits, 24 are actual data, 18 from INMP441
#define I2S_MIC_BCLK_PIN      (GPIO_NUM_14) // Example pin, see HARDWARE_SETUP.md
#define I2S_MIC_LRC_PIN       (GPIO_NUM_15) // Example pin
#define I2S_MIC_DIN_PIN       (GPIO_NUM_13) // Example pin

// Speaker (I2S_NUM_1 for output, or use I2S_NUM_0 if ESP32 variant supports full duplex on one controller)
// For simplicity, let's assume we might use a different I2S port for output if available,
// or the user will ensure pins don't conflict if using same port.
// Often, ESP32s share BCLK/LRC for full duplex on a single I2S controller.
#define I2S_SPEAKER_PORT_NUM  (I2S_NUM_0) // Using same port for simplicity now
#define I2S_SPEAKER_SAMPLE_RATE (16000) // Must match mic for loopback/direct processing
#define I2S_SPEAKER_CHANNEL_FMT (I2S_CHANNEL_FMT_ONLY_LEFT) // Mono output
#define I2S_SPEAKER_BITS_PER_SAMPLE (I2S_BITS_PER_SAMPLE_16BIT) // MAX98357A often takes 16-bit
#define I2S_SPEAKER_BCLK_PIN  (GPIO_NUM_26) // Example pin
#define I2S_SPEAKER_LRC_PIN   (GPIO_NUM_25) // Example pin
#define I2S_SPEAKER_DOUT_PIN  (GPIO_NUM_22) // Example pin

// DMA buffer configuration
#define I2S_DMA_BUF_COUNT     (4)
#define I2S_DMA_BUF_LEN       (256) // Samples

/**
 * @brief Initialize I2S driver for microphone input and speaker output
 *
 * @return ESP_OK on success, ESP_FAIL otherwise
 */
esp_err_t audio_hal_init(void);

/**
 * @brief Read audio data from microphone
 *
 * @param data Buffer to store audio data
 * @param len Length of the buffer in bytes
 * @param bytes_read Number of bytes actually read
 * @param timeout_ms Timeout for the read operation
 * @return ESP_OK on success
 */
esp_err_t audio_hal_mic_read(void* data, size_t len, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Write audio data to speaker
 *
 * @param data Buffer containing audio data
 * @param len Length of the data in bytes
 * @param bytes_written Number of bytes actually written
 * @param timeout_ms Timeout for the write operation
 * @return ESP_OK on success
 */
esp_err_t audio_hal_speaker_write(const void* data, size_t len, size_t *bytes_written, uint32_t timeout_ms);

/**
 * @brief Deinitialize I2S driver
 *
 * @return ESP_OK on success
 */
esp_err_t audio_hal_deinit(void);

#endif // AUDIO_HAL_H

#include "audio_hal.h"
#include "esp_log.h"

static const char *TAG_AUDIO = "AUDIO_HAL";

esp_err_t audio_hal_init(void) {
    ESP_LOGI(TAG_AUDIO, "Initializing Audio HAL");

    // I2S configuration for Microphone (Input)
    i2s_config_t i2s_mic_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_MIC_SAMPLE_RATE,
        .bits_per_sample = I2S_MIC_BITS_PER_SAMPLE,
        .channel_format = I2S_MIC_CHANNEL_FMT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false, // APLL may not be usable for some sample rates / ESP32 variants
        .tx_desc_auto_clear = false, // Not used in RX mode
        .fixed_mclk = 0
    };

    // I2S configuration for Speaker (Output)
    // If using the same I2S port, some configs like sample_rate, bits_per_sample might need to be compatible
    // or ESP32 variant must support full-duplex with differing configurations.
    // For simplicity, we assume I2S_NUM_0 can do both RX and TX.
    // If I2S_SPEAKER_PORT_NUM is different, this would be a separate i2s_driver_install call.
    i2s_config_t i2s_speaker_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SPEAKER_SAMPLE_RATE,
        .bits_per_sample = I2S_SPEAKER_BITS_PER_SAMPLE,
        .channel_format = I2S_SPEAKER_CHANNEL_FMT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true, // Auto clear TX descriptors
        .fixed_mclk = 0
    };
    
    // Install and start I2S driver for Microphone
    // ESP_LOGI(TAG_AUDIO, "Installing I2S driver for Mic on port %d", I2S_MIC_PORT_NUM);
    // ESP_ERROR_CHECK(i2s_driver_install(I2S_MIC_PORT_NUM, &i2s_mic_config, 0, NULL));

    // Pin configuration for Microphone
    i2s_pin_config_t mic_pin_config = {
        .bck_io_num = I2S_MIC_BCLK_PIN,
        .ws_io_num = I2S_MIC_LRC_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE, // Not used for RX
        .data_in_num = I2S_MIC_DIN_PIN
    };
    // ESP_ERROR_CHECK(i2s_set_pin(I2S_MIC_PORT_NUM, &mic_pin_config));
    // ESP_LOGI(TAG_AUDIO, "Mic I2S pins configured: BCLK=%d, LRC=%d, DIN=%d", I2S_MIC_BCLK_PIN, I2S_MIC_LRC_PIN, I2S_MIC_DIN_PIN);

    // Install and start I2S driver for Speaker
    // Important: If I2S_MIC_PORT_NUM and I2S_SPEAKER_PORT_NUM are the same, 
    // driver install should only be called once. The mode should be I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX.
    // The current ESP-IDF driver might require separate ports for truly independent configurations or full duplex.
    // For now, this assumes they are configured for the same port or different ports.
    // A more robust implementation would check this.
    
    // Let's assume I2S_NUM_0 for both for now, configured for full duplex.
    // The mode in i2s_mic_config should be updated if installing once for both.
    // For ESP-IDF v4.x, use i2s_config_t and ensure bits_per_sample is compatible for TX/RX if using same port.
    // For ESP-IDF v5.x, i2s_std_config_t would be used with separate tx_chan_config and rx_chan_config.
    // The provided code uses older i2s_config_t, so we'll adapt for that style.
    i2s_config_t i2s_config_fullduplex = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = I2S_SPEAKER_SAMPLE_RATE, // Assuming mic and speaker will operate at same rate
        .bits_per_sample = I2S_SPEAKER_BITS_PER_SAMPLE, // Dominant bits_per_sample for the port
        .channel_format = I2S_SPEAKER_CHANNEL_FMT, // Assuming mono for both
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    ESP_LOGI(TAG_AUDIO, "Installing I2S driver for port %d (Full Duplex)", I2S_MIC_PORT_NUM);
    esp_err_t drv_install_err = i2s_driver_install(I2S_MIC_PORT_NUM, &i2s_config_fullduplex, 0, NULL);
    if (drv_install_err != ESP_OK) {
        ESP_LOGE(TAG_AUDIO, "Failed to install I2S driver: %s", esp_err_to_name(drv_install_err));
        return ESP_FAIL;
    }

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_BCLK_PIN, // Shared BCLK
        .ws_io_num = I2S_MIC_LRC_PIN,   // Shared LRC/WS
        .data_out_num = I2S_SPEAKER_DOUT_PIN,
        .data_in_num = I2S_MIC_DIN_PIN
    };
     ESP_LOGI(TAG_AUDIO, "I2S pins: BCLK=%d, LRC=%d, DIN=%d, DOUT=%d", 
        I2S_MIC_BCLK_PIN, I2S_MIC_LRC_PIN, I2S_MIC_DIN_PIN, I2S_SPEAKER_DOUT_PIN);

    esp_err_t set_pin_err = i2s_set_pin(I2S_MIC_PORT_NUM, &pin_config);
    if (set_pin_err != ESP_OK) {
        ESP_LOGE(TAG_AUDIO, "Failed to set I2S pins: %s", esp_err_to_name(set_pin_err));
        i2s_driver_uninstall(I2S_MIC_PORT_NUM); // Clean up driver
        return ESP_FAIL;
    }
    
    // If using different bits_per_sample for RX and TX on the same controller (ESP-IDF 4.x limitation for i2s_config_t)
    // you might need to use i2s_set_clk after install if finer control is needed and supported,
    // or ensure the primary bits_per_sample in i2s_config_fullduplex is what the TX path expects,
    // and handle data conversion for RX path if it provides more bits.
    // The INMP441 provides 24 bits of data but often padded to 32. MAX98357A takes 16-bit data.
    // This placeholder assumes Speaker BPS is the primary one for the driver config.
    // Data read from INMP441 (e.g. 32bit) will need to be processed/downsampled to 16bit for G711 and speaker.

    ESP_LOGI(TAG_AUDIO, "Audio HAL Initialized Successfully");
    return ESP_OK;
}

esp_err_t audio_hal_mic_read(void* data, size_t len, size_t *bytes_read, uint32_t timeout_ms) {
    if (!data || !bytes_read) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = i2s_read(I2S_MIC_PORT_NUM, data, len, bytes_read, pdMS_TO_TICKS(timeout_ms));
    if (ret != ESP_OK) {
        // Don't log error for timeout, it can be a normal case
        if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG_AUDIO, "I2S Read Error: %s", esp_err_to_name(ret));
        }
    }
    return ret;
}

esp_err_t audio_hal_speaker_write(const void* data, size_t len, size_t *bytes_written, uint32_t timeout_ms) {
    if (!data || !bytes_written) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = i2s_write(I2S_SPEAKER_PORT_NUM, data, len, bytes_written, pdMS_TO_TICKS(timeout_ms));
     if (ret != ESP_OK) {
        // Don't log error for timeout
        if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGE(TAG_AUDIO, "I2S Write Error: %s", esp_err_to_name(ret));
        }
    }
    return ret;
}

esp_err_t audio_hal_deinit(void) {
    ESP_LOGI(TAG_AUDIO, "Deinitializing Audio HAL");
    // Uninstall I2S driver
    esp_err_t err = i2s_driver_uninstall(I2S_MIC_PORT_NUM); // Assuming only one port was used
    if (err != ESP_OK) {
        ESP_LOGE(TAG_AUDIO, "Failed to uninstall I2S driver: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    return ESP_OK;
}

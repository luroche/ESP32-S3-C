// FUNCIONA A 75kHz  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "esp_timer.h"  
#include <inttypes.h>  // Para usar PRIu32 en printf

#define SAMPLE_RATE    75000  // 75 kHz total (25 kHz por canal)
#define DMA_BUFFER_LEN 4096   // Buffer de lectura aumentado

static const char *TAG = "ADC_DMA";
adc_continuous_handle_t adc_handle;

uint64_t last_time = 0;  
uint32_t sample_count = 0;  

void init_adc_dma() {
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 8192,
        .conv_frame_size = DMA_BUFFER_LEN,
    };
    adc_continuous_new_handle(&adc_config, &adc_handle);

    // Configuración de 3 canales
    adc_digi_pattern_config_t adc_pattern[] = {
        { .atten = ADC_ATTEN_DB_11, .channel = ADC_CHANNEL_0, .bit_width = ADC_BITWIDTH_12 },
        { .atten = ADC_ATTEN_DB_11, .channel = ADC_CHANNEL_1, .bit_width = ADC_BITWIDTH_12 },
        { .atten = ADC_ATTEN_DB_11, .channel = ADC_CHANNEL_2, .bit_width = ADC_BITWIDTH_12 },
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_RATE,  // 75 kHz total (25 kHz por canal)
        .conv_mode = ADC_CONV_BOTH_UNIT,  // Usa ambos ADCs para mejorar rendimiento
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
        .pattern_num = 3,
        .adc_pattern = adc_pattern,
    };
    
    adc_continuous_config(adc_handle, &dig_cfg);
    adc_continuous_start(adc_handle);

    last_time = esp_timer_get_time();  
}

void read_adc_dma() {
    uint8_t data[DMA_BUFFER_LEN] = {0};
    uint32_t bytes_read = 0;  // Cambiado a uint32_t

    while (1) {  
        if (adc_continuous_read(adc_handle, data, DMA_BUFFER_LEN, &bytes_read, 0) == ESP_OK) {
            sample_count += bytes_read / 3;  

            uint64_t current_time = esp_timer_get_time();  
            if (current_time - last_time >= 1000000) {  
                ESP_LOGI(TAG, "Muestras leídas en 1s: %" PRIu32, sample_count);

                sample_count = 0;     
                last_time = current_time;  
            }
        }
        
    }
}

void app_main() {
    init_adc_dma();
    read_adc_dma();  
}

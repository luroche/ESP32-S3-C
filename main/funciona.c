#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "esp_timer.h"  // Para medir tiempo en µs
#include <inttypes.h>


#define SAMPLE_RATE    80000  // 80 kHz por canal
#define DMA_BUFFER_LEN 2048   // Tamaño del buffer de lectura

static const char *TAG = "ADC_DMA";
adc_continuous_handle_t adc_handle;

uint64_t last_time = 0;  // Almacena el último tiempo en µs
uint32_t sample_count = 0;  // Contador total de muestras leídas

// Inicializa el ADC en modo continuo con DMA para 3 canales
void init_adc_dma() {
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 8192,
        .conv_frame_size = DMA_BUFFER_LEN,
    };
    adc_continuous_new_handle(&adc_config, &adc_handle);

    // Configuración de cada patrón para los 3 canales (0, 1 y 2)
    adc_digi_pattern_config_t adc_pattern[] = {
        { .atten = ADC_ATTEN_DB_11, .channel = ADC_CHANNEL_0, .bit_width = ADC_BITWIDTH_12 },
        { .atten = ADC_ATTEN_DB_11, .channel = ADC_CHANNEL_1, .bit_width = ADC_BITWIDTH_12 },
        { .atten = ADC_ATTEN_DB_11, .channel = ADC_CHANNEL_2, .bit_width = ADC_BITWIDTH_12 },
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_RATE * 3,  // 3 canales a 40 kHz cada uno
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 3,
        .adc_pattern = adc_pattern,
    };
    
    adc_continuous_config(adc_handle, &dig_cfg);
    adc_continuous_start(adc_handle);

    last_time = esp_timer_get_time();  // Guardar tiempo inicial (µs)
}

// Función para leer datos del ADC en modo DMA de forma continua
void read_adc_dma() {
    uint8_t data[DMA_BUFFER_LEN] = {0};
    uint32_t bytes_read = 0;

    while (1) {  
        // Lee los datos del ADC; el timeout se establece en 0 para no bloquear
        if (adc_continuous_read(adc_handle, data, DMA_BUFFER_LEN, &bytes_read, 0) == ESP_OK) {
            // Cada muestra consta de 3 bytes (1 byte para identificar el canal y 2 bytes para el valor ADC)
            sample_count += bytes_read / 3;

            uint64_t current_time = esp_timer_get_time();  // Tiempo actual en µs
            if (current_time - last_time >= 1000000) {  // Cada 1 segundo (1,000,000 µs)
                ESP_LOGI(TAG, "Muestras leídas en 1s: %" PRIu32, sample_count);

                // ESP_LOGI(TAG, "Muestras leídas en 1s: %d", sample_count);
                sample_count = 0;     // Reinicia el contador para el siguiente segundo
                last_time = current_time;  // Actualiza el tiempo de referencia
            }
        }
    }
}

void app_main() {
    init_adc_dma();
    read_adc_dma();  // Inicia la captura continua sin pausas
}

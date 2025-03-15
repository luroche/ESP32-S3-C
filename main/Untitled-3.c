#include "driver/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_BUFFER_SIZE 4096  // Buffer grande para evitar pérdidas de datos
#define SAMPLE_RATE_HZ  80000  // Aumentamos la frecuencia de muestreo (80kHz)

adc_continuous_handle_t adc_handle;

void adc_task(void *arg) {
    uint8_t result[ADC_BUFFER_SIZE];
    uint32_t length = 0;

    while (1) {
        esp_err_t ret = adc_continuous_read(adc_handle, result, ADC_BUFFER_SIZE, &length, 0); // No bloqueante
        if (ret == ESP_OK) {
            // Aquí procesamos los datos rápido
            printf("Leídos %d bytes de ADC\n", length);
        }
        // Pequeña pausa para evitar un bucle infinito sin control
        taskYIELD();  
    }
}

void app_main() {
    // Configurar ADC en modo continuo
    adc_continuous_config_t adc_config = {
        .max_store_buf_size = ADC_BUFFER_SIZE,
        .conv_frame_size = ADC_BUFFER_SIZE,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,  // Formato eficiente
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,     // Solo ADC1 para mayor velocidad
        .sample_freq_hz = SAMPLE_RATE_HZ,        // Frecuencia de muestreo
    };

    adc_continuous_new_handle(&adc_config, &adc_handle);
    adc_continuous_start(adc_handle); // Iniciar la conversión continua

    // Crear una tarea para procesar datos sin bloquear el sistema
    xTaskCreate(adc_task, "ADC Task", 4096, NULL, 5, NULL);
}

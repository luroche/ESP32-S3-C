#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define ADC_CHANNEL ADC1_CHANNEL_4
#define ADC_ATTEN ADC_ATTEN_DB_11
#define ADC_UNIT ADC_UNIT_1

void app_main(void) {
    // Configuración del ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);

    int lectura;
    int contador = 0;
    time_t inicio, fin;
    double tiempo_transcurrido;

    printf("Iniciando lecturas del ADC durante 1 segundo...\n");

    time(&inicio); // Guarda el tiempo de inicio

    while (1) {
        lectura = adc1_get_raw(ADC_CHANNEL);
        contador++;

        time(&fin); // Guarda el tiempo actual
        tiempo_transcurrido = difftime(fin, inicio);

        if (tiempo_transcurrido >= 1.0) {
            break; // Sale del bucle después de 1 segundo
        }
    }

    printf("Lecturas realizadas en 1 segundo: %d\n", contador);
}
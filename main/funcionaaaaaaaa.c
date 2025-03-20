// Lee cada 25000

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/adc.h"
#include "soc/adc_channel.h"
#include "driver/timer.h"

#define ADC_WIDTH        ADC_WIDTH_BIT_12   // Resolución de 12 bits para el ADC
#define ADC1_CHANNEL_0  ADC1_CHANNEL_0     // Canal 0 para ADC1
#define NUM_SAMPLES      3                  // Número de muestras a tomar por ADC

#define TIMER_DIVIDER    16                // Divisor del temporizador
#define TIMER_BASE_CLK   80000000          // 80 MHz (frecuencia base del temporizador en ESP32)
#define TIMER_SCALE      (TIMER_BASE_CLK / TIMER_DIVIDER)  // Frecuencia del temporizador
#define TIMER_INTERVAL    (TIMER_SCALE / 25000) // 25kHz (para 25kHz, 1 / 25kHz = 40us, luego el valor se ajusta en el código)

static uint32_t adc1_samples[NUM_SAMPLES];  // Buffer para almacenar las muestras de ADC
static volatile uint32_t interrupt_counter = 0;  // Contador de interrupciones

static _Bool IRAM_ATTR timer_isr(void *param) {
    static int sample_index = 0;
    
    // Leer el ADC (canal 0 de ADC1)
    adc1_samples[sample_index] = adc1_get_raw(ADC1_CHANNEL_0);
    
    sample_index++;
    if (sample_index >= NUM_SAMPLES) {
        sample_index = 0;  // Reinicia el índice para tomar nuevas muestras
    }
    
    // Incrementa el contador de interrupciones
    interrupt_counter++;
    
    // Limpiar la interrupción del temporizador
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    
    return pdTRUE;  // Indicar que la interrupción fue manejada correctamente
}

void app_main(void) {
    // Configuración de ADC
    esp_err_t ret = adc1_config_width(ADC_WIDTH);
    if (ret != ESP_OK) {
        ESP_LOGE("ADC", "Error en la configuración de la resolución del ADC");
        return;
    }

    ret = adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
    if (ret != ESP_OK) {
        ESP_LOGE("ADC", "Error en la configuración del canal de ADC");
        return;
    }

    // Configuración del temporizador
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
        .counter_dir = TIMER_COUNT_UP,
        .divider = TIMER_DIVIDER,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    // Configurar la interrupción del temporizador
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr, NULL, ESP_INTR_FLAG_IRAM);

    // Establecer el valor de la alarma para que el temporizador dispare la interrupción cada 40 us (25kHz)
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER_INTERVAL);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);

    // Iniciar el temporizador
    timer_start(TIMER_GROUP_0, TIMER_0);

    // Mensaje inicial
    ESP_LOGI("Main", "Iniciando muestreo ADC en tiempo real...");

    // Contador de interrupciones
    uint32_t last_interrupt_count = 0;
    uint32_t current_interrupt_count = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo antes de imprimir el contador de interrupciones

        // Verificar cuántas interrupciones han ocurrido en el último segundo
        current_interrupt_count = interrupt_counter;
        uint32_t interrupts_per_second = current_interrupt_count - last_interrupt_count;
        last_interrupt_count = current_interrupt_count;

        printf("Interrupciones por segundo: %lu\n", interrupts_per_second);

        // Aquí puedes agregar el código para procesar las muestras del ADC (en adc1_samples)
        // Este ciclo es solo para ilustrar cómo se tomarían las muestras. Aquí simplemente mostramos el valor de las muestras
        printf("Muestras del ADC:\n");
        for (int i = 0; i < NUM_SAMPLES; i++) {
            printf("Muestra %d: %lu\n", i, adc1_samples[i]);
        }
    }
}

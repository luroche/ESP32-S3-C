#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/adc.h"
#include "soc/adc_channel.h"
#include "driver/timer.h"


#define ADC_WIDTH        ADC_WIDTH_BIT_12   // Resolución de 12 bits para el ADC
#define ADC1_CHANNEL_0  ADC1_CHANNEL_0     // Canal 0 para ADC1
#define NUM_SAMPLES      200                  // Número de muestras a tomar por ADC

#define TIMER_DIVIDER    16                // Divisor del temporizador
#define TIMER_BASE_CLK   80000000          // 80 MHz (frecuencia base del temporizador en ESP32)
#define TIMER_SCALE      (TIMER_BASE_CLK / TIMER_DIVIDER)  // Frecuencia del temporizador
#define TIMER_INTERVAL    (TIMER_SCALE / 20) // 25kHz (para 25kHz, 1 / 25kHz = 40us, luego el valor se ajusta en el código)

static uint32_t adc1_samples[NUM_SAMPLES];  // Buffer para almacenar las muestras de ADC
static volatile uint32_t interrupts_per_second = 0;  // Contador de interrupciones por segundo

// Cola para enviar las muestras a la tarea de procesamiento
QueueHandle_t GlobalQueue = 0;
const char *tag = "main";

// Contador global para el número de ejecuciones de process_adc_samples
volatile uint32_t process_adc_count = 0;

// Función de la interrupción del temporizador
static _Bool IRAM_ATTR timer_isr(void *param) {
    static int sample_index = 0;
    
    // Leer el ADC (canal 0 de ADC1)
    adc1_samples[sample_index] = adc1_get_raw(ADC1_CHANNEL_0);
    if (!xQueueSend(GlobalQueue, &adc1_samples[sample_index], pdMS_TO_TICKS(100))) // si no se pudo enviar, entonces si la cola esta llena retornara este error
    {
        ESP_LOGE(tag, "ERROR ENVIANDO");
        // ESP_LOGE(tag, "Error sending %u to queue", adc1_samples[sample_index]); // Error si no se envia
    }

    sample_index++;
    if (sample_index >= NUM_SAMPLES) {
        // Se han tomado todas las muestras
        // Enviar las muestras a la cola
        // xQueueSendFromISR(adc_samples_queue, adc1_samples, NULL);
        sample_index = 0;  // Reinicia el índice para tomar nuevas muestras
    }

    // Incrementa el contador de interrupciones por segundo
    interrupts_per_second++;
    
    // Limpiar la interrupción del temporizador
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    
    return pdTRUE;  // Indicar que la interrupción fue manejada correctamente
}

// Tarea para procesar las muestras de ADC y contar las ejecuciones
void process_adc_samples(void *pvParameter) {
    uint32_t samples[NUM_SAMPLES];
    uint32_t receivedValue = 0;  // Cambiar a uint32_t

    while (1) {
        if (xQueueReceive(GlobalQueue, &receivedValue, pdMS_TO_TICKS(100)) == pdTRUE) {  // Recibir de la cola
            process_adc_count++;
            // ESP_LOGI(tag, "Value received %lu from queue", receivedValue);  // Si se recibe
        } else {
            ESP_LOGE(tag, "Error receiving value from queue"); // Error si no se recibe
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Asegurarse de no bloquearse indefinidamente
        // // Recibe todo el tamaño de la cola de una vez
        // if (xQueueReceive(adc_samples_queue, samples, sizeof(samples)) == pdTRUE) {
        //     // Incrementar el contador de ejecuciones de la tarea
        //     process_adc_count++;
        //
        //     // No hacemos nada con las muestras, solo incrementamos el contador
        //     // Si quisieras procesar las muestras, lo harías aquí, por ejemplo, imprimiéndolas.
        //     // Pero ahora simplemente incrementamos el contador.
        //     // ESP_LOGI("Process", "Contador de ejecuciones de process_adc_samples: %d", process_adc_count);
        // }

        // vTaskDelay(pdMS_TO_TICKS(1000));  // Espera de 1 segundo antes de la siguiente iteración
    }
}

void app_main(void) {
    printf("HOLAAAAAAAAAA");
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
    
    // Crear la cola para las muestras del ADC
    GlobalQueue = xQueueCreate(NUM_SAMPLES, sizeof(uint32_t));
    if (GlobalQueue == NULL) {
        ESP_LOGE("Queue", "No se pudo crear la cola para las muestras del ADC");
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

    // Crear la tarea para procesar las muestras del ADC
    xTaskCreate(process_adc_samples, "process_adc_samples", 2048, NULL, 5, NULL);

    // Mensaje inicial
    ESP_LOGI("Main", "Iniciando muestreo ADC en tiempo real...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo

        // Verificar cuántas veces se ha ejecutado process_adc_samples en el último segundo
        uint32_t executions_in_last_second = process_adc_count;
        process_adc_count = 0;  // Resetea el contador de ejecuciones para el siguiente segundo
        
        printf("El contador de ejecuciones de process_adc_samples en el último segundo es: %lu\n", executions_in_last_second);

        // También se pueden verificar las interrupciones por segundo si es necesario
        uint32_t interrupts_in_last_second = interrupts_per_second;
        interrupts_per_second = 0;  // Resetea el contador de interrupciones para el siguiente segundo
        printf("Interrupciones en el último segundo: %lu\n", interrupts_in_last_second);
    }
}
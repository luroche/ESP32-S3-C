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
#include "lwip/sockets.h"


#define NUM_SAMPLES 200  // Número de muestras por paquete UDP
#define ADC_CHANNEL ADC1_CHANNEL_0  // Canal del ADC (ajustar según el hardware)

// Buffers de almacenamiento
static uint16_t buffer1[NUM_SAMPLES];
static uint16_t buffer2[NUM_SAMPLES];
volatile uint16_t *active_buffer = buffer1;
volatile uint16_t *ready_buffer = buffer2;
static volatile int index_lectura = 0;

// Semáforo para sincronización
static SemaphoreHandle_t sem_buffer_ready;


#define ADC_WIDTH        ADC_WIDTH_BIT_12   // Resolución de 12 bits para el ADC
#define ADC1_CHANNEL_0  ADC1_CHANNEL_0     // Canal 0 para ADC1

#define TIMER_DIVIDER    16                // Divisor del temporizador
#define TIMER_BASE_CLK   80000000          // 80 MHz (frecuencia base del temporizador en ESP32)
#define TIMER_SCALE      (TIMER_BASE_CLK / TIMER_DIVIDER)  // Frecuencia del temporizador
#define TIMER_INTERVAL    (TIMER_SCALE / 25000) // 25kHz (para 25kHz, 1 / 25kHz = 40us, luego el valor se ajusta en el código)

static uint32_t adc1_samples[NUM_SAMPLES];  // Buffer para almacenar las muestras de ADC
static volatile uint32_t interrupts_per_second = 0;  // Contador de interrupciones por segundo


const char *tag = "main";

// Contador global para el número de ejecuciones de process_adc_samples
volatile uint32_t count_udps = 0;


// Función de la interrupción del temporizador
static _Bool IRAM_ATTR timer_isr(void *arg) {
    // Leer el ADC (canal 0 de ADC1)
    // adc1_samples[sample_index] = adc1_get_raw(ADC1_CHANNEL_0);

    active_buffer[index_lectura] = 5048;
    index_lectura++;

    if (index_lectura >= NUM_SAMPLES) {
        // Intercambiar buffers
        uint16_t *temp = ready_buffer;
        ready_buffer = active_buffer;
        active_buffer = temp;
        // Avisar a la tarea de envío
        xSemaphoreGive(sem_buffer_ready);
        
        // Reiniciar índice
        index_lectura = 0;
    }


    // Incrementa el contador de interrupciones por segundo
    interrupts_per_second++;
    
    // Limpiar la interrupción del temporizador
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    
    return pdTRUE;  // Indicar que la interrupción fue manejada correctamente
}








// Función de envío UDP
void udp_task(void *arg) {
    // struct sockaddr_in dest_addr;
    // int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    // dest_addr.sin_addr.s_addr = inet_addr("192.168.1.100"); // IP destino
    // dest_addr.sin_family = AF_INET;
    // dest_addr.sin_port = htons(1234); // Puerto destino

    while (1) {
        // Esperar a que el buffer esté listo
        if (xSemaphoreTake(sem_buffer_ready, portMAX_DELAY)) {
            // ESP_LOGI(tag, "Recibe");  // Si se recibe
            count_udps++;
            // vTaskDelay(pdMS_TO_TICKS(1000));
            // sendto(sock, ready_buffer, NUM_SAMPLES * sizeof(uint16_t), 0, 
            //        (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        }
    }
}



void app_main(void) {
    printf("Comenzamos 2...");
    vTaskDelay(pdMS_TO_TICKS(3000));
    
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
    
    // Crear semáforo
    sem_buffer_ready = xSemaphoreCreateBinary();


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
    xTaskCreate(udp_task, "udp_task", 2048, NULL, 5, NULL);

    // Mensaje inicial
    ESP_LOGI("Main", "Iniciando muestreo ADC en tiempo real...");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Esperar 1 segundo

        // Verificar cuántas veces se ha ejecutado udp_task en el último segundo
        uint32_t executions_in_last_second = count_udps;
        count_udps = 0;  // Resetea el contador de ejecuciones para el siguiente segundo
        printf("El contador de ejecuciones de udp_task en el último segundo es: %lu\n", executions_in_last_second);


        // También se pueden verificar las interrupciones por segundo si es necesario
        uint32_t interrupts_in_last_second = interrupts_per_second;
        interrupts_per_second = 0;  // Resetea el contador de interrupciones para el siguiente segundo
        printf("Interrupciones en el último segundo: %lu\n", interrupts_in_last_second);
    }
}
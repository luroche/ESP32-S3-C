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

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#define WIFI_SSID "WLAN_4110"
#define WIFI_PASS "IBVJ7U3RBIDB2OI26Y4K"

#define UDP_SERVER_IP "192.168.2.33" // Reemplaza con la IP del servidor destino
#define UDP_PORT 8888            // Puerto del servidor

static const char *TAG = "UDP_CLIENT";



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

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
    ESP_LOGI(TAG, "Intentando conectar al WiFi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Conectado al WiFi con IP: " IPSTR, IP2STR(&event->ip_info.ip));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    ESP_LOGE(TAG, "Desconectado del WiFi. Intentando reconectar...");
    esp_wifi_connect();
    }
}


// Función para inicializar WiFi en modo STA
void wifi_init_sta() { 
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    // Desconectar cualquier conexión previa (si hay alguna)
    esp_wifi_disconnect();

    // Limpiar cualquier configuración de WPA2 Enterprise (si es necesario)
    // esp_wifi_sta_wpa2_ent_clear(); // Si no usas WPA2 Enterprise, no es necesario

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,  // Forzar WPA2
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}


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
    struct sockaddr_in server_addr;
    int sock;
    int vector[] = {10, 20, 30}; // Vector a enviar
    size_t vector_size = sizeof(vector);

    // Crear socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        ESP_LOGE(TAG, "Error creando socket UDP");
        vTaskDelete(NULL);
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT); // Puerto destino
    server_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP); //IP destino

    while (1) {
        // Esperar a que el buffer esté listo
        if (xSemaphoreTake(sem_buffer_ready, portMAX_DELAY)) {
            // ESP_LOGI(tag, "Recibe");  // Si se recibe
            count_udps++;
            // ESP_LOGI(TAG, "Enviando datos a: %s:%d", UDP_SERVER_IP, UDP_PORT);
            int sent_bytes = sendto(sock, ready_buffer, NUM_SAMPLES * sizeof(uint16_t), 0,
                                    (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (sent_bytes < 0) {
                ESP_LOGE(TAG, "Error enviando datos: %s", strerror(errno));
            } else {
                ESP_LOGI(TAG, "Enviado %d bytes", sent_bytes);
            }
            // vTaskDelay(pdMS_TO_TICKS(2000)); // Esperar 2 segundos
        }
    }
    close(sock);
    vTaskDelete(NULL);
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

        

    // Configuración del Wifi
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    printf("Comenzamos señores PARTE 5...");
    // Inicializar NVS para almacenamiento persistente (requerido por WiFi)
    esp_err_t ret2 = nvs_flash_init();
    if (ret2 == ESP_ERR_NVS_NO_FREE_PAGES || ret2 == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta(); // Iniciar WiFi
    


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
    xTaskCreate(udp_task, "udp_task", 4096, NULL, 5, NULL);

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
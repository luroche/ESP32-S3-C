#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "driver/adc.h"
#include "driver/timer.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

#define ADC1_CHANNEL_0 ADC1_CHANNEL_0  // GPIO 36
#define ADC1_CHANNEL_3 ADC1_CHANNEL_3  // GPIO 39
#define ADC1_CHANNEL_6 ADC1_CHANNEL_6  // GPIO 34

#define NUM_MUESTRAS 180     // Tamaño del buffer # SI PONGO 150 se enviaran 500 paquetes al segundo, y si pongo 200 se envian 375 al segundo 
#define MUESTREO_HZ 25000    // Frecuencia de muestreo (25 kHz)
#define TIMER_DIVIDER 80     // Divide el clock base (80 MHz → 1 MHz)

// Configuración UDP
#define REMOTE_IP "10.0.0.1"
#define REMOTE_PORT 8888

uint32_t adc_samples[NUM_MUESTRAS];  // Buffer de datos ADC
volatile int sample_index = 0;       // Índice para almacenar datos

void send_udp_data() {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("Error creando socket UDP");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(REMOTE_PORT);
    server_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);

    size_t data_size = sizeof(adc_samples);
    int sent_bytes = sendto(sock, adc_samples, data_size, 0,
                            (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (sent_bytes < 0) {
        perror("Error enviando datos UDP");
    } else {
        printf("Enviadas %d bytes por UDP\n", sent_bytes);
    }

    close(sock);
}

// ISR del temporizador: Lee 3 ADC y guarda en el buffer
bool IRAM_ATTR timer_isr(void *param) {
    if (sample_index < NUM_MUESTRAS) {
        adc_samples[sample_index++] = adc1_get_raw(ADC1_CHANNEL_0);
        adc_samples[sample_index++] = adc1_get_raw(ADC1_CHANNEL_3);
        adc_samples[sample_index++] = adc1_get_raw(ADC1_CHANNEL_6);
    }
    
    if (sample_index >= NUM_MUESTRAS) {
        send_udp_data();  // Enviar datos por UDP
        sample_index = 0; // Reiniciar índice
    }

    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);  // Limpiar la interrupción
    return true;  // Asegurar compatibilidad con timer_isr_callback_add
}

// Configurar y activar el temporizador
void setup_timer() {
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = true,
    };
    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    double timer_interval_sec = 1.0 / MUESTREO_HZ;
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_interval_sec * 1000000);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_callback_add(TIMER_GROUP_0, TIMER_0, timer_isr, NULL, ESP_INTR_FLAG_IRAM);
    timer_start(TIMER_GROUP_0, TIMER_0);
}

// Configurar ADC
void setup_adc() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
}

void app_main() {
    setup_adc();
    setup_timer();
    printf("Iniciando muestreo ADC a 25 kHz y envío por UDP...\n");
}

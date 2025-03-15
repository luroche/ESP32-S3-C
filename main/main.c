#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_eth.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

// Defino puertos de los sensores ASC712
#define CURRENT_SENSOR_1 GPIO_NUM_5
#define CURRENT_SENSOR_2 GPIO_NUM_6
#define CURRENT_SENSOR_3 GPIO_NUM_7

// Defino puertos del UDP
#define W5500_RST_PIN GPIO_NUM_9
#define W5500_CS_PIN GPIO_NUM_10

#define UDP_PORT 9999
#define REMOTE_PORT 8888
#define REMOTE_IP "10.0.0.1"

#define SENSIBILIDAD_ACS712_30A 0.66
#define NUM_MENSAJES_UDP (60 * 3)
#define INTERVALO_ENVIO_MS 5000

int datos[NUM_MENSAJES_UDP];


// PROGRAMA
void reset_w5500() {
    gpio_set_level(W5500_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(W5500_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void configurar_adc() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
}

void enviar_udp(int *datos, size_t size) {
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(REMOTE_PORT);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        printf("Error al crear socket UDP\n");
        return;
    }

    sendto(sock, datos, size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    close(sock);
}

void sistema() {
    printf("Enviando datos...\n");
    unsigned long startTime = esp_timer_get_time() / 1000;
    int num_envios = 0;
    int index = 0;

    // TODO: CONSEGUIR LEER COMO MINIMO 20-25 kSPS LAS TRES MEDIDAS A LA VEZ
    int c = 0;
    while (c <= 5){ 
    // while ((esp_timer_get_time() / 1000) - startTime < INTERVALO_ENVIO_MS) {
        unsigned long tiempo_inicio = esp_timer_get_time();

        int valor1 = adc1_get_raw(ADC1_CHANNEL_4);
        int valor2 = adc1_get_raw(ADC1_CHANNEL_5);
        int valor3 = adc1_get_raw(ADC1_CHANNEL_6);
        
        // float corriente1 = ((valor1 * (3.3 / 4095.0)) - 2.5) / SENSIBILIDAD_ACS712_30A;
        // float corriente2 = ((valor2 * (3.3 / 4095.0)) - 2.5) / SENSIBILIDAD_ACS712_30A;
        // float corriente3 = ((valor3 * (3.3 / 4095.0)) - 2.5) / SENSIBILIDAD_ACS712_30A;
        
        int corriente1 = ((valor1 * (3300 / 4095000)) - 2500) / SENSIBILIDAD_ACS712_30A; // 3.3V -> 3300 mV;   4095 y    2.5V
        int corriente2 = ((valor2 * (3300 / 4095000)) - 2500) / SENSIBILIDAD_ACS712_30A;
        int corriente3 = ((valor3 * (3300 / 4095000)) - 2500) / SENSIBILIDAD_ACS712_30A;

        datos[index++] = (int)(corriente1);
        datos[index++] = (int)(corriente2);
        datos[index++] = (int)(corriente3);
        
        unsigned long tiempo_fin = esp_timer_get_time();
        printf("Tiempo de lectura ADC3: %lu us\n", tiempo_fin - tiempo_inicio);

        // TODO: Al meter cualquier tipo de delay aquí para cumplir un segundo se bloquea el programa


        
        // TODO: No he comprobado a mandarlo por UDP, pero con la programacion en arduino me lo hacía.
        // Compruebo a mandarlos por udp
        // TODO: En arduino no se podian enviar mas de 60*3 a la vez
        if (index >= NUM_MENSAJES_UDP) {
            enviar_udp(datos, sizeof(datos));
            index = 0;
            num_envios++;
        }
        c += 1;
    }
    printf("C: %d ;", c);
    printf("NUM ENVIOS: %d\n", num_envios);
}

void app_main() {
    configurar_adc();
    reset_w5500();

    char entrada[10];
    while (1) {
        printf("Ingrese '1' para iniciar el sistema: ");
        fflush(stdout); // TODO: Asegurar que el mensaje se imprime antes de esperar entrada
        if (fgets(entrada, sizeof(entrada), stdin) != NULL) {
            if (entrada[0] == '1' && (entrada[1] == '\n' || entrada[1] == '\0')) {
                sistema();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // TODO: Pequeño retraso para evitar consumo excesivo de CPU
    }
}

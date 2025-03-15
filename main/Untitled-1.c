#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_system.h"

#define MAX_SIZE 1000  // Definimos el tamaño máximo del arreglo


void app_main() {
    int medidas1[MAX_SIZE];  // Definimos un arreglo estático con 10 elementos
    int i = 0;
    
    
    // Configurar ADC1 con resolución de 12 bits
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Configurar canales para los sensores en GPIO 5, 6 y 7
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);  // GPIO 5
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);  // GPIO 6
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);  // GPIO 7
    printf("hola\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);  // Retraso de 1 ms para evitar bloqueos innecesarios
    uint64_t start_time = esp_timer_get_time();  // Captura el tiempo inicial en microsegundos

    // 100000   100 milis
    // 1000000 un segundo

    // Ejecutar el bucle mientras no hayan pasado 1 segundo o i < 10
    int medida = 0;
    while (i < MAX_SIZE) {
        medida = adc1_get_raw(ADC1_CHANNEL_4);
        // medidas1[i] = adc1_get_raw(ADC1_CHANNEL_4);  // Lectura de sensor en GPIO 5
        // printf("Lectura %d: %d\n", i, medidas1[i]);  // Imprime la lectura
        // printf("Lectura %d: %d\n", i, medidas1[i]);

        i++;  // Incrementa el índice
        // printf("I: %d", i);
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Retraso de 1 ms para evitar bloqueos innecesarios
    }

    // Después de que termine el bucle
    printf("Bucle terminado. Se han realizado %d lecturas.\n", i);
    // vTaskDelay(5000 / portTICK_PERIOD_MS);  // Retraso de 5seg

    // int cd;
    // for (int cd=0; cd<MAX_SIZE; cd++){
    //     printf("%d \n", medidas1[cd]);
    // }

    // // Inicializar el arreglo con valores
    // for (i = 0; i < MAX_SIZE; i++) {
    //     medidas1[i] = i + 1;  // Asignamos valores 1, 2, 3, ..., 10
    // }

    // // Mostrar el arreglo antes del bucle
    // printf("Arreglo antes del bucle:\n");
    // for (i = 0; i < MAX_SIZE; i++) {
    //     printf("%d ", medidas1[i]);
    // }
    // printf("\n");

    // // Completar el arreglo con más valores en un bucle
    // for (i = 0; i < MAX_SIZE; i++) {
    //     medidas1[i] = medidas1[i] * 2;  // Multiplicamos cada valor por 2
    // }

    // // Mostrar el arreglo después del bucle
    // printf("Arreglo después de completar el bucle:\n");
    // for (i = 0; i < MAX_SIZE; i++) {
    //     printf("%d ", medidas1[i]);
    // }
    // printf("\n");

    
    // // Reservar memoria dinámica para los datos
    // int *medidas1 = (int *)malloc(NUM_MEDIDAS * sizeof(int));
    // int *medidas2 = (int *)malloc(NUM_MEDIDAS * sizeof(int));
    // int *medidas3 = (int *)malloc(NUM_MEDIDAS * sizeof(int));
    
    // if (medidas1 == NULL || medidas2 == NULL || medidas3 == NULL) {
    //     printf("Error: No se pudo asignar memoria\n");
    //     return;
    // }
    
    // printf("Iniciando lectura de datos...\n");
    
    // // Leer valores y almacenarlos en los vectores dinámicos
    // int c = 0;
    
    // // Obtener el tiempo de inicio
    // time_t start_time = time(NULL);
    
    // // Ejecutar el bucle mientras no hayan pasado 1 segundo
    // while (time(NULL) - start_time < 1) {
        //     // Realizar alguna acción mientras no pase 1 segundo
        //     medidas1[i] = adc1_get_raw(ADC1_CHANNEL_4);  // Lectura de sensor en GPIO 5
        //     medidas2[i] = adc1_get_raw(ADC1_CHANNEL_5);  // Lectura de sensor en GPIO 6
        //     medidas3[i] = adc1_get_raw(ADC1_CHANNEL_6);  // Lectura de sensor en GPIO 7
        //     c +=1;
        // }
        // print(c)
        
        // for (int i = 0; i < NUM_MEDIDAS; i++) {
            //     medidas1[i] = adc1_get_raw(ADC1_CHANNEL_4);  // Lectura de sensor en GPIO 5
            //     medidas2[i] = adc1_get_raw(ADC1_CHANNEL_5);  // Lectura de sensor en GPIO 6
            //     medidas3[i] = adc1_get_raw(ADC1_CHANNEL_6);  // Lectura de sensor en GPIO 7
    //     vTaskDelay(pdMS_TO_TICKS(1));  // Espera 1ms entre lecturas
    // }

    // printf("Lecturas completadas. Mostrando primeros valores:\n");
    // for (int i = 0; i < 10; i++) {
        //     printf("[%d] Sensor1: %d | Sensor2: %d | Sensor3: %d\n", 
        //             i, medidas1[i], medidas2[i], medidas3[i]);
        //     // }
        
        //     // Liberar la memoria cuando ya no se necesite
        //     free(medidas1);
        //     free(medidas2);
        //     free(medidas3);
        
        
        //     printf("Memoria liberada. Fin del programa.\n");
        // }
        
}
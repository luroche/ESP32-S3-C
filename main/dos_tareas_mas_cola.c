// programa de dos tareas, que se comunican a través de una cola

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#define ledR 33
#define ledG 25
#define STACK_SIZE 1024 * 2 // tamaño de memoria para cada tarea
#define R_delay 400
#define G_delay 2000

QueueHandle_t GlobalQueue = 0;

const char *tag = "Main";

esp_err_t init_led(void);
esp_err_t create_tasks(void);
void vTaskR(void *pvParameters);
void vTaskG(void *pvParameters);

void app_main(void)
{
    GlobalQueue = xQueueCreate(10, sizeof(uint32_t));
    init_led();
    create_tasks();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        printf("hello main \n");
        printf("Numero de nucleos (cores): %i \n", portNUM_PROCESSORS);
    }
}

esp_err_t init_led()
{
    gpio_reset_pin(ledR);
    gpio_set_direction(ledR, GPIO_MODE_OUTPUT);
    gpio_reset_pin(ledG);
    gpio_set_direction(ledG, GPIO_MODE_OUTPUT);
    return ESP_OK;
}

esp_err_t create_task(void)
{
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;
    xTaskCreatePinnedToCore(vTaskR,
                            "vTaskR",
                            &ucParameterToPass,
                            1,
                            &xHandle,
                            0);

    xTaskCreatePinnedToCore(vTaskG,
                            "vTaskG",
                            &ucParameterToPass,
                            1,
                            &xHandle,
                            1);
    return ESP_OK;
}

void vTaskR(void *pvParameters)
{
    while (1)
    {
        for (size_t i = 0; i < 8; i++)
        {
            vTaskDelay(pdMS_TO_TICKS(R_delay / 2));
            gpio_set_level(ledR, 1);
            ESP_LOGW(tag, "Sending %i to queue", i);              // Un warning
            if (!xQueueSend(GlobalQueue, &i, pdMS_TO_TICKS(100))) // si no se pudo enviar, entonces si la cola esta llena retornara este error
            {
                ESP_LOGE(tag, "Error sending %i to queue", i); // Error si no se envia
            }
            vTaskDelay(pdMS_TO_TICKS(R_delay / 2));
            gpio_set_level(ledR, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(7000));
    }
}

void vTaskG(void *pvParameters)
{
    int receivedValue = 0;
    
    while(1)
    {
        if (!xQueueSend(GlobalQueue, &receivedValue, pdMS_TO_TICKS(100)))
        {
            ESP_LOGE(tag, "Error receiving value from queue"); // Error si no se recibe
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(G_delay / 2));
            gpio_set_level(ledR, 1);
            ESP_LOGI(tag, "Value received %i from queue", receivedValue); // Si se recibe
        }
        vTaskDelay(pdMS_TO_TICKS(G_delay / 2));
        gpio_set_level(ledR, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// void vTaskG(void *pvParameters)
// {
//     while(1)
//     {
//         ESP_LOGI(tag, "Led G Core 0");
//         gpio_set_level(ledR, 1);
//         vTaskDelay(pdMS_TO_TICKS(G_delay));
//         gpio_set_level(ledR, 0);
//         vTaskDelay(pdMS_TO_TICKS(G_delay));
//     }
// }
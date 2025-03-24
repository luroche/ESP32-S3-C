// programa de dos tareas, que se comunican a través de una cola
// https://www.youtube.com/watch?v=YuuQ6YyFqus&list=PL-Hb9zZP9qC65SpXHnTAO0-qV6x5JxCMJ&index=6
// En el final del video, en los logs, sale un numero al comienzo que es cada cuanto se ejecuta ese comando

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

#define led1 2
uint8_t led_level = 0;
static const char *tag ="Main";
TimerHandle_t xTimers;
int interval = 1000; //Esta en milisegundos: 1 segundo = 1000; 
int timerId = 1;


esp_err_t init_led(void);
esp_err_t blink_led(void);
esp_err_t set_timer(void); //inicializar el timer para ejecutarse cada cierto tiempo

void vTimerCallback(TimerHandle_t pxTimer)
{
    ESP_LOGI(tag, "Event was called from timer");
    blink_led();
}

void app_main(void)
{
    printf("HOLA MUNDO!!");
    init_led();
    set_timer();
}

esp_err_t init_led(void)
{
    gpio_reset_pin(led1);
    gpio_set_direction(led1, GPIO_MODE_OUTPUT);
    return ESP_OK;
}

esp_err_t blink_led(void)
{
    led_level = !led_level;
    gpio_set_level(led1, led_level);
    return ESP_OK;
}


esp_err_t set_timer(void)
{
    ESP_LOGI(tag, "Timer init configuration");
    xTimers = xTimerCreate( "Timer",        // Just a text name, not used by the kernel.
                            (pdMS_TO_TICKS(interval)), // The time period in ticks, intervalos en periodos de la cpu. Con esa funcion convierte los milisegundos en periodos de cpu
                            pdTRUE,         // The timers will auto-reload themselves when they expire.
                            (void *)timerId,      // Assign each timer a unique id equal to its array index.
                            vTimerCallback  // Each timer calls the same callback when it expires. Funcion que se ejecuta
    );
    if (xTimers == NULL)
    {
        // The timer was not created.
        ESP_LOGE(tag, "The timer was not created");
    }
    else
    {
        if (xTimerStart(xTimers, 0) != pdPASS)
        {
            // The timer could not be set into the Active state.
            ESP_LOGE(tag, "The timer could not be set into the Active state");
        }
    }
    return ESP_OK;
}
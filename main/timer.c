// programa de dos tareas, que se comunican a través de una cola
// https://www.youtube.com/watch?v=YuuQ6YyFqus&list=PL-Hb9zZP9qC65SpXHnTAO0-qV6x5JxCMJ&index=6
// En el final del video, en los logs, sale un numero al comienzo que es cada cuanto se ejecuta ese comando

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_pm.h"

#define led1 2
uint8_t led_level = 0;
static const char *tag ="main"; //Etiqueta de donde vienen los logs
TimerHandle_t xTimers;
int interval = 1000; //Esta en milisegundos: 1 segundo = 1000; 
int timerId = 1;
volatile int contador = 0;

esp_err_t init_led(void);
esp_err_t blink_led(void);
esp_err_t set_timer(void); //inicializar el timer para ejecutarse cada cierto tiempo

void vTimerCallback(TimerHandle_t pxTimer)
{
    ESP_LOGI(tag, "Event was called from timer");
    blink_led();
}

void IRAM_ATTR my_timer_callback(void *arg)
{
    contador++;
}

void print_task(void *arg)
{
    while (1){
        ESP_LOGI(tag, "¡Timer ejecutado %d veces!", contador);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void app_main(void)
{
    printf("HOLA MUNDO!!");
    // Configurar el PM (Power Management) para asegurar que la CPU esté a 240 MHz
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 240,  // Asegúrate de que la CPU esté a 240 MHz
        .min_freq_mhz = 80,   // Frecuencia mínima permitida (80 MHz)
        .light_sleep_enable = false  // No permitir el modo de bajo consumo (light sleep)
    };
    esp_pm_configure(&pm_config);  // Aplicar la configuración

    
    
    // Crear un temporizador de 40 microsegundos
    const esp_timer_create_args_t timer_args = {
        .callback = &my_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_ISR, // habilitar idf.py menuconfig -> Component config  ---> ESP Timer -> [*] Enable ISR dispatch method for esp_timer
        .name = "my_timer"
    };
    
    esp_timer_handle_t my_timer;
    esp_timer_create(&timer_args, &my_timer);
    
    // Iniciar el timer para que se ejecute después de 40 µs
    esp_timer_start_periodic(my_timer, 40); // 40 µs
    printf("Hola otra vez!");
    // init_led();
    // set_timer();
    // xTaskCreate(print_task,"print_task", 2048, NULL, 1, NULL);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); //Esperar un segundo
        ESP_LOGI(tag, "¡Timer ejecutado %d veces!", contador);
        contador = 0; //Reiniciar el contador
    }

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


// esp_err_t set_timer(void)
// {
//     ESP_LOGI(tag, "Timer init configuration");
//     xTimers = xTimerCreate( "Timer",        // Just a text name, not used by the kernel.
//                             (pdMS_TO_TICKS(interval)), // The time period in ticks, intervalos en periodos de la cpu. Con esa funcion convierte los milisegundos en periodos de cpu
//                             pdTRUE,         // The timers will auto-reload themselves when they expire.
//                             (void *)timerId,      // Assign each timer a unique id equal to its array index.
//                             vTimerCallback  // Each timer calls the same callback when it expires. Funcion que se ejecuta
//     );
//     if (xTimers == NULL)
//     {
//         // The timer was not created.
//         ESP_LOGE(tag, "The timer was not created");
//     }
//     else
//     {
//         if (xTimerStart(xTimers, 0) != pdPASS)
//         {
//             // The timer could not be set into the Active state.
//             ESP_LOGE(tag, "The timer could not be set into the Active state");
//         }
//     }
//     return ESP_OK;
// }
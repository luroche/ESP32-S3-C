#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/sockets.h"

#define ADC_WIDTH ADC_WIDTH_BIT_12  // Usamos 12 bits de resolución
#define ADC1_CHANNEL_0 ADC1_CHANNEL_0  // Canal 0 del ADC1
#define ADC1_CHANNEL_1 ADC1_CHANNEL_1  // Canal 1 del ADC1
#define ADC1_CHANNEL_2 ADC1_CHANNEL_2  // Canal 2 del ADC1

#define UDP_PORT 12345
#define QUEUE_SIZE 200
#define UDP_IP "192.168.1.100"  // Cambia esta IP por la de destino

// Estructura para almacenar los datos de las lecturas de los ADCs
typedef struct {
    uint16_t adc0_value;
    uint16_t adc1_value;
    uint16_t adc2_value;
} adc_data_t;

// Declaración de la cola
QueueHandle_t adc_data_queue = NULL;
TimerHandle_t adc_timer = NULL;

static const char *TAG = "ADC_UDP";

// Función que será llamada por el temporizador para leer los ADCs
void adc_read_callback(TimerHandle_t xTimer) {
    static adc_data_t data;

    // Leer los valores de los ADCs
    data.adc0_value = adc1_get_raw(ADC1_CHANNEL_0);
    data.adc1_value = adc1_get_raw(ADC1_CHANNEL_1);
    data.adc2_value = adc1_get_raw(ADC1_CHANNEL_2);

    // Añadir los datos a la cola
    if (xQueueSend(adc_data_queue, &data, portMAX_DELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Queue is full, data discarded");
    }
}



// Tarea para enviar los datos de la cola a través de UDP cuando haya 200 datos
void udp_send_task(void *arg) {
    //TODO: Cuando se implemente todo lo demas, metere aqui el udp
    adc_data_t data[QUEUE_SIZE];
    while (1) {
        // Esperar hasta que haya 200 datos en la cola
        if (xQueueReceive(adc_data_queue, data, pdMS_TO_TICKS(1000)) == pdTRUE) { //recibe los datos de la adc_data_queue y los almacena en data. el ultimo parametro es el tiempo que espera a recibir los datos
            // Recoger hasta 200 datos
            int i = 0;
            while (i < QUEUE_SIZE) {
                if (xQueueReceive(adc_data_queue, &data[i], 0) != pdTRUE) { //si se ha recibido un dato anteriormente, pasa a este bucle que es donde se llena data
                    // TODO: PONER ALGUN LOGI
                    break;
                }
                i++;
            }
        }
    }
}




void app_main(void) {
    // Crear la cola
    adc_data_queue = xQueueCreate(QUEUE_SIZE, sizeof(adc_data_t));
    if (adc_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }

    // Configurar los ADCs
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_0);

    // Crear un temporizador para leer ADCs cada 40 microsegundos
    adc_timer = xTimerCreate(   "ADC_Timer",
                                pdMS_TO_TICKS(0.04),
                                pdTRUE, (void *)0,
                                adc_read_callback);
    if (adc_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create timer");
        return;
    }

    // Iniciar el temporizador
    xTimerStart(adc_timer, 0);

    // Crear las tareas
    xTaskCreate(udp_send_task, "UDP Send Task", 2048, NULL, 5, NULL);
}





// y puedo hacer algo para bajar el tiempo de lectura de los ADC?
// ChatGPT said:
// Sí, hay algunas estrategias que podrías emplear para reducir el tiempo necesario para leer los ADCs en el ESP32. Sin embargo, hay un límite en cuanto a lo rápido que puedes hacer estas lecturas debido a las características del hardware y la forma en que el ESP32 maneja la conversión analógica a digital. A continuación te explico varias maneras en que podrías optimizar el proceso de lectura de los ADCs para reducir el tiempo:

// 1. Reducir la resolución de los ADCs
// El ESP32 permite configurar la resolución de los ADCs, y una menor resolución implica que el proceso de conversión será más rápido. La resolución de los ADCs varía entre 9 y 12 bits en el ESP32. Cuanto menor sea la resolución, menor será el tiempo de conversión.

// Resolución de 9 bits: Más rápida, pero menos precisa.
// Resolución de 10 bits: Buena relación entre velocidad y precisión.
// Resolución de 12 bits: Más precisa, pero más lenta.
// Para reducir el tiempo de lectura, puedes configurar la resolución a 9 bits o 10 bits. Esto hará que la conversión ADC sea más rápida.

// Ejemplo de código para reducir la resolución a 9 bits:
// c
// Copy
// // Configurar la resolución de los ADCs a 9 bits
// adc1_config_width(ADC_WIDTH_BIT_9);
// 2. Utilizar los ADCs de forma secuencial
// Si estás leyendo múltiples canales de ADC, el ESP32 realiza las conversiones de manera secuencial (por ejemplo, primero lee un canal, luego el siguiente, y así sucesivamente). Para optimizar el tiempo de lectura, podrías configurar los ADCs de forma que se lean en un orden que maximice el rendimiento.

// Además, puedes evitar la sobrecarga de lectura de múltiples canales configurando los ADCs en la forma correcta. Si lees canales de forma secuencial sin hacer cálculos adicionales entre lecturas, es posible que obtengas una lectura más rápida.

// 3. Utilizar un canal ADC en modo rápido
// Si puedes permitirte sacrificar algunas características (como la precisión), puedes usar un solo canal ADC en modo de conversión rápida. Esto podría reducir significativamente el tiempo necesario para realizar la conversión. Sin embargo, como estás leyendo tres canales de ADC, la reducción del tiempo de lectura en cada canal no afectará demasiado el tiempo total de lectura de todos los canales si no hay una optimización en la forma en que se gestionan las conversiones.

// 4. Leer los ADCs de forma simultánea (utilizando ADC2 en paralelo con ADC1)
// El ESP32 tiene dos grupos de ADCs: ADC1 y ADC2. Los canales de ADC1 y ADC2 se pueden leer en paralelo. Si estás utilizando canales en diferentes grupos, puedes acelerar el proceso al leer varios canales simultáneamente, aunque esto depende de cómo configures tu código.

// Sin embargo, ten en cuenta que las conversiones simultáneas pueden tener sus propios desafíos en términos de ruido y precisión, así que es algo que debes evaluar según tus necesidades.

// 5. Usar ADC_WIDTH para configurar la resolución de la conversión
// Puedes configurar la resolución de la conversión ADC (más baja para tiempos de conversión más rápidos). Además de la resolución (anteriormente mencionada), también puedes configurar otros parámetros para mejorar la velocidad, como la frecuencia de muestreo.

// Ejemplo de configuración para alta velocidad:
// c
// Copy
// // Configurar el ADC para que utilice una frecuencia de conversión más rápida
// adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
// adc1_config_width(ADC_WIDTH_BIT_9); // Reducir la resolución a 9 bits
// 6. Uso de DMA (Acceso Directo a Memoria)
// El ESP32 también soporta DMA (Direct Memory Access) para transferir datos de los ADCs a la memoria de manera eficiente. Utilizar DMA puede liberar el microcontrolador de la tarea de mover los datos desde los ADCs a la memoria, lo que permite realizar lecturas más rápidas. El uso de DMA puede ser más complicado, pero puede ayudar a reducir el tiempo de lectura.

// 7. Configurar la frecuencia de muestreo del ADC
// En algunos casos, los ADCs tienen configuraciones para establecer la frecuencia de muestreo. Si tu aplicación no requiere de alta precisión o si puedes tolerar una menor tasa de muestreo, reducir la frecuencia de muestreo puede disminuir el tiempo de conversión.

// 8. Uso de interrupciones para la lectura
// En lugar de usar un temporizador para activar la lectura, podrías configurar un interruptor de ADC. Esto puede hacer que las lecturas de ADC se realicen de manera más eficiente sin tener que estar continuamente verificando el temporizador o bloqueando el sistema.

// Resumen de pasos para reducir el tiempo de lectura de los ADC:
// Reducir la resolución del ADC: Configura el ADC a 9 bits o 10 bits en lugar de 12 bits.
// Leer en secuencia rápida: Configura los ADCs para que se lean rápidamente uno tras otro sin realizar cálculos intermedios.
// Configurar la frecuencia de muestreo: Si es posible, reduce la frecuencia de muestreo para acelerar la conversión.
// Leer canales en paralelo: Si estás usando múltiples canales de ADC, intenta configurar los canales de los diferentes grupos (ADC1 y ADC2) para que se lean simultáneamente.
// Usar DMA: Si es necesario, configura DMA para transferir datos de los ADCs a la memoria sin que el microcontrolador intervenga en el proceso.
// ¿Cuánto podrías reducir el tiempo de lectura?
// El tiempo de lectura de un solo canal de ADC puede ser bastante variable dependiendo de la configuración (resolución, frecuencia de muestreo, etc.), pero al reducir la resolución y optimizar el código, puedes reducir significativamente el tiempo de lectura de cada canal, permitiendo que las lecturas se realicen más rápido y puedas ajustar mejor el intervalo para realizar las lecturas de ADC.

// Sin embargo, no vas a conseguir tiempos de lectura en el orden de microsegundos con una precisión alta a menos que reduzcas la resolución significativamente. Por ejemplo, podrías configurar cada ADC para que lea a una resolución de 9 bits y hacerlo de manera secuencial con un intervalo adecuado, lo que te permitirá obtener un buen balance entre velocidad y precisión.



// ###############################################################################################################################
// ###############################################################################################################################
// ###############################################################################################################################
// ###############################################################################################################################


// e intentar hacerlo con un adc externo como el ADS8688? aunque tendría que mandar el udp por wifi en vez por ethernet??
// ChatGPT said:
// ¡Sí, definitivamente es una excelente opción! Utilizar un ADC externo como el ADS8688 puede ser una forma muy efectiva de reducir los tiempos de lectura y mejorar el rendimiento general de tu sistema. El ADS8688 es un ADC de 16 bits con una velocidad de conversión bastante rápida, lo que podría ayudarte a obtener las lecturas de forma mucho más eficiente que usando los ADCs internos del ESP32.

// Ventajas de usar el ADS8688:
// Alta velocidad de conversión: El ADS8688 es un ADC de 16 bits, pero puede tener una tasa de conversión rápida, hasta 1 MSPS (un millón de muestras por segundo), dependiendo de la configuración. Esto es mucho más rápido que los ADCs internos del ESP32, especialmente si estás buscando leer datos en menos tiempo.

// Precisión: Al ser un ADC de 16 bits, el ADS8688 te proporciona una alta precisión, lo cual puede ser útil si necesitas mediciones más exactas.

// No sobrecarga del microcontrolador: Al utilizar un ADC externo, el ESP32 no tiene que hacer todo el trabajo de convertir las señales analógicas a digitales, lo que deja más recursos libres para otras tareas, como el procesamiento de datos y el envío de información a través de la red.

// Menor interferencia y ruido: A menudo, los ADCs internos de los microcontroladores pueden ser más ruidosos debido a la interferencia de la propia circuitería interna. Un ADC externo como el ADS8688 puede tener una mayor inmunidad al ruido, lo que mejora la calidad de las lecturas.

// Cómo integrar el ADS8688 con el ESP32:
// 1. Comunicación entre el ESP32 y el ADS8688:
// El ADS8688 utiliza SPI (Serial Peripheral Interface) para la comunicación, y el ESP32 tiene soporte nativo para SPI, lo que facilita la integración del ADC con el ESP32.

// Puedes conectar el ADS8688 al ESP32 a través del bus SPI, que permite transferir los datos de manera rápida. Los pines que necesitarás son los siguientes:

// SCK (Serial Clock): El pin de reloj de SPI.
// MISO (Master In Slave Out): El pin por el que el ADS8688 enviará los datos digitales al ESP32.
// CS (Chip Select): Para habilitar la comunicación con el ADS8688.
// 2. Configuración de la comunicación SPI:
// El primer paso sería configurar el bus SPI en el ESP32 para que pueda comunicarse con el ADS8688. Aquí hay un ejemplo de configuración básica de SPI:

// c
// Copy
// #include "driver/spi_master.h"

// #define SPI_CLK_SPEED 1000000 // Velocidad del reloj SPI (ajústalo según lo que necesitas)
// #define SPI_HOST    SPI2_HOST  // Usa SPI2 si el SPI1 está ocupado

// spi_device_handle_t spi_handle;

// // Configuración del bus SPI
// esp_err_t setup_spi()
// {
//     esp_err_t ret;
//     spi_bus_config_t buscfg = {
//         .miso_io_num = MISO_PIN,   // Pin MISO del ESP32
//         .mosi_io_num = MOSI_PIN,   // Pin MOSI del ESP32 (aunque no lo usaremos para lectura)
//         .sclk_io_num = SCK_PIN,    // Pin SCK del ESP32
//         .quadwp_io_num = -1,
//         .quadhd_io_num = -1,
//     };

//     // Inicializar el bus SPI
//     ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
//     if (ret != ESP_OK) {
//         printf("Error en la inicialización del bus SPI\n");
//         return ret;
//     }

//     // Configurar el dispositivo SPI (el ADS8688)
//     spi_device_interface_config_t devcfg = {
//         .clock_speed_hz = SPI_CLK_SPEED,
//         .mode = 0,  // Modo SPI 0
//         .spics_io_num = CS_PIN,  // Pin CS para el ADS8688
//         .queue_size = 7,   // Tamaño de la cola de SPI
//     };

//     ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi_handle);
//     if (ret != ESP_OK) {
//         printf("Error en la configuración del dispositivo SPI\n");
//         return ret;
//     }

//     return ESP_OK;
// }
// Este código configura el bus SPI y le asigna los pines adecuados para la comunicación con el ADS8688.

// 3. Leer datos del ADS8688:
// Una vez que hayas configurado el bus SPI, puedes leer los datos del ADS8688 utilizando la interfaz SPI. El ADS8688 convertirá los valores analógicos en datos digitales y los enviará al ESP32 a través de SPI.

// Aquí tienes un ejemplo básico de cómo podrías leer un valor del ADC:

// c
// Copy
// uint8_t rx_data[2]; // Para almacenar los 16 bits leídos
// esp_err_t read_adc_data()
// {
//     esp_err_t ret;
//     spi_transaction_t t;
//     memset(&t, 0, sizeof(t));
//     t.length = 16; // 16 bits
//     t.rx_buffer = rx_data;  // Datos leídos se almacenarán en rx_data
//     ret = spi_device_transmit(spi_handle, &t);
//     if (ret != ESP_OK) {
//         printf("Error en la lectura del ADC\n");
//         return ret;
//     }

//     // Procesa los datos (rx_data contiene los valores leídos)
//     uint16_t adc_value = (rx_data[0] << 8) | rx_data[1];  // Concatenamos los 2 bytes en un valor de 16 bits
//     printf("Valor del ADC: %d\n", adc_value);
//     return ESP_OK;
// }
// Este código realiza una transacción SPI para leer 16 bits de datos del ADC.

// 4. Enviar datos a través de UDP:
// Después de haber leído los datos del ADC, puedes enviar los datos a través de UDP. El ESP32 tiene soporte para la pila TCP/IP (Wi-Fi), lo que te permite enviar datos UDP de manera fácil. El proceso de enviar datos a través de UDP no cambia mucho, ya que solo necesitas configurar el socket UDP y enviar los datos leídos:

// c
// Copy
// #include "lwip/sockets.h"

// // Configuración de UDP
// void send_data_via_udp(uint16_t *data, size_t len)
// {
//     struct sockaddr_in dest_addr;
//     int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//     if (sock < 0) {
//         printf("Error al crear el socket UDP\n");
//         return;
//     }

//     dest_addr.sin_family = AF_INET;
//     dest_addr.sin_port = htons(12345); // Puerto de destino
//     dest_addr.sin_addr.s_addr = inet_addr("192.168.1.100"); // Dirección IP de destino

//     // Enviar datos
//     ssize_t sent = sendto(sock, data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//     if (sent < 0) {
//         printf("Error al enviar los datos\n");
//     }

//     close(sock);
// }
// Este código envía los datos del ADC a través de UDP a una dirección IP y puerto específicos.

// Consideraciones al usar Wi-Fi en lugar de Ethernet:
// Rendimiento: El rendimiento de Wi-Fi puede ser variable dependiendo de la calidad de la señal y la congestión de la red. Asegúrate de que tu ESP32 esté lo más cerca posible del punto de acceso Wi-Fi para minimizar la latencia y la pérdida de paquetes.

// Pérdida de paquetes y fiabilidad: Wi-Fi puede ser menos confiable que Ethernet, por lo que deberías considerar el manejo de errores en la capa de UDP, como reintentos o la verificación de integridad de los datos.

// Consumo de energía: El uso de Wi-Fi puede consumir más energía que Ethernet. Si tu proyecto es sensible al consumo de energía, ten en cuenta esto.

// Conclusión:
// Usar un ADC externo como el ADS8688 puede ser una solución muy eficiente para mejorar la velocidad y la precisión de las lecturas de ADC en tu proyecto. Al utilizar SPI para la comunicación, puedes obtener lecturas más rápidas y precisas, y luego enviar los datos por Wi-Fi a través de UDP. Aunque tendrás que adaptarte al uso de Wi-Fi en lugar de Ethernet, esta solución es perfectamente viable y probablemente mejorará el rendimiento de tu sistema.
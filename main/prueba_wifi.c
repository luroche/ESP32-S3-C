#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#define WIFI_SSID "WLAN_4110"
#define WIFI_PASS "IBVJ7U3RBIDB2OI26Y4K"

#define UDP_SERVER_IP "192.168.2.33" // Reemplaza con la IP del servidor destino
#define UDP_PORT 8888            // Puerto del servidor

static const char *TAG = "UDP_CLIENT";

// Función para manejar eventos de WiFi
// static void wifi_event_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data) {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         ESP_LOGI(TAG, "Conectado a WiFi");
//     }
// }

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

// Función para enviar un vector por UDP
void udp_client_task(void *pvParameters) {
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
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(UDP_SERVER_IP);

    while (1) {
        // Enviar vector al servidor
        ESP_LOGI(TAG, "Enviando datos a: %s:%d", UDP_SERVER_IP, UDP_PORT);
        int sent_bytes = sendto(sock, vector, vector_size, 0,
                                (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0) {
            ESP_LOGE(TAG, "Error enviando datos: %s", strerror(errno));
        } else {
            ESP_LOGI(TAG, "Enviado %d bytes", sent_bytes);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // Esperar 2 segundos
    }

    close(sock);
    vTaskDelete(NULL);
}


// **Main Application**
void app_main() {
    esp_log_level_set("wifi", ESP_LOG_DEBUG);
    printf("Comenzamos señores PARTE 5...");
    // Inicializar NVS para almacenamiento persistente (requerido por WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta(); // Iniciar WiFi

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, 5, NULL); // Crear tarea UDP
}

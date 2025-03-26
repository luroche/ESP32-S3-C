#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#define WIFI_SSID      "WLAN_4110"
#define WIFI_PASS      "IBVJ7U3RBIDB2OI26Y4K"
#define MAX_RETRY      5

static const char *TAG = "wifi_sta";
static int retry_num = 0;

esp_netif_t *esp_netif = NULL;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < MAX_RETRY) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Intentando reconectar a %s (%d)", WIFI_SSID, retry_num);
        } else {
            ESP_LOGI(TAG, "Conexión fallida al Wi-Fi");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Conectado a Wi-Fi, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
    }
}

void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    
    esp_netif = esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .bssid_set = false
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void app_main() {
    esp_log_level_set("*", ESP_LOG_DEBUG);
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
}

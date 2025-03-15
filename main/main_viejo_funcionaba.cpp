#include <SPI.h>
#include <Ethernet.h>

// Definir los pines para el W5500
#define W5500_CS_PIN 5  // Pin de selección de chip (CS) para el W5500
#define W5500_RST_PIN 4 // Pin de reset para el W5500

// Dirección MAC para el dispositivo (debe ser única en tu red)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

void setup() {
  // Iniciar la comunicación serial
  Serial.begin(115200);
  while (!Serial) {
    ; // Esperar a que el puerto serial se conecte
  }

  // Iniciar el pin de reset y ponerlo en alto
  pinMode(W5500_RST_PIN, OUTPUT);
  digitalWrite(W5500_RST_PIN, HIGH);

  // Resetear el W5500
  digitalWrite(W5500_RST_PIN, LOW);
  delay(100);
  digitalWrite(W5500_RST_PIN, HIGH);
  delay(100);

  // Iniciar Ethernet con SPI y el pin CS
  Ethernet.init(W5500_CS_PIN);

  // Iniciar la conexión Ethernet usando DHCP
  Serial.println("Iniciando Ethernet con DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Error al configurar Ethernet usando DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("No se encontró el módulo Ethernet. Revisa las conexiones.");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("El cable Ethernet no está conectado.");
    }
    // No hacer nada más
    while (true) {
      delay(1);
    }
  }

  // Mostrar la dirección IP asignada por DHCP
  Serial.print("Dirección IP asignada: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // Aquí puedes agregar código para manejar la conexión Ethernet
  // Por ejemplo, hacer peticiones HTTP, escuchar en un puerto, etc.
  delay(1000); // Solo para evitar que el loop se ejecute demasiado rápido
}












// #include <stdio.h>
// #include "esp_event.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"
// #include "esp_eth.h"
// #include "driver/spi_master.h"
// #include "esp_eth_mac_spi.h"
// #include "esp_eth_phy.h"

// static const char *TAG = "ETH_W5500";

// // Definición de pines
// #define PIN_MISO 13
// #define PIN_MOSI 11
// #define PIN_SCLK 12
// const int PIN_CS = 10;
// #define SPI_HOST SPI2_HOST  // Asegúrate de que SPI2_HOST sea el adecuado para tu configuración

// // Configuración de IP estática
// // esp_netif_ip_info_t ip_info = {
// //     .ip = { 10, 0, 0, 7 },  // Dirección IP
// //     .gw = { 10, 0, 0, 1 },  // Puerta de enlace
// //     .netmask = { 255, 255, 255, 0 }  // Máscara de subred
// // };
// esp_netif_ip_info_t ip_info = ESP_NETIF_IP_INFO_INIT(ESP_IP4TOADDR(10, 0, 0, 7), ESP_IP4TOADDR(10, 0, 0, 1), ESP_IP4TOADDR(255, 255, 255, 0));

// static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
//     if (event_id == ETHERNET_EVENT_CONNECTED) {
//         ESP_LOGI(TAG, "Ethernet Conectado");
//     } else if (event_id == ETHERNET_EVENT_DISCONNECTED) {
//         ESP_LOGI(TAG, "Ethernet Desconectado");
//     }
// }

// static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
//     ESP_LOGI(TAG, "Dirección IP Asignada: " IPSTR, IP2STR(&ip_info.ip));
// }

// extern "C" void app_main(void) {
//     ESP_LOGI(TAG, "Inicializando...");

//     // Inicializar NVS
//     ESP_ERROR_CHECK(nvs_flash_init());

//     // Inicializar la pila de red
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());

//     esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
//     esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);

//     esp_netif_dhcpc_stop(eth_netif);
//     ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));

//     // Configuración del bus SPI
//     spi_bus_config_t buscfg = {
//         .mosi_io_num = PIN_MOSI,
//         .miso_io_num = PIN_MISO,
//         .sclk_io_num = PIN_SCLK,
//         .max_transfer_sz = 4096
//     };

//     ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

//     // Configuración de la MAC y PHY del W5500
//     eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(SPI_HOST, PIN_CS);  // Usar SPI2_HOST

//     eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
//     eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
//     phy_config.phy_addr = 1;
//     phy_config.reset_gpio_num = -1;

//     esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
//     esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

//     esp_eth_handle_t eth_handle = NULL;
//     esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
//     ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));

//     ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

//     ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
//     ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

//     ESP_ERROR_CHECK(esp_eth_start(eth_handle));

//     ESP_LOGI(TAG, "Configuración Ethernet completada.");
// }

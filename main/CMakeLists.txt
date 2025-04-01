idf_component_register(
    SRCS "prueba_wifi.c"
    # SRCS "timer.c" "prueba_wifi.c" #Archivo fuente
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer lwip esp_eth freertos esp_wifi esp_event nvs_flash )



# idf_component_register(SRCS "main.cpp"
#                         INCLUDE_DIRS "."
#                         REQUIRES esp_eth esp_netif esp_wifi lwip nvs_flash driver)

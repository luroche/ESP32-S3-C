idf_component_register(
    SRCS "main.c" #Archivo fuente
    INCLUDE_DIRS ""
    REQUIRES driver esp_timer lwip freertos esp_eth)



# idf_component_register(SRCS "main.cpp"
#                         INCLUDE_DIRS "."
#                         REQUIRES esp_eth esp_netif esp_wifi lwip nvs_flash driver)

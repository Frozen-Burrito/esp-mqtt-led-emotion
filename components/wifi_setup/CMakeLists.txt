idf_component_register(SRCS "wifi_setup.c"
                       INCLUDE_DIRS "."
                       REQUIRES freertos
                                esp_wifi
                                esp_common
                                esp_event
                                esp_netif)
idf_component_register(SRCS "mqtt_app_client.c"
                       INCLUDE_DIRS "./include"
                       REQUIRES esp-tls
                                json
                                driver
                                esp_common
                                esp_event
                                esp_netif
                                mqtt)
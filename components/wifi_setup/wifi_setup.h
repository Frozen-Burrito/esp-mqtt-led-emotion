#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#ifndef WIFI_SETUP_H_
#define WIFI_SETUP_H_

// WiFi driver configuration.
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PSWD "your-wifi-password"
#define WIFI_MAX_RETRY 3

// Bits for FreeRTOS event group.
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/**
 * @brief Event handler for specific WIFI_EVENTs
 * 
 * @param void*: args
 * @param esp_event_base_t: base type of the event
 * @param int32_t: event id
 * @param void*: event data
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

/**
 * @brief Config and start the WiFi driver
 * 
 * This function creates a FreeRTOS event group with a connected bit
 * and a fail bit. Then, it registers the wifi_event_handler
 * for both WIFI_EVENT and IP_EVENT.
 * 
 * This also uses WIFI_SSID and WIFI_PSWD to configure the driver.
 * 
 * @return ESP_OK: wifi driver initialized
 *         ESP_FAIL: error during initialization
 */
esp_err_t wifi_sta_init(void);

#endif
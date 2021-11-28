#include "wifi_setup.h"

static const char* WIFI_TAG = "WiFi Setup";

// FreeRTOS event group to signal wifi driver status.
static EventGroupHandle_t s_wifi_event_group;

// Number of times the WiFi driver has failed to connect.
static int s_retry_count = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                // The WiFi driver was successfully initialized,
                // try to connect.
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(WIFI_TAG, "WiFi connected");
                break;
            
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_retry_count < WIFI_MAX_RETRY)
                {
                    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                    esp_wifi_connect();
                    s_retry_count++;
                    ESP_LOGI(WIFI_TAG, "Retrying connection to AP...");
                }
                else 
                {   
                    // If the max amount of retries was reached,
                    // connection failed completely.
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }

                ESP_LOGI(WIFI_TAG, "Connection to AP failed");
                break;

            default:
                ESP_LOGW(WIFI_TAG, "Unhandled WIFI_EVENT: %d", event_id);
                break;
        }
    }    
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_sta_init(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PSWD,

	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "WiFi driver initialized");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    esp_err_t wifi_status = ESP_OK;

    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PSWD);

        wifi_status = ESP_OK;
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PSWD);

        wifi_status = ESP_FAIL;
    } 
    else 
    {
        ESP_LOGE(WIFI_TAG, "Unexpected event");
        wifi_status = ESP_FAIL;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);

    return wifi_status;
}
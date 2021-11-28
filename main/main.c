#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"

#include "wifi_setup.h"
#include "mqtt_app_client.h"

// MQTT config. Usar broker en mi red local.
// #define BROKER_IP "mqtt://mqtt.eclipseprojects.io"
#define BROKER_IP "your-broker-url"

// LEDc config
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_R_GPIO     (21)
#define LEDC_R_CHANNEL  LEDC_CHANNEL_0
#define LEDC_G_GPIO     (22)
#define LEDC_G_CHANNEL  LEDC_CHANNEL_1
#define LEDC_B_GPIO     (23)
#define LEDC_B_CHANNEL  LEDC_CHANNEL_2

#define LEDC_RGB_CH_NUM (3)

#define LEDC_DUTY_RES   LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY  (5000) // Frequency in Hertz. Set frequency at 5 kHz
#define INIT_DUTY       (4095)           

void rgb_ledc_init(void)
{
    ledc_timer_config_t ledc_timer = 
    {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_r_ch_cfg = 
    {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_R_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_R_GPIO,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config_t ledc_g_ch_cfg = 
    {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_G_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_G_GPIO,
        .duty = INIT_DUTY,
        .hpoint = 0
    };

    ledc_channel_config_t ledc_b_ch_cfg = 
    {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_B_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_B_GPIO,
        .duty = 0,
        .hpoint = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_r_ch_cfg));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_g_ch_cfg));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_b_ch_cfg));
}

void app_main(void)
{
    esp_err_t nvs_status = nvs_flash_init();

    if (nvs_status == ESP_ERR_NVS_NO_FREE_PAGES || nvs_status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_status = nvs_flash_init();
    }

    ESP_ERROR_CHECK(nvs_status);

    esp_err_t wifi_start_status = wifi_sta_init();

    ESP_ERROR_CHECK(wifi_start_status);
    
    mqtt_client_init(BROKER_IP);

    rgb_ledc_init();
}

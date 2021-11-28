#include <cJSON.h>
#include <math.h>

#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "mqtt_client.h"

#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#define TOTAL_EMOTIONS 5

static const char* MQTT_TAG = "MQTT_CLIENT";

struct rgb_color 
{
    int red;
    int green;
    int blue;
};

enum emotion
{
    NEUTRAL = -1,
    HAPPY = 0,
    ANGRY,
    SURPRISE,
    SAD, 
    FEAR
};

typedef struct rgb_color rgb_color_t;

typedef enum emotion emotion_t;

void set_led_color(int red, int green, int blue);

rgb_color_t color_from_emotion(emotion_t user_emotion);

emotion_t get_emotion_from_mqtt(const char* message, size_t msg_len);

int ledc_duty_cycle_from_hex(int hex_color);

void mqtt_event_handler(
    void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data
);

int mqtt_publish_msg(const char* topic, const char* message, int qos);

void mqtt_client_init(const char* broker_uri);

#endif
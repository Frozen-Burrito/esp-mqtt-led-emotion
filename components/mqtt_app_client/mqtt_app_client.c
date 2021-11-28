#include "mqtt_app_client.h"

static esp_mqtt_client_handle_t client = NULL;

static rgb_color_t current_color = { 0, 0, 0 };

static rgb_color_t colors[6] = 
{
    { .red = 0, .green = 0, .blue = 0},
    { .red = 255, .green = 255, .blue = 30}, // Feliz
    { .red = 255, .green = 0, .blue = 0}, // Enojado
    { .red = 79, .green = 229, .blue = 86}, // Sorprendido
    { .red = 75, .green = 192, .blue = 255}, // Triste
    { .red = 190, .green = 100, .blue = 235}, // Miedo
};

void set_led_color(int red, int green, int blue)
{
    current_color.red = red;
    current_color.green = green;
    current_color.blue = blue;
}

void log_err_if_nonzero(const char* message, int err_code)
{
    if (err_code != 0)
    {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, err_code);
    }
}

void mqtt_event_handler(
    void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data
)
{
    ESP_LOGD(MQTT_TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);

    esp_mqtt_event_handle_t event = event_data;
    client = event->client;

    int msg_id;

    char msg_data_buffer[128];

    switch ((esp_mqtt_event_id_t) event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            // ESP_LOGI(MQTT_TAG, "Sent publish successful, msg_id=%d", event->msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            // ESP_LOGI(MQTT_TAG, "Sent subscribe successful, msg_id=%d", event->msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "emotion", 0);
            ESP_LOGI(MQTT_TAG, "Sent subscribe successful, msg_id=%d", event->msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // ESP_LOGI(MQTT_TAG, "Sent unsubscribe successful, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            client = NULL;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(MQTT_TAG, "Sent publish successful, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            //TODO: Limpiar esta cosa.
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");

            strncpy(msg_data_buffer, event->data, event->data_len);

            emotion_t user_emotion = get_emotion_from_mqtt(msg_data_buffer, event->data_len);
            rgb_color_t led_color = color_from_emotion(user_emotion);
            printf("The color is: %d, %d, %d\n", led_color.red, led_color.green, led_color.blue);

            int red_duty = ledc_duty_cycle_from_hex(led_color.red);
            int green_duty = ledc_duty_cycle_from_hex(led_color.green);
            int blue_duty = ledc_duty_cycle_from_hex(led_color.blue);

            printf("New duty cycles: %d, %d, %d\n", red_duty, green_duty, blue_duty);

            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, red_duty));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, green_duty));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, blue_duty));

            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));

            printf("Topic = %.*s\r\n", event->topic_len, event->topic);
            printf("Data = %.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
            {
                log_err_if_nonzero("from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_err_if_nonzero("from tls stack", event->error_handle->esp_tls_stack_err);
                log_err_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        
        default:
            ESP_LOGI(MQTT_TAG, "Other event id:%d", event_id);
            break;
    }

}

int mqtt_publish_msg(const char* topic, const char* message, int qos)
{
    if (client == NULL) 
    {
        ESP_LOGE(MQTT_TAG, "Client is not assigned.");
        return -1;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, message, 0, qos, 0);

    ESP_LOGI(MQTT_TAG, "Published to topic '%d', message: %s", msg_id, message);

    return msg_id;
} 

emotion_t get_emotion_from_mqtt(const char* message, size_t msg_len)
{
    ESP_LOGI(MQTT_TAG, "Message for emotion analysis: %s", message);

    int main_emotion_index = -1;
    double max_emotion_value = 0.0;
    char emotionLabels[TOTAL_EMOTIONS][10] = 
    {
        "happy",
        "angry",
        "surprise",
        "sad",
        "fear"
    }; 

    cJSON* root = cJSON_Parse(message);

    for (int i = 0; i < TOTAL_EMOTIONS; ++i)
    {
        double value = cJSON_GetObjectItem(root, emotionLabels[i])->valuedouble;
        ESP_LOGI(MQTT_TAG, "%s level: %f", emotionLabels[i], value);

        if (value > max_emotion_value)
        {
            max_emotion_value = value;
            main_emotion_index = i;
        }
    }

    switch (main_emotion_index)
    {
        case 0:
            ESP_LOGI(MQTT_TAG, "The user is happy");
            return HAPPY;
        case 1:
            ESP_LOGI(MQTT_TAG, "The user is angry");
            return ANGRY;
        case 2:
            ESP_LOGI(MQTT_TAG, "The user is surprised");
            return SURPRISE;
        case 3:
            ESP_LOGI(MQTT_TAG, "The user is sad");
            return SAD;
        case 4:
            ESP_LOGI(MQTT_TAG, "The user is fearful");
            return FEAR;
        default:
            ESP_LOGI(MQTT_TAG, "The user is neutral");
            return NEUTRAL;
    }
}

rgb_color_t color_from_emotion(emotion_t user_emotion)
{
    return colors[(int)user_emotion + 1];

    // switch(user_emotion)
    // {
    //     case HAPPY:
    //         return colors[0];
    //     default:
    //         return colors[5];
    // }
}

// Para obtener el duty cycle, se usa:
// n% = 100% * color / 255, donde color es un numero de 0 a 255.
// (2 ^ 13) - 1 * n%, donde 13 es la resolucion para la frecuencia.
int ledc_duty_cycle_from_hex(int hex_color)
{
    double percent = hex_color / 255.0;
    int duty_cycle = (int) ((pow(2, 13) - 1) * percent);
    return duty_cycle;
}

void mqtt_client_init(const char* broker_uri)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker_uri,
        .port = 1883,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);
}
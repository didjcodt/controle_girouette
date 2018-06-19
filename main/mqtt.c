#include "mqtt.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "mqtt_client.h"

// MQTT
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
   esp_mqtt_client_handle_t client = event->client;
   int msg_id;
   // your_context_t *context = event->context;
   switch (event->event_id) {
      case MQTT_EVENT_CONNECTED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_CONNECTED");

         msg_id = esp_mqtt_client_subscribe(client, "/topic/binouze_rezel/led_0", 0);
         ESP_LOGI("MQTT_CLIENT", "sent subscribe successful, msg_id=%d", msg_id);
         break;

      case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_DISCONNECTED");
         break;

      case MQTT_EVENT_SUBSCRIBED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
         break;

      case MQTT_EVENT_UNSUBSCRIBED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
         break;

      case MQTT_EVENT_PUBLISHED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
         break;

      case MQTT_EVENT_DATA:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_DATA");
         printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
         printf("DATA=%.*s\r\n", event->data_len, event->data);
         break;

      case MQTT_EVENT_ERROR:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_ERROR");
         break;
   }
   return ESP_OK;
}

void mqtt_init(void) {
   const esp_mqtt_client_config_t mqtt_cfg = {
      .uri = CONFIG_MQTT_URL,
      .event_handle = mqtt_event_handler,
      // .user_context = (void *)your_context
   };

   esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
   esp_mqtt_client_start(client);
}


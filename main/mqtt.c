#include "mqtt.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "mqtt_client.h"

char mqtt_string[MQTT_PAYLOAD_MAX_SIZE] = {0};
int new_string = 0;

// MQTT event group
EventGroupHandle_t mqtt_event_group;

// MQTT event handler
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event) {
   esp_mqtt_client_handle_t client = event->client;
   int msg_id;
   switch (event->event_id) {
      case MQTT_EVENT_CONNECTED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_CONNECTED");

         msg_id = esp_mqtt_client_subscribe(client, CONFIG_MQTT_TOPIC_DISPLAY, 0);
         ESP_LOGI("MQTT_CLIENT", "Subscribed to display topic (%s), msg_id=%d",
               CONFIG_MQTT_TOPIC_DISPLAY, msg_id);

         msg_id = esp_mqtt_client_subscribe(client, CONFIG_MQTT_TOPIC_OTA, 0);
         ESP_LOGI("MQTT_CLIENT", "Subscribed to ota topic (%s), msg_id=%d",
               CONFIG_MQTT_TOPIC_OTA, msg_id);

         xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);

         break;

      case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI("MQTT_CLIENT", "MQTT_EVENT_DISCONNECTED");
         xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
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

         // Sanity check
         if (event->data_len >= MQTT_PAYLOAD_MAX_SIZE) {
            ESP_LOGI("MQTT_CLIENT", "Payload is larger than buffer!");
            break;
         }

         printf("DATA=%.*s\r\n", event->data_len, event->data);
         memcpy(mqtt_string, event->data, event->data_len*sizeof(char));
         memset(&mqtt_string[event->data_len], 0, MQTT_PAYLOAD_MAX_SIZE-event->data_len);
         mqtt_string[event->data_len] = '\0';

         if (memcmp(event->topic, CONFIG_MQTT_TOPIC_OTA, event->topic_len) == 0) {
            ESP_LOGI("MQTT_CLIENT", "OTA update!");
            xEventGroupSetBits(mqtt_event_group, MQTT_OTA_BIT);
         } else if (memcmp(event->topic, CONFIG_MQTT_TOPIC_DISPLAY, event->topic_len) == 0) {
            ESP_LOGI("MQTT_CLIENT", "New string to display");
            new_string = 1;
         } else {
            ESP_LOGE("MQTT_CLIENT", "Error, topic can be \"%s\" or \"%s\", but got \"%s\"",
                  CONFIG_MQTT_TOPIC_OTA, CONFIG_MQTT_TOPIC_DISPLAY,
                  event->topic);
         }

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
      .client_id = CONFIG_MQTT_CLIENT_ID,
      .event_handle = mqtt_event_handler,
      .lwt_topic = CONFIG_MQTT_TOPIC_LAST_WILL,
      .lwt_msg = CONFIG_MQTT_LAST_WILL_MESSAGE,
      .lwt_msg_len = strlen(CONFIG_MQTT_LAST_WILL_MESSAGE),
   };

   mqtt_event_group = xEventGroupCreate();

   esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
   esp_mqtt_client_start(client);
}


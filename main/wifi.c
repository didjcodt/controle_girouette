#include "wifi.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// ESP specific includes
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

// WiFi
EventGroupHandle_t wifi_event_group;

// Permanently try to connect to WiFi when connection is lost
// Never surrender!
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
   switch (event->event_id) {
      case SYSTEM_EVENT_STA_START:
         esp_wifi_connect();
         break;
      case SYSTEM_EVENT_STA_GOT_IP:
         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
         break;
      case SYSTEM_EVENT_STA_DISCONNECTED:
         esp_wifi_connect();
         xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
         break;
      default:
         break;
   }
   return ESP_OK;
}

void wifi_init(void) {
   tcpip_adapter_init();
   wifi_event_group = xEventGroupCreate();
   ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));
   ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
   wifi_config_t wifi_config = {
      .sta = {
         .ssid = CONFIG_WIFI_SSID,
         .password = CONFIG_WIFI_PASSWORD,
      },
   };
   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
   ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
   ESP_LOGI("WIFI", "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
   ESP_ERROR_CHECK(esp_wifi_start());
}

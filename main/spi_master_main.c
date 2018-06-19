// stdlib
#include <stdlib.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"

// Driver includes
#include "driver/spi_master.h"
#include "driver/gpio.h"

// ESP specific includes
#include "esp_system.h"
#include "esp_log.h"
#include "soc/gpio_struct.h"
#include "nvs_flash.h"

// Other
#include "mqtt.h"
#include "wifi.h"
#include "display.h"

// GPIO Definition
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

static const char *TAG = "MAIN_APP";

void app_main() {
   // Initialize logging
   ESP_LOGI(TAG, "[APP] System initialization\n");
   ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
   ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
   esp_log_level_set("*", ESP_LOG_INFO);
   esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
   esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
   esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
   esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
   esp_log_level_set("WIFI", ESP_LOG_VERBOSE);
   esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);

   nvs_flash_init();
   wifi_init();
   mqtt_init();
   display_init(PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
}

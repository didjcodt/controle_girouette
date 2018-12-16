#pragma once

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void mqtt_init(void);

#define MQTT_PAYLOAD_MAX_SIZE 256
extern char mqtt_string[MQTT_PAYLOAD_MAX_SIZE];
extern int new_string;

extern EventGroupHandle_t mqtt_event_group;
#define MQTT_CONNECTED_BIT BIT0
#define MQTT_OTA_BIT BIT1

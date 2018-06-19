#pragma once

void mqtt_init(void);

#define MQTT_PAYLOAD_MAX_SIZE 256
extern char mqtt_string[MQTT_PAYLOAD_MAX_SIZE];

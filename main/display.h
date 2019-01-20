#pragma once

typedef struct {
    int clock_speed_hz;
    int mosi;
    int clk;
    int cs;
} display_spi_device_t;

void display_init(display_spi_device_t *device);

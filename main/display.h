#pragma once

typedef struct {
    int mosi;
    int clk;
    int cs;
} display_spi_device_t;

void display_init(display_spi_device_t *device);

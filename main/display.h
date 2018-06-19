#pragma once

#include "driver/spi_master.h"

spi_device_handle_t display_init(int mosi, int clk, int cs);

void animate(spi_device_handle_t spi);

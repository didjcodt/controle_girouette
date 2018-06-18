#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// There are 5 8-bit shift registers per panel, 2 panels
#define BUFFER_SIZE (5*8)

// Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
DRAM_ATTR static uint8_t buffer_0[BUFFER_SIZE];
DRAM_ATTR static uint8_t buffer_1[BUFFER_SIZE];

// Display buffer. The write buffer is the other (0->1, 1->0)
static int display_buffer_idx = 0;
static uint8_t* display_buffer = buffer_0;

// Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
// function is finished because the SPI driver needs access to it even while we're already calculating the next line.
static spi_transaction_t trans_buf[2] = {
   {
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = BUFFER_SIZE,
      .rxlength = 0,
      .tx_buffer = &buffer_0,
      .rx_buffer = NULL,
   },
   { 
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = BUFFER_SIZE,
      .rxlength = 0,
      .tx_buffer = &buffer_1,
      .rx_buffer = NULL,
   }
};

static spi_transaction_t *last_buf_desc = &trans_buf[1];

static void pre_g(spi_transaction_t *t) {
   gpio_set_level(PIN_NUM_CS, 1);
}

static void post_g(spi_transaction_t *t) {
   gpio_set_level(PIN_NUM_CS, 0);
}

// Simple routine to generate some patterns and send them to the LED Panel.
static void animate(spi_device_handle_t spi) {
    int frame=0;
    esp_err_t ret;

    while(1) {
        // No spam :)
        if(frame % (2 << 12) == 0)
           printf("Frame %d animating!\n", frame);

        // TODO Calculate the animation
        for(int idx = 0; idx < BUFFER_SIZE; idx++) {
           display_buffer[idx] = (frame+idx)%128;
        }

        // Wait for last transmission to be successful before swapping buffers
        if(frame != 0) {
           ret=spi_device_get_trans_result(spi, &last_buf_desc, portMAX_DELAY);
           assert(ret==ESP_OK);
        }

        // Swap buffers
        last_buf_desc = &trans_buf[display_buffer_idx];
        display_buffer_idx = 1 - display_buffer_idx;
        display_buffer = display_buffer_idx == 0 ? buffer_0 : buffer_1;

        // Send the data to the SPI device
        ret=spi_device_queue_trans(spi, &trans_buf[display_buffer_idx], portMAX_DELAY);
        assert(ret==ESP_OK);
        frame++;
    }
}

void app_main() {
    esp_err_t ret;

    printf("System initialization!\n");
    // SPI device configuration structs
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=BUFFER_SIZE
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=100*1000,           //Clock out at 100 kHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=-1,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .command_bits=0, // Do not use command/address, just send raw data to Shift Registers
        .address_bits=0,
        .dummy_bits=0,
        .pre_cb=pre_g,
        .post_cb=post_g
    };

    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);

    // Initialize the SPI bus with configuration
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    ESP_ERROR_CHECK(ret);
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    printf("SPI Bus initialized!\n");

    // Do wolffy stuff
    animate(spi);
}

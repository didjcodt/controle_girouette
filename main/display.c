#include "display.h"

// Driver includes
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "soc/gpio_struct.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt.h"
#include "font.h"

// There are 5 8-bit shift registers per panel, 2 panels
#define NB_OF_PANELS 2
#define BUFFER_SIZE_PER_PANEL 5
#define BUFFER_SIZE (BUFFER_SIZE_PER_PANEL*NB_OF_PANELS)

// Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
DRAM_ATTR static uint8_t buffer_0[BUFFER_SIZE];
DRAM_ATTR static uint8_t buffer_1[BUFFER_SIZE];

// Display buffer. The write buffer is the other (0->1, 1->0)
static int display_buffer_idx = 0;
static uint8_t* display_buffer = buffer_0;
static uint8_t* write_buffer = buffer_1;

// Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
// function is finished because the SPI driver needs access to it even while we're already calculating the next line.
static spi_transaction_t trans_buf[2] = {
   {
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = 8*BUFFER_SIZE,
      .rxlength = 0,
      .tx_buffer = &buffer_0,
      .rx_buffer = NULL,
   },
   { 
      .flags = 0,
      .cmd = 0,
      .addr = 0,
      .length = 8*BUFFER_SIZE,
      .rxlength = 0,
      .tx_buffer = &buffer_1,
      .rx_buffer = NULL,
   }
};

static spi_transaction_t *last_buf_desc = &trans_buf[0];
static spi_device_handle_t spi;

// Simple routine to generate some patterns and send them to the LED Panel.
#define STACK_SIZE 4096
StaticTask_t animate_task_buffer;
StackType_t animate_task_stack[STACK_SIZE];
static void animate_task(void* pvParameter) {
   int frame = 0;
   int update_counter = 0;
   esp_err_t ret;

   while(1) {
      // No spam :)
      if(frame % (1 << 9) == 0) {
         printf("frame %d scanning!\n", frame);
         printf("%d, %d\n", (int)mqtt_string[0], (int)'a');
         update_counter++;
      }

      for (int line = 0; line < 7; line++) {
         // Animate :)
         int scanline_offset = 0;
         for(int idx = 0; idx < BUFFER_SIZE; idx++) {
            // Scan line
            if(idx%BUFFER_SIZE_PER_PANEL == 4) {
               write_buffer[idx] = 1<<line;
               scanline_offset++;
            } else {
               // Write the character
               int char_in_string = (int)mqtt_string[idx-scanline_offset];
               write_buffer[idx] = cp437_horizontal_font[char_in_string][1+line];
            }
         }

         // Wait for last transmission to be successful before swapping buffers
         if(line != 0) {
            ret=spi_device_get_trans_result(spi, &last_buf_desc, portMAX_DELAY);
            assert(ret==ESP_OK);
         }

         // Swap buffers
         display_buffer_idx = 1 - display_buffer_idx;
         display_buffer = display_buffer_idx == 0 ? buffer_0 : buffer_1;
         write_buffer = display_buffer_idx == 0 ? buffer_1 : buffer_0;
         last_buf_desc = &trans_buf[1 - display_buffer_idx];

         // Send the data to the SPI device
         ret=spi_device_queue_trans(spi, &trans_buf[display_buffer_idx], portMAX_DELAY);
         assert(ret==ESP_OK);
      }

      // New frame
      frame++;
   }
}

void display_init(int mosi, int clk, int cs) {
   // SPI device configuration structs
   spi_bus_config_t buscfg={
      .miso_io_num=-1,
      .mosi_io_num=mosi,
      .sclk_io_num=clk,
      .quadwp_io_num=-1,
      .quadhd_io_num=-1,
      .max_transfer_sz=BUFFER_SIZE
   };
   spi_device_interface_config_t devcfg={
      .clock_speed_hz=2*1000*1000, // Clock out at 1 MHz
      .mode=0,                     // SPI mode 0
      .spics_io_num=cs,    // CS pin
      .queue_size=10,              // We want to be able to queue 10 transactions at a time
      .command_bits=0,             // Do not use command/address, just send raw data to Shift Registers
      .address_bits=0,
      .dummy_bits=0
   };

   // Initialize the SPI bus with configuration
   esp_err_t ret;

   ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
   ESP_ERROR_CHECK(ret);
   ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
   ESP_ERROR_CHECK(ret);
   printf("SPI Bus initialized!\n");

   xTaskCreateStatic(&animate_task, "animate_task", STACK_SIZE, NULL, tskIDLE_PRIORITY+1, animate_task_stack, &animate_task_buffer);
}

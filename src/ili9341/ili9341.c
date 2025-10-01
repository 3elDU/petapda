#include "ili9341.h"

#include "FreeRTOS.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdio.h>

uint16_t ili9341_color565(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t color = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
  return color >> 8 | color << 8; // RP2040 is little-endian, while the display is big-endian
}

void ili9341_send_command_no_args(ili9341_command cmd)
{
  gpio_put(ILI_PIN_DC, 0);
  spi_write_blocking(ILI_SPI_INST, &cmd, 1);
  gpio_put(ILI_PIN_DC, 1);
}

void ili9341_send_command(ili9341_command cmd, uint8_t args_count, uint8_t args[])
{
  gpio_put(ILI_PIN_DC, 0);
  spi_write_blocking(ILI_SPI_INST, &cmd, 1);
  gpio_put(ILI_PIN_DC, 1);
  spi_write_blocking(ILI_SPI_INST, args, args_count);
}

void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  ili9341_send_command(IL_CMD_COLUMN_ADDRESS_SET, 4, (uint8_t[]){x0 >> 8, x0 & 0xff, x1 >> 8, x1 & 0xff});
  ili9341_send_command(IL_CMD_PAGE_ADDRESS_SET, 4, (uint8_t[]){y0 >> 8, y0 & 0xff, y1 >> 8, y1 & 0xff});
}

void ili9341_init(void)
{
  // Display SPI
  gpio_set_function(ILI_SPI_CLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ILI_SPI_CS_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ILI_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ILI_SPI_RX_PIN, GPIO_FUNC_SPI);

  uint baud_rate = spi_init(spi1, 64 * 1000 * 1000);
  printf("ili9341_init: baud rate %u\n", baud_rate);

  gpio_init(ILI_PIN_RESET);
  gpio_init(ILI_PIN_DC);
  gpio_init(ILI_PIN_LED);
  gpio_set_dir(ILI_PIN_RESET, GPIO_OUT);
  gpio_set_dir(ILI_PIN_DC, GPIO_OUT);
  gpio_set_dir(ILI_PIN_LED, GPIO_OUT);

  gpio_put(ILI_PIN_RESET, 0);
  sleep_ms(1);
  gpio_put(ILI_PIN_RESET, 1);

  sleep_ms(5);

  ili9341_send_command_no_args(IL_CMD_SOFTWARE_RESET);

  // Wait 120ms before sending sleep out
  sleep_ms(120);

  ili9341_send_command(IL_CMD_PIXEL_FORMAT_SET, 1, (uint8_t[]){0x55});
  ili9341_send_command(IL_CMD_MEMORY_ACCESS_CONTROL, 1, (uint8_t[]){ILI_ORIENTATION_PORTRAIT_FLIPPED});
  ili9341_send_command(IL_CMD_NORMAL_MODE_FRAME_RATE, 2, (uint8_t[]){0b00, 0b11111}); // Set screen refresh rate to 61 FPS

  // Wait 5ms after sleep out before sending commands
  sleep_ms(5);

  ili9341_send_command_no_args(IL_CMD_SLEEP_MODE_OFF);
  ili9341_send_command_no_args(IL_CMD_DISPLAY_ON);
  gpio_put(ILI_PIN_LED, 1);
}

void ili9341_send_data(void *data, uint32_t length)
{
  spi_write_blocking(ILI_SPI_INST, (uint8_t *)data, length);
}
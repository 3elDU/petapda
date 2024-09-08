#include "7segment.h"

#include <stdio.h>

#include "hardware/gpio.h"
#include "pico/time.h"

void sseg_init()
{
  spi_init(SSEG_SPI_INST, 8 * 1000 * 1000); // Run at 8 MHz

  gpio_set_function(SSEG_SPI_SRCLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SSEG_SPI_TX_PIN, GPIO_FUNC_SPI);
  gpio_init(SSEG_SPI_RCLK_PIN);
  gpio_set_dir(SSEG_SPI_RCLK_PIN, GPIO_OUT);
}

void sseg_write_raw(uint32_t value)
{
  value = value << 12 & 0xFFFFF000 + 0xFFF;
  printf("%x\n", value);
  spi_write_blocking(SSEG_SPI_INST, (uint8_t *)&value, 3);
  gpio_put(SSEG_SPI_RCLK_PIN, 1);
  sleep_us(64);
  gpio_put(SSEG_SPI_RCLK_PIN, 0);
}

void vSevenSegmentDisplayTask(__unused void *pvParameters)
{
  sseg_init();

  uint32_t on = 0b11111111111111111111;
  uint32_t off = 0;

  while (true)
  {
    sseg_write_raw(on);
    sleep_ms(1000);
    sseg_write_raw(off);
    sleep_ms(1000);
  }
}
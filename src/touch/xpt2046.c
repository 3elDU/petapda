#include "xpt2046.h"
#include "screen/screen.h"

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/time.h>
#include <stdio.h>

volatile unsigned short touch_x = 0;
volatile unsigned short touch_y = 0;
volatile unsigned long touch_max = 0;

// The functions touch_write_byte, touch_read_byte, etc. implement software SPI
// Since the two spi instances on the RP2040 are already used by the SD card and the display,
// we don't have another instance to use for XPT2046.

uint8_t touch_write_byte(uint8_t data)
{
  // for (uint8_t i = 0; i < 8; i++)
  // {
  //   gpio_put(XPT2046_PIN_CLK, 1);
  //   gpio_put(XPT2046_PIN_TX, data >> (7 - i) & 1);
  //   pico_default_asm("nop;nop;nop;nop;nop;nop;nop;nop");
  //   gpio_put(XPT2046_PIN_CLK, 0);
  //   pico_default_asm("nop;nop;nop;nop;nop;nop;nop;nop");
  // }

  spi_write_blocking(spi0, &data, 1);
}

uint8_t touch_read_byte()
{
  uint8_t data = 0;

  // for (uint8_t i = 0; i < 8; i++)
  // {
  //   gpio_put(XPT2046_PIN_CLK, 1);
  //   pico_default_asm("nop;nop;nop;nop;nop;nop;nop;nop");

  //   data <<= 1;
  //   if (gpio_get(XPT2046_PIN_RX))
  //   {
  //     data |= 1;
  //   }

  //   gpio_put(XPT2046_PIN_CLK, 0);
  //   pico_default_asm("nop;nop;nop;nop;nop;nop;nop;nop");
  // }

  spi_read_blocking(spi0, 0xFF, &data, 1);

  return data;
}

uint16_t touch_read_word()
{
  uint16_t data = touch_read_byte();
  data <<= 8;
  data |= touch_read_byte();
  return data;
}

void touch_read_bytes(uint8_t *data, uint8_t count)
{
  for (uint8_t i = 0; i < count; i++)
  {
    data[i] = touch_read_byte();
  }
}

uint16_t touch_tx8_rx16(uint8_t byte)
{
  touch_write_byte(byte);
  return touch_read_word();
}

uint16_t xpt2046_read_adc(uint8_t command)
{
  touch_write_byte(command);
  sleep_us(200);

  uint16_t data = touch_read_byte();
  data <<= 8;
  data |= touch_read_byte();
  data >>= 3;

  printf("xpt2046_read_adc(%0x) -> %0x\n", command, data);

  return data;
}

uint16_t xpt2046_read_adc_average(uint8_t command)
{
  uint16_t buf[XPT2046_AVERAGE_POINTS];
  uint16_t sum = 0;

  for (int i = 0; i < XPT2046_AVERAGE_POINTS; i++)
  {
    buf[i] = xpt2046_read_adc(command);
    sleep_us(200);
  }

  // Sort from lowest to highest
  for (int i = 0; i < XPT2046_AVERAGE_POINTS - 1; i++)
  {
    for (int j = i + 1; j < XPT2046_AVERAGE_POINTS; j++)
    {
      if (buf[i] > buf[j])
      {
        uint16_t temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  // Exclude the largest and the smallest
  for (int i = 1; i < XPT2046_AVERAGE_POINTS - XPT2046_DISCARD_NUM; i++)
  {
    sum += buf[i];
  }

  return sum / (XPT2046_AVERAGE_POINTS - 2 * XPT2046_DISCARD_NUM);
}

void xpt2046_read_adc_xy(uint16_t *x, uint16_t *y)
{
  *x = xpt2046_read_adc_average(0xD0);
  *y = xpt2046_read_adc_average(0x90);
}

void xpt2046_update(uint _gpio, uint32_t _mask)
{
  uint8_t buf = screen_get_buffer();
  buf++;
  if (buf > SCREEN_BUFFERS - 1)
    buf = 0;
  screen_set_buffer(buf);
  screen_render();
}

bool xpt2046_read_twice_adc(uint16_t *x, uint16_t *y)
{
  uint16_t xadc1, yadc1, xadc2, yadc2;

  // Read the ADC values twice
  xpt2046_read_adc_xy(&xadc1, &yadc1);
  sleep_us(10);
  xpt2046_read_adc_xy(&xadc2, &yadc2);
  sleep_us(10);

  // The ADC error used twice is greater than ERR_RANGE to take the acreage
  if (((xadc2 <= xadc1 && xadc1 < xadc2 + XPT2046_ERROR_RANGE) ||
       (xadc1 <= xadc2 && xadc2 < xadc1 + XPT2046_ERROR_RANGE)) &&
      ((yadc2 <= yadc1 && yadc1 < yadc2 + XPT2046_ERROR_RANGE) ||
       (yadc1 <= yadc2 && yadc2 < yadc1 + XPT2046_ERROR_RANGE)))
  {
    *x = (xadc1 + xadc2) / 2;
    *y = (yadc1 + yadc2) / 2;
    return true;
  }

  return false;
}

void xpt2046_init()
{
  // gpio_init(XPT2046_PIN_CS);
  gpio_init(XPT2046_PIN_RX);
  gpio_init(XPT2046_PIN_TX);
  gpio_init(XPT2046_PIN_CLK);
  // gpio_set_function(XPT2046_PIN_CS, GPIO_FUNC_SPI);
  // gpio_set_function(XPT2046_PIN_RX, GPIO_FUNC_SPI);
  // gpio_set_function(XPT2046_PIN_TX, GPIO_FUNC_SPI);
  // gpio_set_function(XPT2046_PIN_CLK, GPIO_FUNC_SPI);
  // spi_init(spi0, 500 * 1000);
  gpio_init(XPT2046_PIN_IRQ);
  gpio_set_dir(XPT2046_PIN_IRQ, GPIO_IN);

  gpio_put(XPT2046_PIN_CS, 0);

  // gpio_set_dir(XPT2046_PIN_CS, GPIO_OUT);
  // gpio_set_dir(XPT2046_PIN_CLK, GPIO_OUT);
  // gpio_set_dir(XPT2046_PIN_RX, GPIO_IN);
  // gpio_set_dir(XPT2046_PIN_TX, GPIO_OUT);
  gpio_set_irq_enabled_with_callback(XPT2046_PIN_IRQ, GPIO_IRQ_EDGE_FALL, true, xpt2046_update);
}

xpt2046_touch_event xpt2046_get_last_touch_position()
{
  return (xpt2046_touch_event){.x = touch_x, .y = touch_y};
}
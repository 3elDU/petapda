#ifndef ILI9431_H
#define ILI9431_H

#include <hardware/spi.h>

#define ILI_ORIENTATION_LANDSCAPE 0x20 | 0x08                       // D2U_L2R
#define ILI_ORIENTATION_PORTRAIT 0x40 | 0x08                        // L2R_U2D
#define ILI_ORIENTATION_LANDSCAPE_FLIPPED 0x40 | 0x80 | 0x20 | 0x08 // U2D_R2L
#define ILI_ORIENTATION_PORTRAIT_FLIPPED 0x80 | 0x08                // R2L_D2U
// #define ILI_SELECTED_ORIENTATION ILI_ORIENTATION_PORTRAIT
#define ILI_SELECTED_ORIENTATION ILI_ORIENTATION_PORTRAIT

#if ILI_SELECTED_ORIENTATION & 0x40
#define ILI_DISPLAY_WIDTH 240
#define ILI_DISPLAY_HEIGHT 320
#else
#define ILI_DISPLAY_WIDTH 320
#define ILI_DISPLAY_HEIGHT 240
#endif

#define ILI_SPI_INST spi1
#define ILI_SPI_CS_PIN 13
#define ILI_SPI_CLK_PIN 10
#define ILI_SPI_TX_PIN 11
#define ILI_SPI_RX_PIN 12
#define ILI_PIN_DC 8 // Data/Command selection pi240n. Low for command, high for data
#define ILI_PIN_RESET 9
#define ILI_PIN_LED 7 // Pin to turn the backlight on/off

typedef enum
{
  IL_CMD_SOFTWARE_RESET = 0x01,
  IL_CMD_MEMORY_ACCESS_CONTROL = 0x36,
  IL_CMD_PIXEL_FORMAT_SET = 0x3a,
  IL_CMD_SLEEP_MODE_ON = 0x10,
  IL_CMD_SLEEP_MODE_OFF = 0x11,
  IL_CMD_INVERT_ON = 0x20,
  IL_CMD_INVERT_OFF = 0x21,
  IL_CMD_DISPLAY_OFF = 0x28,
  IL_CMD_DISPLAY_ON = 0x29,
  IL_CMD_COLUMN_ADDRESS_SET = 0x2a,
  IL_CMD_PAGE_ADDRESS_SET = 0x2b,
  IL_CMD_MEMORY_WRITE = 0x2c,
  IL_CMD_TEARING_LINE_OFF = 0x34,
  IL_CMD_TEARING_LINE_ON = 0x35,
  IL_CMD_IDLE_MODE_OFF = 0x38,
  IL_CMD_IDLE_MODE_ON = 0x39,
  IL_CMD_SET_BRIGHTNESS = 0x51,
  IL_CMD_CONTENT_ADAPTIVE_BRIGHTNESS = 0x55,
  IL_CMD_NORMAL_MODE_FRAME_RATE = 0xb1,
  IL_CMD_IDLE_MODE_FRAME_RATE = 0xb2,
  IL_CMD_INTERFACE_CONTROL = 0xf6,
} ili9341_command;

// Convert the color from RGB888 to RGB565 format, recognized by the display
// Also shifts the bits to big-endian
uint16_t ili9341_color565(uint8_t r, uint8_t g, uint8_t b);

void ili9341_send_command_no_args(ili9341_command cmd);
void ili9341_send_command(ili9341_command cmd, uint8_t args_count, uint8_t args[]);
void ili9341_send_data(void *data, uint32_t length);
void ili9341_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void ili9341_init(void);

#endif
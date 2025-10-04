#include "screen.h"

#include <pico/stdio.h>
#include <pico/printf.h>
#include <string.h>
#include <stdarg.h>

character_t screenbufs[SCREEN_BUFFERS][SCREEN_LINES][SCREEN_COLUMNS];
// One pixel is stored using 2 bits
uint8_t screenbuf[EPD_4IN2_V2_WIDTH * EPD_4IN2_V2_HEIGHT / 8];
// The active screen buffer
uint8_t act = 0;

uint16_t cursor_x = 0;
uint16_t cursor_y = 0;
// These are applied when the text is printer under the cursor
uint16_t cursor_background = BACKGROUND;
uint16_t cursor_foreground = FOREGROUND;
uint16_t cursor_flags = 0;

void screen_set_pixel(uint16_t x, uint16_t y, bool value)
{
  uint16_t idx = (y * EPD_4IN2_V2_WIDTH + x) / 8;

  uint8_t mask = ~(1 << (7 - x % 8));
  uint8_t current_pixel_masked = screenbuf[idx] & mask;
  screenbuf[idx] = current_pixel_masked | value << (7 - x % 8);
}

void screen_set_char(uint16_t x, uint16_t y, character_t character)
{
  screenbufs[act][y][x] = character;
}
void screen_set_char_default(uint16_t x, uint16_t y, char character)
{
  screenbufs[act][y][x] = (character_t){
      .background = cursor_background,
      .foreground = cursor_foreground,
      .flags = cursor_flags,
      .character = character,
  };
}

void screen_move_cursor(uint16_t x, uint16_t y)
{
  cursor_x = x;
  cursor_y = y;
}
void screen_move_cursor_relative(int16_t x, int16_t y)
{
  cursor_x += x;
  cursor_y += y;

  if (cursor_x >= SCREEN_COLUMNS)
  {
    cursor_x = 0;
    cursor_y++;
  }

  if (cursor_y >= SCREEN_LINES)
  {
    cursor_y = SCREEN_LINES - 1;
    cursor_x = SCREEN_COLUMNS - 1;
  }
}
cursor_t screen_get_cursor()
{
  return (cursor_t){
      .x = cursor_x,
      .y = cursor_y,
  };
}

void screen_set_foreground(uint16_t color)
{
  cursor_foreground = color;
}
void screen_set_background(uint16_t color)
{
  cursor_background = color;
}
void screen_set_color(uint16_t background, uint16_t foreground)
{
  cursor_foreground = foreground;
  cursor_background = background;
}
void screen_set_flag(uint8_t flag)
{
  cursor_flags = flag;
}

void screen_fill_background(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  for (uint16_t y = y0; y <= y1; y++)
  {
    for (uint16_t x = x0; x <= x1; x++)
    {
      if (x >= SCREEN_COLUMNS || y >= SCREEN_LINES)
        continue;

      screenbufs[act][y][x].background = cursor_background;
    }
  }
}

void screen_set_text(uint16_t x, uint16_t y, char *text)
{
  uint16_t i = 0;
  char curchar;

  while ((curchar = text[i++]))
  {
    switch (curchar)
    {
    case '\n':
      y++;
      x = 0;
      break;

    case '\t':
      x += 4;
      break;

    default:
      screen_set_char(x, y, (character_t){
                                .background = cursor_background,
                                .foreground = cursor_foreground,
                                .flags = cursor_flags,
                                .character = curchar,
                            });
      x++;
    }

    // Check for horizontal overflow
    if (x >= SCREEN_COLUMNS)
    {
      y++;
      x = 0;
    }

    // Check for overflows
    if (y >= SCREEN_LINES)
    {
      break;
    }
  }
}

void screen_print_char(char character)
{
  switch (character)
  {
  case '\n':
    cursor_y++;
    cursor_x = 0;
    break;
  case '\t':
    cursor_x += 4;
    break;

  default:
    screen_set_char_default(cursor_x, cursor_y, character);
    cursor_x++;
  }

  // Check for horizontal overflow
  if (cursor_x >= SCREEN_COLUMNS)
  {
    cursor_y++;
    cursor_x = 0;
  }

  if (cursor_y >= SCREEN_LINES)
  {
    screen_scroll(1);
    cursor_y = SCREEN_LINES - 1;
    cursor_x = 0;
  }
}

void screen_print_text(const char *text)
{
  uint16_t i = 0;
  char curchar;

  while ((curchar = text[i++]))
  {
    screen_print_char(curchar);
  }
}

void screen_printf(const char *format, ...)
{
  va_list args;
  va_start(args, format);

  char buf[1024];
  vsprintf(buf, format, args);

  screen_print_text(buf);
}

uint8_t screen_get_buffer()
{
  return act;
}

void screen_set_buffer(uint8_t buffer)
{
  if (buffer > SCREEN_BUFFERS - 1)
  {
    buffer = SCREEN_BUFFERS - 1;
  }

  act = buffer;
}

void screen_clear()
{
  for (uint16_t y = 0; y < SCREEN_LINES; y++)
  {
    for (uint16_t x = 0; x < SCREEN_COLUMNS; x++)
    {
      screen_set_char(x, y, (character_t){
                                .background = BACKGROUND,
                                .foreground = FOREGROUND,
                                .flags = 0,
                                .character = ' ',
                            });
    }
  }
  screen_move_cursor(0, 0);
}

void screen_init()
{
  memset(screenbufs[act], 0, SCREEN_LINES * SCREEN_COLUMNS * sizeof(character_t));
}

void screen_scroll(uint8_t lines)
{
  for (uint16_t y = 0; y < SCREEN_LINES; y++)
  {
    if (y + lines < SCREEN_LINES)
    {
      memcpy(&screenbufs[act][y], &screenbufs[act][y + lines], SCREEN_COLUMNS * sizeof(character_t));
    }
    else
    {
      for (uint16_t x = 0; x < SCREEN_COLUMNS; x++)
      {
        screenbufs[act][y][x] = (character_t){
            .background = BACKGROUND,
            .foreground = FOREGROUND,
            .flags = 0,
            .character = ' ',
        };
      }
    }
  }
}

void screen_render()
{
  for (uint16_t y = 0; y < SCREEN_LINES; y++)
  {
    for (uint16_t x = 0; x < SCREEN_COLUMNS; x++)
    {

      character_t character = screenbufs[act][y][x];

      for (uint16_t py = 0; py < FONT_HEIGHT; py++)
      {
        for (uint16_t px = x * FONT_WIDTH; px < x * FONT_WIDTH + FONT_WIDTH; px++)
        {
          uint8_t pixel;

          uint8_t local_x = (px % FONT_WIDTH) / FONT_SCALING;
          uint8_t local_y = py / FONT_SCALING;

          if (local_x >= CHAR_WIDTH || local_y >= CHAR_HEIGHT)
          {
            pixel = character.background;
          }
          else
          {
            bool filled = font[character.character][local_y][local_x];
            pixel = filled ? character.foreground : character.background;
          }

          screen_set_pixel(px, y * FONT_HEIGHT + py, pixel);
        }
      }
    }
  }

  EPD_4IN2_V2_Display_Fast(screenbuf);
}
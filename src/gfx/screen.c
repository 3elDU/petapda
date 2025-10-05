#include "screen.h"

#include <pico/stdio.h>
#include <pico/printf.h>
#include <string.h>
#include <stdlib.h>

// 8 bytes can store 8 pixels, so divide width by eight.
static uint8_t framebufs[1][EINK_HEIGHT][EINK_WIDTH / 8];
// active framebuffer
static uint8_t act = 0;

#define PIXEL(x, y) framebufs[act][y][x >> 3]
#define MASK(x) (uint8_t)(0b10000000 >> (x & 0b111))

void screen_set(uint16_t x, uint16_t y, color_t color)
{
  uint8_t pixel = PIXEL(x, y) & ~MASK(x);
  pixel |= color & MASK(x);
  PIXEL(x, y) = pixel;
}
color_t screen_get(uint16_t x, uint16_t y)
{
  return (color_t)((PIXEL(x, y) >> (7 - x & 0b111)) & 0x1);
}

void screen_fill(color_t color)
{
  memset(&framebufs[act], color ? 0xFF : 0, EINK_WIDTH * EINK_HEIGHT / 8);
}

void screen_fill_rect(box_t rect, color_t color)
{
  for (uint16_t y = rect.y; y < rect.y1 && y < EINK_HEIGHT; y++)
  {
    for (uint16_t x = rect.x; x < rect.x1 && x < EINK_WIDTH; x++)
    {
      screen_set(x, y, color);
    }
  }
}

void screen_outline_rect(box_t rect, color_t outline)
{
  // top / bottom lines
  for (uint16_t x = rect.x; x < rect.x1 && x < EINK_WIDTH; x++)
  {
    screen_set(x, rect.y, outline);
    screen_set(x, rect.y1, outline);
  }
  // left / right lines
  for (uint16_t y = rect.y; y < rect.y1 && y < EINK_HEIGHT; y++)
  {
    screen_set(rect.x, y, outline);
    screen_set(rect.x1, y, outline);
  }
}

void screen_draw_line(box_t line, color_t color)
{
  int dx = abs(line.x1 - line.x);
  int sx = (line.x < line.x1) ? 1 : -1;
  int dy = -abs(line.y1 - line.y);
  int sy = (line.y < line.y1) ? 1 : -1;
  int err = dx + dy; // error term

  while (1)
  {
    screen_set(line.x, line.y, color);

    if (line.x == line.x1 && line.y == line.y1)
      break;

    int e2 = 2 * err;
    if (e2 >= dy)
    { // step in x
      err += dy;
      line.x += sx;
    }
    if (e2 <= dx)
    { // step in y
      err += dx;
      line.y += sy;
    }
  }
}

box_size_t screen_measure_char(__unused char ch)
{
  return (box_size_t){
      .w = CHAR_WIDTH,
      .h = CHAR_HEIGHT,
  };
}

box_size_t screen_draw_char(uint16_t x, uint16_t y, char ch, color_t color)
{
  for (uint16_t fy = 0; fy < CHAR_WIDTH; fy++)
  {
    for (uint16_t fx = 0; fx < CHAR_HEIGHT; fx++)
    {
      if (x + fx < EINK_WIDTH && y + fy < EINK_HEIGHT)
      {
        bool filled = font[ch][fy][fx];

        if (filled)
        {
          screen_set(x + fx, y + fy, color);
        }
      }
    }
  }

  return (box_size_t){
      .w = CHAR_WIDTH,
      .h = CHAR_HEIGHT,
  };
}

static box_size_t screen_draw_text_internal(uint16_t x0, uint16_t y0, uint16_t width, const char *text, color_t color, bool draw)
{
  box_size_t box = {
      .w = 0,
      .h = CHAR_HEIGHT,
  };
  uint16_t i = 0;
  char ch;

  // Those are the current X / Y coordinates we're at
  uint16_t x = x0, y = y0;

  while ((ch = text[i++]))
  {
    // add spacing from previous character
    if (i > 0)
      x += CHAR_SPACING;

    box_size_t chbox = draw ? screen_draw_char(x, y, ch, color) : screen_measure_char(ch);

    x += chbox.w;

    // if close to requested box width, wrap to new line
    if (x >= x0 + width - CHAR_WIDTH)
    {
      x = x0;
      y += FONT_HEIGHT;
    }

    // Update box with maximum X / Y values
    int w = (int)x - (int)x0, h = (int)y - (int)y0;
    if (w > box.w)
      box.w = w;
    if (h > box.h)
      box.h = h;
  }

  return box;
}

box_size_t screen_draw_text_fit(uint16_t x, uint16_t y, const char *text, color_t color, uint16_t width)
{
  return screen_draw_text_internal(x, y, width, text, color, true);
}
box_size_t screen_draw_text(uint16_t x, uint16_t y, const char *text, color_t color)
{
  return screen_draw_text_internal(x, y, EINK_WIDTH - x, text, color, true);
}

box_size_t screen_measure_text_fit(const char *text, uint16_t width)
{
  return screen_draw_text_internal(0, 0, width, text, 0, false);
}
box_size_t screen_measure_text(const char *text)
{
  return screen_draw_text_internal(0, 0, EINK_WIDTH, text, 0, false);
}

void screen_full_refresh()
{
  eink_display((uint8_t *)framebufs[act]);
}
void screen_fast_refresh()
{
  eink_fast_display((uint8_t *)framebufs[act]);
}
void screen_partial_refresh()
{
  eink_partial_display((uint8_t *)framebufs[act], 0, 0, EINK_WIDTH, EINK_HEIGHT);
}
void screen_partial_refresh_area(box_t area)
{
  eink_partial_display((uint8_t *)framebufs[act], area.x, area.y, area.x1, area.y1);
}
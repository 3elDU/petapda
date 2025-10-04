#ifndef GFX_SCREEN_H
#define GFX_SCREEN_H

#include <dev/eink.h>
#include <stdint.h>
#include "font.h"

typedef uint8_t color_t;

// Common colors, in RGB565 format
#define C_BLACK (color_t)0
#define C_WHITE (color_t)255

typedef struct
{
  uint16_t x;
  uint16_t y;
  uint16_t x1;
  uint16_t y1;
} box_t;

typedef struct
{
  uint16_t w;
  uint16_t h;
} box_size_t;

/** Set an individual pixel */
void screen_set(uint16_t x, uint16_t y, color_t color);
/** Get an individual pixel */
color_t screen_get(uint16_t x, uint16_t y);

/** Fill screen with solid color */
void screen_fill(color_t color);

/** Fill a rectangular area with solid color */
void screen_fill_rect(box_t rect, color_t color);
/** Draw an outline rectangle */
void screen_outline_rect(box_t rect, color_t outline);

/** Draw a line from point A to point B */
void screen_draw_line(box_t line, color_t color);

/**
 * Measure width and height of the character, without drawing it on screen
 *
 * @returns width and height of the character
 */
box_size_t screen_measure_char(char ch);

/**
 * Draw a single character on the screen
 *
 * @returns width and height of the character
 */
box_size_t screen_draw_char(uint16_t x, uint16_t y, char ch, color_t color);

/**
 * Draw text on the screen, while trying to fit it inside a box of specified width
 *
 * @returns width and height of the rendered text box
 */
box_size_t screen_draw_text_fit(uint16_t x, uint16_t y, const char *text, color_t color, uint16_t width);
/**
 * Draw text on the screen
 *
 * @returns width and height of the rendered text box
 */
box_size_t screen_draw_text(uint16_t x, uint16_t y, const char *text, color_t color);

/**
 * Measure width and height of given string on the screen, without drawing it, while trying to fit inside a box of specified width.
 *
 * @returns width and height of the text box
 */
box_size_t screen_measure_text_fit(const char *text, uint16_t width);
/**
 * Measure width and height of given string on the screen, without drawing it.
 *
 * @returns width and height of the text box
 */
box_size_t screen_measure_text(const char *text);

/**
 * Do a full refresh. This is the slowest refresh, screen will flicker multiple
 * times, but this removes any ghosting left by fast/partial refreshes.
 */
void screen_full_refresh();
/**
 * Do a fast refresh. Screen will flicker only once with negative of the image.
 * I have not spotted any ghosting left by a fast refresh.
 */
void screen_fast_refresh();
/**
 * Do a partial refresh of the whole screen. This is the fastest refresh method,
 * showing new image immediately, but leaving some ghosting of the previous image.
 *
 * After a couple partial refreshes, it is advised to do a full refresh.
 */
void screen_partial_refresh();
/**
 * Do a partial refresh of an area.
 * Similar to `screen_partial_refresh()`, but only shows the specified area
 */
void screen_partial_refresh_area(box_t area);

#endif
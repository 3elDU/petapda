#ifndef GFX_SCREEN_H
#define GFX_SCREEN_H

#include <dev/eink.h>
#include <stdint.h>
#include "font.h"

// Screen Y resolution in characters
#define SCREEN_LINES (EPD_4IN2_V2_HEIGHT / FONT_HEIGHT)
// Screen X resolution in characters
#define SCREEN_COLUMNS (EPD_4IN2_V2_WIDTH / FONT_WIDTH)
#define SCREEN_BUFFERS 1

// Common colors, in RGB565 format
#define BLACK 0
#define WHITE 1
#define BACKGROUND WHITE
#define FOREGROUND BLACK

/* A character with all metadata required to render it */
typedef struct
{
  uint16_t background;
  uint16_t foreground;
  uint8_t flags;
  char character;
} character_t;

typedef struct
{
  uint16_t x;
  uint16_t y;
} cursor_t;

// Set the character at given position
void screen_set_char(uint16_t x, uint16_t y, character_t character);
// Set the character at given position with the default colors
void screen_set_char_default(uint16_t x, uint16_t y, char character);

// Move cursor to a specific position
void screen_move_cursor(uint16_t x, uint16_t y);
// Move cursor relative to it's current position. Handles horizontal overflow
void screen_move_cursor_relative(int16_t x, int16_t y);
// Get cursor position on the screen
cursor_t screen_get_cursor();

// Set the foreground color for text
void screen_set_foreground(uint16_t color);
// Set the background color for text
void screen_set_background(uint16_t color);
// Set the foreground and background colors for text
void screen_set_color(uint16_t background, uint16_t foreground);
// Set the flags for text
void screen_set_flag(uint8_t flag);

// Fill the rectangle with the current background color.
// Does not clear characters in the way, only changes their background color
void screen_fill_background(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

// Set the text at given position. Handles line breaks, overflows and tabulation
void screen_set_text(uint16_t x, uint16_t y, char *text);

// Print a character at the cursor position
void screen_print_char(char character);
// Print text at the cursor position
void screen_print_text(const char *text);
// Print formatted text at the cursor position
void screen_printf(const char *format, ...);

// Get the active screen buffer
uint8_t screen_get_buffer();
// Set the active screen buffer
void screen_set_buffer(uint8_t buffer);
// Clear the screen
void screen_clear();
// Scroll the screen by N lines up
void screen_scroll(uint8_t lines);
// Render the screen buffer to the screen
void screen_render();

void screen_init();

#endif
#ifndef SEVENSEGMENT_H
#define SEVENSEGMENT_H

#include "hardware/spi.h"

// The displays are connected through a shift register
// For more info, take a look at README in this directory

#define SSEG_SPI_INST spi1
// This pin gets used as the SPI clock
// Pulsing it high shifts the bit in the shift register
#define SSEG_SPI_SRCLK_PIN 10
// Here we supply the bit value to shift in
#define SSEG_SPI_TX_PIN 11
// When this pin goes high, the bits stored in the shift registers
// get "applied" to the actual outputs
// We control it manually here
#define SSEG_SPI_RCLK_PIN 13

// Those are the low-level function to work with the display
void sseg_init();
// Writes 24 bits to the shift registers
void sseg_write_raw(uint32_t value);

#define SSEG_SCROLLING_TEXT_SPEED 10

// Show a line of text on the display, that is scrolled across the screen when too long
void sseg_show_scrolling_text(const char *text);
// Same as `sseg_show_scrolling_text`, but clears the display after a specified timeout in milliseconds
void sseg_show_scrolling_text_timeout(const char *text, uint32_t timeout_ms);

// This is the FreeRTOS task that drives the display
// We can control it through the high-level functions provided above
void vSevenSegmentDisplayTask(__unused void *pvParameters);

#endif
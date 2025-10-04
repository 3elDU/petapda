#ifndef DEV_EINK_H
#define DEV_EINK_H

#include <sys/bus.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <stdint.h>

// Pinout
#define EINK_RST_PIN 2
#define EINK_DC_PIN 3
#define EINK_CS_PIN 4
#define EINK_BUSY_PIN 1

// Display resolution
#define EINK_WIDTH 400
#define EINK_HEIGHT 300
// Size of buffer to hold whole screen
#define EINK_BUFSIZE EINK_WIDTH *EINK_HEIGHT / 8

void eink_init();
void eink_display(const uint8_t *image);
void eink_fast_display(const uint8_t *image);
void eink_4gray_display(const uint8_t *Image);
void eink_partial_display(const uint8_t *image, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void eink_sleep(void);

#endif
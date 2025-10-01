#ifndef DEV_EINK_H
#define DEV_EINK_H

#include <sys/bus.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <stdint.h>

// Pinout
#define EPD_RST_PIN 2
#define EPD_DC_PIN 3
#define EPD_CS_PIN 4
#define EPD_BUSY_PIN 1

// Display resolution
#define EPD_4IN2_V2_WIDTH 400
#define EPD_4IN2_V2_HEIGHT 300

#define Seconds_1_5S 0
#define Seconds_1S 1

void EPD_4IN2_V2_Init(void);
void EPD_4IN2_V2_Init_Fast(uint8_t Mode);
void EPD_4IN2_V2_Init_4Gray(void);
void EPD_4IN2_V2_Clear(void);
void EPD_4IN2_V2_Display(uint8_t *Image);
void EPD_4IN2_V2_Display_Fast(uint8_t *Image);
void EPD_4IN2_V2_Display_4Gray(const uint8_t *Image);
void EPD_4IN2_V2_PartialDisplay(uint8_t *Image, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);
void EPD_4IN2_V2_Sleep(void);

#endif
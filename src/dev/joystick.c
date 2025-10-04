#include "joystick.h"

#include <hardware/gpio.h>
#include <hardware/adc.h>
#include <pico/printf.h>

void joystick_init()
{
    adc_init();
    adc_gpio_init(JOYSTICK_PIN_X);
    adc_gpio_init(JOYSTICK_PIN_Y);

    gpio_init(JOYSTICK_PIN_BTN);
    gpio_set_dir(JOYSTICK_PIN_BTN, false);
}

joy_state_t joystick_read()
{
    joy_state_t joy;
    uint16_t xraw, yraw;
    // Conversion factor for converting from raw adc input to [0; 2] range
    const float conv_factor = 2.f / (1 << 12);

    adc_select_input(0);
    xraw = adc_read();
    adc_select_input(1);
    yraw = adc_read();

    return (joy_state_t){
        .x = xraw * conv_factor - 1,
        .y = yraw * conv_factor - 1,
        .pressed = gpio_get(JOYSTICK_PIN_BTN),
    };
}
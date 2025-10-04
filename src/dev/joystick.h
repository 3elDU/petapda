#ifndef JOYSTICK_H
#define JOYSTICK_H

#define JOYSTICK_PIN_X 26
#define JOYSTICK_PIN_Y 27
#define JOYSTICK_PIN_BTN 5

#include <stdbool.h>

/**
 * Code to handle input from analog joystick.
 *
 * It can move in X and Y axis, and has a button that can be pressed.
 */

typedef struct
{
    float x;
    float y;
    bool pressed;
} joy_state_t;

void joystick_init();
joy_state_t joystick_read();

#endif
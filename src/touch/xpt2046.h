#ifndef ILI9431_TOUCH_H
#define ILI9431_TOUCH_H

#include <stdint.h>
#include <stdbool.h>

#define XPT2046_PIN_IRQ 1
#define XPT2046_PIN_CLK 2
#define XPT2046_PIN_TX 3
#define XPT2046_PIN_RX 4
#define XPT2046_PIN_CS 5

#define XPT2046_START_BIT (0x01 << 7)
#define XPT2046_A2_ENABLE 0x01 << 6
#define XPT2046_A1_ENABLE 0x01 << 5
#define XPT2046_A0_ENABLE 0x01 << 4
#define XPT2046_MODE_8BIT 0x01 << 3
#define XPT2046_MODE_SER 0x01 << 2
#define XPT2046_PD1_ENABLE 0x01 << 1
#define XPT2046_PD0_ENABLE 0x01 << 0

#define XPT2046_AVERAGE_POINTS 5
#define XPT2046_DISCARD_NUM 1
#define XPT2046_ERROR_RANGE 50

typedef enum pressedModes
{
  XPT2046_NOT_PRESSED = 0,
  XPT2046_PRESSED = 1
} xpt2046_pressed_modes;
typedef enum powerModes
{
  XPT2046_POWER_DOWN = 0x00,
  XPT2046_REFERENCE_OFF_ADC_ON = XPT2046_PD0_ENABLE,
  XPT2046_REFERENCE_ON_ADC_OFF = XPT2046_PD1_ENABLE,
  XPT2046_DEVICE_ALWAYS_ON = XPT2046_PD1_ENABLE | XPT2046_PD0_ENABLE
} xpt2046_power_modes;

typedef enum referenceModes
{
  XPT2046_DFR_MODE = 0,
  XPT2046_SER_MODE = XPT2046_MODE_SER
} xpt2046_reference_modes;

typedef enum
{
  XPT2046_SER_TEMP0 = 0,
  XPT2046_SER_Y = XPT2046_A0_ENABLE,
  XPT2046_SER_VBAT = XPT2046_A1_ENABLE,
  XPT2046_SER_Z1 = XPT2046_A1_ENABLE | XPT2046_A0_ENABLE,
  XPT2046_SER_Z2 = XPT2046_A2_ENABLE,
  XPT2046_SER_X = XPT2046_A2_ENABLE | XPT2046_A0_ENABLE,
  XPT2046_SER_AUXIN = XPT2046_A2_ENABLE | XPT2046_A1_ENABLE,
  XPT2046_SER_TEMP1 = XPT2046_A2_ENABLE | XPT2046_A1_ENABLE | XPT2046_A0_ENABLE,
  XPT2046_DFR_Y = XPT2046_A0_ENABLE,
  XPT2046_DFR_Z1 = XPT2046_A1_ENABLE | XPT2046_A0_ENABLE,
  XPT2046_DFR_Z2 = XPT2046_A2_ENABLE,
  XPT2046_DFR_X = XPT2046_A2_ENABLE | XPT2046_A0_ENABLE
} xpt2046_channel_modes;

typedef enum bitModes
{
  XPT2046_12BIT_MODE = 0,
  XPT2046_8BIT_MODE = XPT2046_MODE_8BIT
} xpt2046_bit_modes;

typedef enum startBits
{
  XPT2046_NONE = 0,
  XPT2046_START = XPT2046_START_BIT
} xpt2046_start_modes;

typedef struct
{
  unsigned short x;
  unsigned short y;
} xpt2046_touch_event;

/*******************************************************************************
function:
        Read the ADC of the channel
parameter:
    Channel_Cmd :	0x90: Read channel Y +, select the ADC resolution is 12 bits, set to differential mode
                    0xd0: Read channel x +, select the ADC resolution is 12 bits, set to differential mode
*******************************************************************************/
uint16_t xpt2046_read_adc(uint8_t command);
/*******************************************************************************
function:
        Read the 5th channel value and exclude the maximum and minimum returns the average
parameter:
    Channel_Cmd :	0x90 :Read channel Y +
                    0xd0 :Read channel x +
*******************************************************************************/
uint16_t xpt2046_read_adc_average(uint8_t command);
/*******************************************************************************
function:
        Read X channel and Y channel AD value
parameter:
    Channel_Cmd :	0x90 :Read channel Y +
                    0xd0 :Read channel x +
*******************************************************************************/
void xpt2046_read_adc_xy(uint16_t *x, uint16_t *y);

/*******************************************************************************
function:
        2 times to read the touch screen IC, and the two can not exceed the deviation,
        ERR_RANGE, meet the conditions, then that the correct reading, otherwise the reading error.
parameter:
    Channel_Cmd :	pYCh_Adc = 0x90 :Read channel Y +
                    pXCh_Adc = 0xd0 :Read channel x +
*******************************************************************************/
bool xpt2046_read_twice_adc(uint16_t *x, uint16_t *y);

void xpt2046_init();
xpt2046_touch_event xpt2046_get_last_touch_position();

#endif
#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <pico/float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TP_PRESS_DOWN 0x80
#define TP_PRESSED 0x40

// Touch screen structure
typedef struct
{
	uint16_t Xpoint0;
	uint16_t Ypoint0;
	uint16_t Xpoint;
	uint16_t Ypoint;
	uint8_t chStatus;
	uint8_t chType;
	int16_t iXoff;
	int16_t iYoff;
	float fXfac;
	float fYfac;
} TP_DEV;

// Brush structure
typedef struct
{
	uint16_t Xpoint;
	uint16_t Ypoint;
	uint16_t Color;
	uint8_t DotPixel;
} TP_DRAW;

void TP_GetAdFac(void);
void TP_Adjust(void);
void TP_Dialog();
void TP_DrawBoard();
void TP_Init();

#endif

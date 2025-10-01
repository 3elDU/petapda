/*****************************************************************************
 * | File      	:	LCD_Touch.c
 * | Author      :   Waveshare team
 * | Function    :	LCD Touch Pad Driver and Draw
 * | Info        :
 *   Image scanning
 *      Please use progressive scanning to generate images or fonts
 *----------------
 * |	This version:   V1.0
 * | Date        :   2017-08-16
 * | Info        :   Basic version
 *
 ******************************************************************************/
#include "calibration.h"
#include "xpt2046.h"

#include <screen/screen.h>
#include <hardware/gpio.h>
#include <pico/time.h>

// extern LCD_DIS sLCD_DIS;
// extern uint8_t id;
static TP_DEV sTP_DEV;
static TP_DRAW sTP_Draw;

/*******************************************************************************
function:
        Calculation
parameter:
        chCoordType:
                    1 : calibration
                    0 : relative position
*******************************************************************************/
static uint8_t TP_Scan(uint8_t chCoordType)
{

    // In X, Y coordinate measurement, IRQ is disabled and output is low
    if (!gpio_get(XPT2046_PIN_IRQ))
    { // Press the button to press
        // Read the physical coordinates
        if (chCoordType)
        {
            xpt2046_read_twice_adc(&sTP_DEV.Xpoint, &sTP_DEV.Ypoint);
            // Read the screen coordinates
        }
        else if (xpt2046_read_twice_adc(&sTP_DEV.Xpoint, &sTP_DEV.Ypoint))
        {

            if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_PORTRAIT_FLIPPED)
            { // Converts the result to screen coordinates
                sTP_Draw.Xpoint = sTP_DEV.fXfac * sTP_DEV.Xpoint +
                                  sTP_DEV.iXoff;
                sTP_Draw.Ypoint = sTP_DEV.fYfac * sTP_DEV.Ypoint +
                                  sTP_DEV.iYoff;
            }
            else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_PORTRAIT)
            {
                sTP_Draw.Xpoint = ILI_DISPLAY_WIDTH -
                                  sTP_DEV.fXfac * sTP_DEV.Xpoint -
                                  sTP_DEV.iXoff;
                sTP_Draw.Ypoint = ILI_DISPLAY_HEIGHT -
                                  sTP_DEV.fYfac * sTP_DEV.Ypoint -
                                  sTP_DEV.iYoff;
            }
            else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_LANDSCAPE_FLIPPED)
            {
                sTP_Draw.Xpoint = sTP_DEV.fXfac * sTP_DEV.Ypoint +
                                  sTP_DEV.iXoff;
                sTP_Draw.Ypoint = sTP_DEV.fYfac * sTP_DEV.Xpoint +
                                  sTP_DEV.iYoff;
            }
            else
            {
                sTP_Draw.Xpoint = ILI_DISPLAY_WIDTH -
                                  sTP_DEV.fXfac * sTP_DEV.Ypoint -
                                  sTP_DEV.iXoff;
                sTP_Draw.Ypoint = ILI_DISPLAY_HEIGHT -
                                  sTP_DEV.fYfac * sTP_DEV.Xpoint -
                                  sTP_DEV.iYoff;
            }
        }
        if (0 == (sTP_DEV.chStatus & TP_PRESS_DOWN))
        { // Not being pressed
            sTP_DEV.chStatus = TP_PRESS_DOWN | TP_PRESSED;
            sTP_DEV.Xpoint0 = sTP_DEV.Xpoint;
            sTP_DEV.Ypoint0 = sTP_DEV.Ypoint;
        }
    }
    else
    {
        if (sTP_DEV.chStatus & TP_PRESS_DOWN)
        {                                  // 0x80
            sTP_DEV.chStatus &= ~(1 << 7); // 0x00
        }
        else
        {
            sTP_DEV.Xpoint0 = 0;
            sTP_DEV.Ypoint0 = 0;
            sTP_DEV.Xpoint = 0xffff;
            sTP_DEV.Ypoint = 0xffff;
        }
    }

    return (sTP_DEV.chStatus & TP_PRESS_DOWN);
}

/*******************************************************************************
function:
        Draw Cross
parameter:
            Xpoint :	The x coordinate of the point
            Ypoint :	The y coordinate of the point
            Color  :	Set color
*******************************************************************************/
static void TP_DrawCross(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color)
{
    // Convert into character coordinates
    Xpoint /= 10;
    Ypoint /= 10;

    // horizontal line from x-12, y, x+12, y
    // GUI_DrawLine(Xpoint - 12, Ypoint, Xpoint + 12, Ypoint,
    //              Color, LINE_SOLID, DOT_PIXEL_1X1);
    // vertical line from y-12 to y+12
    // GUI_DrawLine(Xpoint, Ypoint - 12, Xpoint, Ypoint + 12,
    //              Color, LINE_SOLID, DOT_PIXEL_1X1);

    // GUI_DrawPoint(Xpoint, Ypoint, Color, DOT_PIXEL_2X2, DOT_FILL_AROUND);

    screen_set_background(Color);

    // horizontal line
    screen_fill_background(Xpoint - 1, Ypoint, Xpoint + 1, Ypoint);
    // vertical line
    screen_fill_background(Xpoint, Ypoint - 1, Xpoint, Ypoint + 1);

    // there's no circle drawing function, for now
    // GUI_DrawCircle(Xpoint, Ypoint, 6, Color, DRAW_EMPTY, DOT_PIXEL_1X1);

    screen_render();
}

/*******************************************************************************
function:
        The corresponding ADC value is displayed on the LC
parameter:
            (Xpoint0 ,Xpoint0):	The coordinates of the first point
            (Xpoint1 ,Xpoint1):	The coordinates of the second point
            (Xpoint2 ,Xpoint2):	The coordinates of the third point
            (Xpoint3 ,Xpoint3):	The coordinates of the fourth point
            hwFac	:	Percentage of error
*******************************************************************************/
static void TP_ShowInfo(uint16_t Xpoint0, uint16_t Ypoint0,
                        uint16_t Xpoint1, uint16_t Ypoint1,
                        uint16_t Xpoint2, uint16_t Ypoint2,
                        uint16_t Xpoint3, uint16_t Ypoint3,
                        uint16_t hwFac)
{
    // GUI_DrawRectangle(40, 160, 240, 270, WHITE, DRAW_FULL, DOT_PIXEL_1X1);
    screen_set_background(FOREGROUND);
    screen_fill_background(4, SCREEN_LINES / 2, SCREEN_COLUMNS - 1, 20);

    screen_set_foreground(RED);

    // GUI_DisString_EN(40, 160, "x1", TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisString_EN(40 + 80, 160, "y1", TP_Font, FONT_BACKGROUND, RED);
    screen_set_text(4, 16, "x1");
    screen_set_text(4 + 8, 16, "y1");

    // GUI_DisString_EN(40, 180, "x2", TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisString_EN(40 + 80, 180, "y2", TP_Font, FONT_BACKGROUND, RED);
    screen_set_text(4, 18, "x2");
    screen_set_text(4 + 8, 18, "y2");

    // GUI_DisString_EN(40, 200, "x3", TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisString_EN(40 + 80, 200, "y3", TP_Font, FONT_BACKGROUND, RED);
    screen_set_text(4, 20, "x3");
    screen_set_text(4 + 8, 20, "y3");

    // GUI_DisString_EN(40, 220, "x4", TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisString_EN(40 + 80, 220, "y4", TP_Font, FONT_BACKGROUND, RED);
    screen_set_text(4, 22, "x4");
    screen_set_text(4 + 8, 22, "y4");

    // GUI_DisString_EN(40, 240, "fac is : ", TP_Font, FONT_BACKGROUND, RED);
    screen_set_text(4, 24, "fac is : ");

    // GUI_DisNum(40 + 27, 160, Xpoint0, TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisNum(40 + 27 + 80, 160, Ypoint0, TP_Font, FONT_BACKGROUND, RED);
    screen_move_cursor(8, 16);
    screen_printf("%u", Xpoint0);
    screen_move_cursor(8 + 8, 16);
    screen_printf("%u", Ypoint0);

    // GUI_DisNum(40 + 27, 180, Xpoint1, TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisNum(40 + 27 + 80, 180, Ypoint1, TP_Font, FONT_BACKGROUND, RED);
    screen_move_cursor(8, 18);
    screen_printf("%u", Xpoint1);
    screen_move_cursor(16, 18);
    screen_printf("%u", Ypoint1);

    // GUI_DisNum(40 + 27, 200, Xpoint2, TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisNum(40 + 27 + 80, 200, Ypoint2, TP_Font, FONT_BACKGROUND, RED);
    screen_move_cursor(8, 20);
    screen_printf("%u", Xpoint2);
    screen_move_cursor(16, 20);
    screen_printf("%u", Ypoint2);

    // GUI_DisNum(40 + 27, 220, Xpoint3, TP_Font, FONT_BACKGROUND, RED);
    // GUI_DisNum(40 + 27 + 80, 220, Ypoint3, TP_Font, FONT_BACKGROUND, RED);
    screen_move_cursor(8, 22);
    screen_printf("%u", Xpoint3);
    screen_move_cursor(16, 22);
    screen_printf("%u", Ypoint3);

    // GUI_DisNum(40 + 56, 240, hwFac, TP_Font, FONT_BACKGROUND, RED);
    screen_move_cursor(13, 24);
    screen_printf("%u", hwFac);

    screen_render();
}

/*******************************************************************************
function:
        Touch screen adjust
*******************************************************************************/
void TP_Adjust(void)
{
    uint8_t cnt = 0;
    uint16_t XYpoint_Arr[4][2];
    uint32_t Dx, Dy;
    uint16_t Sqrt1, Sqrt2;
    float Dsqrt;

    screen_set_background(BACKGROUND);
    screen_clear();
    screen_render();

    // GUI_DisString_EN(0, 40, "Please use the stylus click the cross"
    //                         "on the screen. The cross will always move until"
    //                         "the screen adjustment is completed.",
    //                  &Font16, FONT_BACKGROUND, RED);

    screen_set_foreground(RED);
    screen_set_text(0, 4, "Please use the stylus to click the cross on the screen. The cross will always move until the screen adjustment is completed");

    uint8_t Mar_Val = 12;
    TP_DrawCross(Mar_Val, Mar_Val, RED);

    sTP_DEV.chStatus = 0;
    sTP_DEV.fXfac = 0;

    printf("wah1\n");
    while (1)
    {
        TP_Scan(1);
        if ((sTP_DEV.chStatus & 0xC0) == TP_PRESSED)
        {
            printf("wah2\n");
            sTP_DEV.chStatus &= ~(1 << 6);
            XYpoint_Arr[cnt][0] = sTP_DEV.Xpoint;
            XYpoint_Arr[cnt][1] = sTP_DEV.Ypoint;
            printf("X%d,Y%d = %d,%d\r\n", cnt, cnt, XYpoint_Arr[cnt][0], XYpoint_Arr[cnt][1]);
            cnt++;
            sleep_ms(200);

            switch (cnt)
            {
            case 1:
                // DEBUG("not touch TP_IRQ 2 = %d\r\n", GET_TP_IRQ);
                TP_DrawCross(Mar_Val, Mar_Val, WHITE);
                TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val, Mar_Val, RED);
                sleep_ms(200);
                break;
            case 2:
                // DEBUG("not touch TP_IRQ 3 = %d\r\n", GET_TP_IRQ);
                TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val, Mar_Val, WHITE);
                TP_DrawCross(Mar_Val, ILI_DISPLAY_HEIGHT - Mar_Val, RED);
                sleep_ms(200);
                break;
            case 3:
                // DEBUG("not touch TP_IRQ 4 = %d\r\n", GET_TP_IRQ);
                TP_DrawCross(Mar_Val, ILI_DISPLAY_HEIGHT - Mar_Val, WHITE);
                TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val,
                             ILI_DISPLAY_HEIGHT - Mar_Val, RED);
                sleep_ms(200);
                break;
            case 4:

                // 1.Compare the X direction
                Dx = abs((int16_t)(XYpoint_Arr[0][0] -
                                   XYpoint_Arr[1][0])); // x1 - x2
                Dy = abs((int16_t)(XYpoint_Arr[0][1] -
                                   XYpoint_Arr[1][1])); // y1 - y2
                Dx *= Dx;
                Dy *= Dy;
                Sqrt1 = sqrt(Dx + Dy);

                Dx = abs((int16_t)(XYpoint_Arr[2][0] -
                                   XYpoint_Arr[3][0])); // x3 - x4
                Dy = abs((int16_t)(XYpoint_Arr[2][1] -
                                   XYpoint_Arr[3][1])); // y3 - y4
                Dx *= Dx;
                Dy *= Dy;
                Sqrt2 = sqrt(Dx + Dy);

                Dsqrt = (float)Sqrt1 / Sqrt2;
                if (Dsqrt < 0.95 || Dsqrt > 1.05 || Sqrt1 == 0 || Sqrt2 == 0)
                {
                    // DEBUG("Adjust X direction \r\n");
                    cnt = 0;
                    TP_ShowInfo(XYpoint_Arr[0][0], XYpoint_Arr[0][1],
                                XYpoint_Arr[1][0], XYpoint_Arr[1][1],
                                XYpoint_Arr[2][0], XYpoint_Arr[2][1],
                                XYpoint_Arr[3][0], XYpoint_Arr[3][1],
                                Dsqrt * 100);
                    //    Driver_Delay_ms(1000);
                    TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val,
                                 ILI_DISPLAY_HEIGHT - Mar_Val, WHITE);
                    TP_DrawCross(Mar_Val, Mar_Val, RED);
                    continue;
                }

                // 2.Compare the Y direction
                Dx = abs((int16_t)(XYpoint_Arr[0][0] -
                                   XYpoint_Arr[2][0])); // x1 - x3
                Dy = abs((int16_t)(XYpoint_Arr[0][1] -
                                   XYpoint_Arr[2][1])); // y1 - y3
                Dx *= Dx;
                Dy *= Dy;
                Sqrt1 = sqrt(Dx + Dy);

                Dx = abs((int16_t)(XYpoint_Arr[1][0] -
                                   XYpoint_Arr[3][0])); // x2 - x4
                Dy = abs((int16_t)(XYpoint_Arr[1][1] -
                                   XYpoint_Arr[3][1])); // y2 - y4
                Dx *= Dx;
                Dy *= Dy;
                Sqrt2 = sqrt(Dx + Dy); //

                Dsqrt = (float)Sqrt1 / Sqrt2;
                if (Dsqrt < 0.95 || Dsqrt > 1.05)
                {
                    // DEBUG("Adjust Y direction \r\n");
                    cnt = 0;
                    TP_ShowInfo(XYpoint_Arr[0][0], XYpoint_Arr[0][1],
                                XYpoint_Arr[1][0], XYpoint_Arr[1][1],
                                XYpoint_Arr[2][0], XYpoint_Arr[2][1],
                                XYpoint_Arr[3][0], XYpoint_Arr[3][1],
                                Dsqrt * 100);
                    //    Driver_Delay_ms(1000);
                    TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val,
                                 ILI_DISPLAY_HEIGHT - Mar_Val, WHITE);
                    TP_DrawCross(Mar_Val, Mar_Val, RED);
                    continue;
                } //

                // 3.Compare diagonal
                Dx = abs((int16_t)(XYpoint_Arr[1][0] -
                                   XYpoint_Arr[2][0])); // x1 - x3
                Dy = abs((int16_t)(XYpoint_Arr[1][1] -
                                   XYpoint_Arr[2][1])); // y1 - y3
                Dx *= Dx;
                Dy *= Dy;
                Sqrt1 = sqrt(Dx + Dy); //;

                Dx = abs((int16_t)(XYpoint_Arr[0][0] -
                                   XYpoint_Arr[3][0])); // x2 - x4
                Dy = abs((int16_t)(XYpoint_Arr[0][1] -
                                   XYpoint_Arr[3][1])); // y2 - y4
                Dx *= Dx;
                Dy *= Dy;
                Sqrt2 = sqrt(Dx + Dy); //

                Dsqrt = (float)Sqrt1 / Sqrt2;
                if (Dsqrt < 0.95 || Dsqrt > 1.05)
                {
                    printf("Adjust diagonal direction\r\n");
                    cnt = 0;
                    TP_ShowInfo(XYpoint_Arr[0][0], XYpoint_Arr[0][1],
                                XYpoint_Arr[1][0], XYpoint_Arr[1][1],
                                XYpoint_Arr[2][0], XYpoint_Arr[2][1],
                                XYpoint_Arr[3][0], XYpoint_Arr[3][1],
                                Dsqrt * 100);
                    sleep_ms(1000);
                    TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val,
                                 ILI_DISPLAY_HEIGHT - Mar_Val, WHITE);
                    TP_DrawCross(Mar_Val, Mar_Val, RED);
                    continue;
                }

                // 4.Get the scale factor and offset
                // Get the scanning direction of the touch screen
                // sTP_DEV.TP_Scan_Dir = sLCD_DIS.LCD_Scan_Dir;
                sTP_DEV.fXfac = 0;

                // According to the display direction to get
                // the corresponding scale factor and offset
                if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_PORTRAIT_FLIPPED)
                {
                    printf("R2L_D2U\r\n");

                    sTP_DEV.fXfac = (float)(ILI_DISPLAY_WIDTH - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[1][0] -
                                              XYpoint_Arr[0][0]);
                    sTP_DEV.fYfac = (float)(ILI_DISPLAY_HEIGHT - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[2][1] -
                                              XYpoint_Arr[0][1]);

                    sTP_DEV.iXoff = (ILI_DISPLAY_WIDTH -
                                     sTP_DEV.fXfac * (XYpoint_Arr[1][0] +
                                                      XYpoint_Arr[0][0])) /
                                    2;
                    sTP_DEV.iYoff = (ILI_DISPLAY_HEIGHT -
                                     sTP_DEV.fYfac * (XYpoint_Arr[2][1] +
                                                      XYpoint_Arr[0][1])) /
                                    2;
                }
                else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_PORTRAIT)
                {
                    printf("L2R_U2D\r\n");

                    sTP_DEV.fXfac = (float)(ILI_DISPLAY_WIDTH - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[0][0] -
                                              XYpoint_Arr[1][0]);
                    sTP_DEV.fYfac = (float)(ILI_DISPLAY_HEIGHT - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[0][1] -
                                              XYpoint_Arr[2][1]);

                    sTP_DEV.iXoff = (ILI_DISPLAY_WIDTH -
                                     sTP_DEV.fXfac * (XYpoint_Arr[0][0] +
                                                      XYpoint_Arr[1][0])) /
                                    2;
                    sTP_DEV.iYoff = (ILI_DISPLAY_HEIGHT - sTP_DEV.fYfac *
                                                              (XYpoint_Arr[0][1] + XYpoint_Arr[2][1])) /
                                    2;
                }
                else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_LANDSCAPE_FLIPPED)
                {
                    printf("U2D_R2L\r\n");

                    sTP_DEV.fXfac = (float)(ILI_DISPLAY_WIDTH - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[1][1] - XYpoint_Arr[0][1]);
                    sTP_DEV.fYfac = (float)(ILI_DISPLAY_HEIGHT - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[2][0] - XYpoint_Arr[0][0]);

                    sTP_DEV.iXoff = (ILI_DISPLAY_WIDTH -
                                     sTP_DEV.fXfac * (XYpoint_Arr[1][1] +
                                                      XYpoint_Arr[0][1])) /
                                    2;
                    sTP_DEV.iYoff = (ILI_DISPLAY_HEIGHT -
                                     sTP_DEV.fYfac * (XYpoint_Arr[2][0] +
                                                      XYpoint_Arr[0][0])) /
                                    2;
                }
                else
                {
                    printf("D2U_L2R\r\n");

                    sTP_DEV.fXfac = (float)(ILI_DISPLAY_WIDTH - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[0][1] -
                                              XYpoint_Arr[1][1]);
                    sTP_DEV.fYfac = (float)(ILI_DISPLAY_HEIGHT - 2 * Mar_Val) /
                                    (int16_t)(XYpoint_Arr[0][0] -
                                              XYpoint_Arr[2][0]);

                    sTP_DEV.iXoff = (ILI_DISPLAY_WIDTH -
                                     sTP_DEV.fXfac * (XYpoint_Arr[0][1] +
                                                      XYpoint_Arr[1][1])) /
                                    2;
                    sTP_DEV.iYoff = (ILI_DISPLAY_HEIGHT -
                                     sTP_DEV.fYfac * (XYpoint_Arr[0][0] +
                                                      XYpoint_Arr[2][0])) /
                                    2;
                }
                printf("sTP_DEV.fXfac = %f \r\n", sTP_DEV.fXfac);
                printf("sTP_DEV.fYfac = %f \r\n", sTP_DEV.fYfac);
                printf("sTP_DEV.iXoff = %d \r\n", sTP_DEV.iXoff);
                printf("sTP_DEV.iYoff = %d \r\n", sTP_DEV.iYoff);

                // 6.Calibration is successful
                printf("Adjust OK\r\n");

                screen_set_background(BACKGROUND);
                screen_set_foreground(RED);
                screen_clear();

                // GUI_DisString_EN(20, 110, "Touch Screen Adjust OK!",
                //                  &Font16, FONT_BACKGROUND, RED);

                screen_set_text(2, 11, "Touch Screen Adjust OK!");
                screen_render();

                sleep_ms(1000);

                screen_clear();
                screen_render();
                return;

                // Exception handling,Reset  Initial value
            default:
                cnt = 0;
                TP_DrawCross(ILI_DISPLAY_WIDTH - Mar_Val,
                             ILI_DISPLAY_HEIGHT - Mar_Val, WHITE);
                TP_DrawCross(Mar_Val, Mar_Val, RED);
                // GUI_DisString_EN(40, 26, "TP Need readjust!",
                //                  &Font16, FONT_BACKGROUND, RED);
                screen_set_text(4, 3, "TP Need Readjust!");
                screen_render();
                break;
            }
        }
    }
}

/*******************************************************************************
function:
        Use the default calibration factor
*******************************************************************************/
void TP_GetAdFac(void)
{
    if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_PORTRAIT) // L2R_U2D
    {
        sTP_DEV.fXfac = 0.066626;
        sTP_DEV.fYfac = 0.089779;
        sTP_DEV.iXoff = -20;
        sTP_DEV.iYoff = -34;
    }
    else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_LANDSCAPE) // D2U_L2R
    {
        sTP_DEV.fXfac = -0.089997;
        sTP_DEV.fYfac = 0.067416;
        sTP_DEV.iXoff = 350;
        sTP_DEV.iYoff = -20;
    }
    else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_PORTRAIT_FLIPPED) //  R2L_D2U
    {
        sTP_DEV.fXfac = 0.066339;
        sTP_DEV.fYfac = 0.087059;
        sTP_DEV.iXoff = -13;
        sTP_DEV.iYoff = -26;
    }
    else if (ILI_SELECTED_ORIENTATION == ILI_ORIENTATION_LANDSCAPE_FLIPPED) // U2D_R2L
    {
        sTP_DEV.fXfac = -0.089616;
        sTP_DEV.fYfac = 0.063399;
        sTP_DEV.iXoff = 350;
        sTP_DEV.iYoff = -5;
    }
    else
    {
        screen_set_background(BACKGROUND);
        screen_set_foreground(RED);
        screen_clear();
        screen_set_text(0, 6, "Does not support touch-screen calibration in this direction");
        screen_render();
    }
}

/*******************************************************************************
function:
        Paint the Delete key and paint color choose area
*******************************************************************************/
void TP_Dialog()
{
    screen_set_background(BLUE);
    screen_set_foreground(RED);
    screen_clear();

    // if (SCREEN_COLUMNS > SCREEN_LINES)
    // {
    // Clear screen
    screen_set_text(SCREEN_COLUMNS - 6, 0, "CLEAR");
    // adjustment
    screen_set_text(SCREEN_COLUMNS - 12, 0, "AD");

    // choose the color
    // GUI_DrawRectangle(sLCD_DIS.LCD_Dis_Column - 30, 20,
    //                   sLCD_DIS.LCD_Dis_Column, 50,
    //                   BLUE, DRAW_FULL, DOT_PIXEL_1X1);
    screen_fill_background(SCREEN_COLUMNS - 3, 2, SCREEN_COLUMNS, 5);

    // GUI_DrawRectangle(sLCD_DIS.LCD_Dis_Column - 30, 60,
    //                   sLCD_DIS.LCD_Dis_Column, 90,
    //                   GREEN, DRAW_FULL, DOT_PIXEL_1X1);
    screen_set_background(GREEN);
    screen_fill_background(SCREEN_COLUMNS - 3, 6, SCREEN_COLUMNS, 9);

    // GUI_DrawRectangle(sLCD_DIS.LCD_Dis_Column - 30, 100,
    //                   sLCD_DIS.LCD_Dis_Column, 130,
    //                   RED, DRAW_FULL, DOT_PIXEL_1X1);
    screen_set_background(RED);
    screen_fill_background(SCREEN_COLUMNS - 3, 10, SCREEN_COLUMNS, 13);

    // GUI_DrawRectangle(sLCD_DIS.LCD_Dis_Column - 30, 140,
    //                   sLCD_DIS.LCD_Dis_Column, 170,
    //                   YELLOW, DRAW_FULL, DOT_PIXEL_1X1);
    screen_set_background(YELLOW);
    screen_fill_background(SCREEN_COLUMNS - 3, 14, SCREEN_COLUMNS, 17);

    // GUI_DrawRectangle(sLCD_DIS.LCD_Dis_Column - 30, 180,
    //                   sLCD_DIS.LCD_Dis_Column, 210,
    //                   BLACK, DRAW_FULL, DOT_PIXEL_1X1);
    screen_set_background(BLACK);
    screen_fill_background(SCREEN_COLUMNS - 3, 18, SCREEN_COLUMNS, 21);
    // }
    // else
    // {
    // GUI_DisString_EN(sLCD_DIS.LCD_Dis_Column - 60, 0,
    //                  "CLEAR", &Font16, RED, BLUE);
    // GUI_DisString_EN(sLCD_DIS.LCD_Dis_Column - 120, 0,
    //                  "AD", &Font16, RED, BLUE);
    // GUI_DrawRectangle(0, 0, 15, 15, BLUE, DRAW_FULL, DOT_PIXEL_1X1);
    // GUI_DrawRectangle(20, 0, 35, 15, GREEN, DRAW_FULL, DOT_PIXEL_1X1);
    // GUI_DrawRectangle(40, 0, 55, 15, RED, DRAW_FULL, DOT_PIXEL_1X1);
    // GUI_DrawRectangle(60, 0, 75, 15, YELLOW, DRAW_FULL, DOT_PIXEL_1X1);
    // GUI_DrawRectangle(80, 0, 95, 15, BLACK, DRAW_FULL, DOT_PIXEL_1X1);
    // }
    // }

    screen_render();
}

/*******************************************************************************
function:
        Draw Board
*******************************************************************************/
void TP_DrawBoard()
{
    sTP_DEV.chStatus &= ~(1 << 6);
    TP_Scan(0);
    if (sTP_DEV.chStatus & TP_PRESS_DOWN)
    { // Press the button
      // Horizontal screen
        printf("x:%d  y:%d\r\n", sTP_Draw.Xpoint, sTP_Draw.Ypoint);
        if (ILI_DISPLAY_WIDTH > ILI_DISPLAY_HEIGHT)
        {
            if (sTP_Draw.Xpoint > (ILI_DISPLAY_WIDTH - 60) &&
                sTP_Draw.Ypoint < 16)
            { // Clear Board
                TP_Dialog();
            }
            else if (sTP_Draw.Xpoint > (ILI_DISPLAY_WIDTH - 120) &&
                     sTP_Draw.Xpoint < (ILI_DISPLAY_WIDTH - 80) &&
                     sTP_Draw.Ypoint < 16)
            { // afresh adjustment
                TP_Adjust();
                sTP_Draw.Xpoint = 0;
                sTP_Draw.Ypoint = 0;
                TP_Dialog();
            }
            else if (sTP_Draw.Xpoint > 290 && sTP_Draw.Xpoint < 320 &&
                     sTP_Draw.Ypoint > 17 && sTP_Draw.Ypoint < 47)
            {
                sTP_Draw.Color = BLUE;
            }
            else if (sTP_Draw.Xpoint > 290 && sTP_Draw.Xpoint < 320 &&
                     sTP_Draw.Ypoint > 57 && sTP_Draw.Ypoint < 87)
            {
                sTP_Draw.Color = GREEN;
            }
            else if (sTP_Draw.Xpoint > 290 && sTP_Draw.Xpoint < 320 &&
                     sTP_Draw.Ypoint > 97 && sTP_Draw.Ypoint < 127)
            {
                sTP_Draw.Color = RED;
            }
            else if (sTP_Draw.Xpoint > 290 && sTP_Draw.Xpoint < 320 &&
                     sTP_Draw.Ypoint > 137 && sTP_Draw.Ypoint < 167)
            {
                sTP_Draw.Color = YELLOW;
            }
            else if (sTP_Draw.Xpoint > 290 && sTP_Draw.Xpoint < 320 &&
                     sTP_Draw.Ypoint > 177 && sTP_Draw.Ypoint < 207)
            {
                sTP_Draw.Color = BLACK;
            }
            else
            {
                // GUI_DrawPoint(sTP_Draw.Xpoint, sTP_Draw.Ypoint,
                //               sTP_Draw.Color, DOT_PIXEL_2X2, DOT_FILL_RIGHTUP);
                screen_set_char(sTP_Draw.Xpoint / 10, sTP_Draw.Ypoint / 10, (character_t){.background = BACKGROUND, .foreground = sTP_Draw.Color, .character = '.'});
                screen_render();
            }
        }
        else
        {
            if (sTP_Draw.Xpoint > (ILI_DISPLAY_WIDTH - 60) &&
                sTP_Draw.Ypoint < 16)
            { // Clear Board
                TP_Dialog();
            }
            else if (sTP_Draw.Xpoint > (ILI_DISPLAY_WIDTH - 120) &&
                     sTP_Draw.Xpoint < (ILI_DISPLAY_WIDTH - 80) &&
                     sTP_Draw.Ypoint < 16)
            { // afresh adjustment
                TP_Adjust();
                sTP_Draw.Xpoint = 81;
                sTP_Draw.Ypoint = 10;
                TP_Dialog();
            }
            else if (sTP_Draw.Xpoint > 0 && sTP_Draw.Xpoint < 15 &&
                     sTP_Draw.Ypoint > 0 && sTP_Draw.Ypoint < 15)
            {
                sTP_Draw.Color = BLUE;
            }
            else if (sTP_Draw.Xpoint > 20 && sTP_Draw.Xpoint < 35 &&
                     sTP_Draw.Ypoint > 0 && sTP_Draw.Ypoint < 15)
            {
                sTP_Draw.Color = GREEN;
            }
            else if (sTP_Draw.Xpoint > 40 && sTP_Draw.Xpoint < 55 &&
                     sTP_Draw.Ypoint > 0 && sTP_Draw.Ypoint < 15)
            {
                sTP_Draw.Color = RED;
            }
            else if (sTP_Draw.Xpoint > 60 && sTP_Draw.Xpoint < 75 &&
                     sTP_Draw.Ypoint > 0 && sTP_Draw.Ypoint < 15)
            {
                sTP_Draw.Color = YELLOW;
            }
            else if (sTP_Draw.Xpoint > 80 && sTP_Draw.Xpoint < 95 &&
                     sTP_Draw.Ypoint > 0 && sTP_Draw.Ypoint < 15)
            {
                sTP_Draw.Color = BLACK;
            }
            else
            {
                // GUI_DrawPoint(sTP_Draw.Xpoint, sTP_Draw.Ypoint,
                //               sTP_Draw.Color, DOT_PIXEL_2X2, DOT_FILL_RIGHTUP);
                screen_set_char(sTP_Draw.Xpoint / 10, sTP_Draw.Ypoint / 10, (character_t){.background = BACKGROUND, .foreground = sTP_Draw.Color, .character = '.'});
                screen_render();
            }
        }
    }
}
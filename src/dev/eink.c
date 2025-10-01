#include "eink.h"

#include <pico/printf.h>

const unsigned char LUT_ALL[233] = {
    0x01,
    0x0A,
    0x1B,
    0x0F,
    0x03,
    0x01,
    0x01,
    0x05,
    0x0A,
    0x01,
    0x0A,
    0x01,
    0x01,
    0x01,
    0x05,
    0x08,
    0x03,
    0x02,
    0x04,
    0x01,
    0x01,
    0x01,
    0x04,
    0x04,
    0x02,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x0A,
    0x1B,
    0x0F,
    0x03,
    0x01,
    0x01,
    0x05,
    0x4A,
    0x01,
    0x8A,
    0x01,
    0x01,
    0x01,
    0x05,
    0x48,
    0x03,
    0x82,
    0x84,
    0x01,
    0x01,
    0x01,
    0x84,
    0x84,
    0x82,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x0A,
    0x1B,
    0x8F,
    0x03,
    0x01,
    0x01,
    0x05,
    0x4A,
    0x01,
    0x8A,
    0x01,
    0x01,
    0x01,
    0x05,
    0x48,
    0x83,
    0x82,
    0x04,
    0x01,
    0x01,
    0x01,
    0x04,
    0x04,
    0x02,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x8A,
    0x1B,
    0x8F,
    0x03,
    0x01,
    0x01,
    0x05,
    0x4A,
    0x01,
    0x8A,
    0x01,
    0x01,
    0x01,
    0x05,
    0x48,
    0x83,
    0x02,
    0x04,
    0x01,
    0x01,
    0x01,
    0x04,
    0x04,
    0x02,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x8A,
    0x9B,
    0x8F,
    0x03,
    0x01,
    0x01,
    0x05,
    0x4A,
    0x01,
    0x8A,
    0x01,
    0x01,
    0x01,
    0x05,
    0x48,
    0x03,
    0x42,
    0x04,
    0x01,
    0x01,
    0x01,
    0x04,
    0x04,
    0x42,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x01,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x07,
    0x17,
    0x41,
    0xA8,
    0x32,
    0x30,
};

/******************************************************************************
function :	Software reset
parameter:
******************************************************************************/
static void EPD_4IN2_V2_Reset(void)
{
    gpio_put(EPD_RST_PIN, 1);
    sleep_ms(100);
    gpio_put(EPD_RST_PIN, 0);
    sleep_ms(2);
    gpio_put(EPD_RST_PIN, 1);
    sleep_ms(100);
}

/******************************************************************************
function :	send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_4IN2_V2_SendCommand(uint8_t Reg)
{
    gpio_put(EPD_DC_PIN, 0);
    gpio_put(EPD_CS_PIN, 0);
    spi_write_blocking(SYS_BUS_PRIMARY_INST, &Reg, 1);
    gpio_put(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_4IN2_V2_SendData(uint8_t Data)
{
    gpio_put(EPD_DC_PIN, 1);
    gpio_put(EPD_CS_PIN, 0);
    spi_write_blocking(SYS_BUS_PRIMARY_INST, &Data, 1);
    gpio_put(EPD_CS_PIN, 1);
}

/******************************************************************************
function :	Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_4IN2_V2_ReadBusy(void)
{
    while (gpio_get(EPD_BUSY_PIN) == 1)
    { // LOW: idle, HIGH: busy
        sleep_ms(10);
    }
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void EPD_4IN2_V2_TurnOnDisplay(void)
{
    EPD_4IN2_V2_SendCommand(0x22);
    EPD_4IN2_V2_SendData(0xF7);
    EPD_4IN2_V2_SendCommand(0x20);
    EPD_4IN2_V2_ReadBusy();
}

static void EPD_4IN2_V2_TurnOnDisplay_Fast(void)
{
    EPD_4IN2_V2_SendCommand(0x22);
    EPD_4IN2_V2_SendData(0xC7);
    EPD_4IN2_V2_SendCommand(0x20);
    EPD_4IN2_V2_ReadBusy();
}

static void EPD_4IN2_V2_TurnOnDisplay_Partial(void)
{
    EPD_4IN2_V2_SendCommand(0x22);
    EPD_4IN2_V2_SendData(0xFF);
    EPD_4IN2_V2_SendCommand(0x20);
    EPD_4IN2_V2_ReadBusy();
}

static void EPD_4IN2_V2_TurnOnDisplay_4Gray(void)
{
    EPD_4IN2_V2_SendCommand(0x22);
    EPD_4IN2_V2_SendData(0xCF);
    EPD_4IN2_V2_SendCommand(0x20);
    EPD_4IN2_V2_ReadBusy();
}

/******************************************************************************
function :	Setting the display window
parameter:
******************************************************************************/
static void EPD_4IN2_V2_SetWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    EPD_4IN2_V2_SendCommand(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    EPD_4IN2_V2_SendData((Xstart >> 3) & 0xFF);
    EPD_4IN2_V2_SendData((Xend >> 3) & 0xFF);

    EPD_4IN2_V2_SendCommand(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    EPD_4IN2_V2_SendData(Ystart & 0xFF);
    EPD_4IN2_V2_SendData((Ystart >> 8) & 0xFF);
    EPD_4IN2_V2_SendData(Yend & 0xFF);
    EPD_4IN2_V2_SendData((Yend >> 8) & 0xFF);
}

/******************************************************************************
function :	Set Cursor
parameter:
******************************************************************************/
static void EPD_4IN2_V2_SetCursor(uint16_t Xstart, uint16_t Ystart)
{
    EPD_4IN2_V2_SendCommand(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    EPD_4IN2_V2_SendData((Xstart >> 3) & 0xFF);

    EPD_4IN2_V2_SendCommand(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    EPD_4IN2_V2_SendData(Ystart & 0xFF);
    EPD_4IN2_V2_SendData((Ystart >> 8) & 0xFF);
}

// LUT download
static void EPD_4IN2_V2_4Gray_lut(void)
{
    unsigned char i;

    // WS byte 0~152, the content of VS[nX-LUTm], TP[nX], RP[n], SR[nXY], FR[n] and XON[nXY]
    EPD_4IN2_V2_SendCommand(0x32);
    for (i = 0; i < 227; i++)
    {
        EPD_4IN2_V2_SendData(LUT_ALL[i]);
    }
    // WS byte 153, the content of Option for LUT end
    EPD_4IN2_V2_SendCommand(0x3F);
    EPD_4IN2_V2_SendData(LUT_ALL[i++]);

    // WS byte 154, the content of gate leve
    EPD_4IN2_V2_SendCommand(0x03);
    EPD_4IN2_V2_SendData(LUT_ALL[i++]); // VGH

    // WS byte 155~157, the content of source level
    EPD_4IN2_V2_SendCommand(0x04);
    EPD_4IN2_V2_SendData(LUT_ALL[i++]); // VSH1
    EPD_4IN2_V2_SendData(LUT_ALL[i++]); // VSH2
    EPD_4IN2_V2_SendData(LUT_ALL[i++]); // VSL

    // WS byte 158, the content of VCOM level
    EPD_4IN2_V2_SendCommand(0x2c);
    EPD_4IN2_V2_SendData(LUT_ALL[i++]); // VCOM
}

/******************************************************************************
function :	Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_4IN2_V2_Init(void)
{
    printf("dev: eink screen initilization...\n");

    gpio_init(EPD_RST_PIN);
    gpio_init(EPD_DC_PIN);
    gpio_init(EPD_CS_PIN);
    gpio_init(EPD_BUSY_PIN);

    gpio_set_dir(EPD_RST_PIN, GPIO_OUT);
    gpio_set_dir(EPD_DC_PIN, GPIO_OUT);
    gpio_set_dir(EPD_CS_PIN, GPIO_OUT);
    gpio_set_dir(EPD_BUSY_PIN, GPIO_IN);

    EPD_4IN2_V2_Reset();

    EPD_4IN2_V2_ReadBusy();
    EPD_4IN2_V2_SendCommand(0x12); // soft  reset
    EPD_4IN2_V2_ReadBusy();

    // EPD_4IN2_V2_SendCommand(0x01); //Driver output control
    // EPD_4IN2_V2_SendData((EPD_4IN2_V2_HEIGHT-1)%256);
    // EPD_4IN2_V2_SendData((EPD_4IN2_V2_HEIGHT-1)/256);
    // EPD_4IN2_V2_SendData(0x00);

    EPD_4IN2_V2_SendCommand(0x21); //  Display update control
    EPD_4IN2_V2_SendData(0x40);
    EPD_4IN2_V2_SendData(0x00);

    EPD_4IN2_V2_SendCommand(0x3C); // BorderWavefrom
    EPD_4IN2_V2_SendData(0x05);

    EPD_4IN2_V2_SendCommand(0x11); // data  entry  mode
    EPD_4IN2_V2_SendData(0x03);    // X-mode

    EPD_4IN2_V2_SetWindows(0, 0, EPD_4IN2_V2_WIDTH - 1, EPD_4IN2_V2_HEIGHT - 1);

    EPD_4IN2_V2_SetCursor(0, 0);

    EPD_4IN2_V2_ReadBusy();

    printf("dev: eink screen initialized!\n");
}

/******************************************************************************
function :	Initialize Fast the e-Paper register
parameter:
******************************************************************************/
void EPD_4IN2_V2_Init_Fast(uint8_t Mode)
{
    EPD_4IN2_V2_Reset();

    EPD_4IN2_V2_ReadBusy();
    EPD_4IN2_V2_SendCommand(0x12); // soft  reset
    EPD_4IN2_V2_ReadBusy();

    EPD_4IN2_V2_SendCommand(0x21);
    EPD_4IN2_V2_SendData(0x40);
    EPD_4IN2_V2_SendData(0x00);

    EPD_4IN2_V2_SendCommand(0x3C);
    EPD_4IN2_V2_SendData(0x05);

    if (Mode == Seconds_1_5S)
    {
        // 1.5s
        EPD_4IN2_V2_SendCommand(0x1A); // Write to temperature register
        EPD_4IN2_V2_SendData(0x6E);
    }
    else if (Mode == Seconds_1S)
    {
        // 1s
        EPD_4IN2_V2_SendCommand(0x1A); // Write to temperature register
        EPD_4IN2_V2_SendData(0x5A);
    }

    EPD_4IN2_V2_SendCommand(0x22); // Load temperature value
    EPD_4IN2_V2_SendData(0x91);
    EPD_4IN2_V2_SendCommand(0x20);
    EPD_4IN2_V2_ReadBusy();

    EPD_4IN2_V2_SendCommand(0x11); // data  entry  mode
    EPD_4IN2_V2_SendData(0x03);    // X-mode

    EPD_4IN2_V2_SetWindows(0, 0, EPD_4IN2_V2_WIDTH - 1, EPD_4IN2_V2_HEIGHT - 1);

    EPD_4IN2_V2_SetCursor(0, 0);

    EPD_4IN2_V2_ReadBusy();
}

void EPD_4IN2_V2_Init_4Gray(void)
{
    EPD_4IN2_V2_Reset();

    EPD_4IN2_V2_SendCommand(0x12); // SWRESET
    EPD_4IN2_V2_ReadBusy();

    EPD_4IN2_V2_SendCommand(0x21);
    EPD_4IN2_V2_SendData(0x00);
    EPD_4IN2_V2_SendData(0x00);

    EPD_4IN2_V2_SendCommand(0x3C);
    EPD_4IN2_V2_SendData(0x03);

    EPD_4IN2_V2_SendCommand(0x0C); // BTST
    EPD_4IN2_V2_SendData(0x8B);    // 8B
    EPD_4IN2_V2_SendData(0x9C);    // 9C
    EPD_4IN2_V2_SendData(0xA4);    // 96 A4
    EPD_4IN2_V2_SendData(0x0F);    // 0F

    // EPD_4IN2_V2_SendCommand(0x01);   // 驱动输出控制      drive output control
    // EPD_4IN2_V2_SendData(0x2B); //  Y 的低字节
    // EPD_4IN2_V2_SendData(0x01); //  Y 的高字节
    // EPD_4IN2_V2_SendData(0x00);

    EPD_4IN2_V2_4Gray_lut(); // LUT

    EPD_4IN2_V2_SendCommand(0x11); // data  entry  mode
    EPD_4IN2_V2_SendData(0x03);    // X-mode

    EPD_4IN2_V2_SetWindows(0, 0, EPD_4IN2_V2_WIDTH - 1, EPD_4IN2_V2_HEIGHT - 1);

    EPD_4IN2_V2_SetCursor(0, 0);
}
/******************************************************************************
function :	Clear screen
parameter:
******************************************************************************/
void EPD_4IN2_V2_Clear(void)
{
    uint16_t Width, Height;
    Width = (EPD_4IN2_V2_WIDTH % 8 == 0) ? (EPD_4IN2_V2_WIDTH / 8) : (EPD_4IN2_V2_WIDTH / 8 + 1);
    Height = EPD_4IN2_V2_HEIGHT;

    EPD_4IN2_V2_SendCommand(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            EPD_4IN2_V2_SendData(0xFF);
        }
    }

    EPD_4IN2_V2_SendCommand(0x26);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            EPD_4IN2_V2_SendData(0xFF);
        }
    }
    EPD_4IN2_V2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_4IN2_V2_Display(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_4IN2_V2_WIDTH % 8 == 0) ? (EPD_4IN2_V2_WIDTH / 8) : (EPD_4IN2_V2_WIDTH / 8 + 1);
    Height = EPD_4IN2_V2_HEIGHT;

    EPD_4IN2_V2_SendCommand(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            EPD_4IN2_V2_SendData(Image[i + j * Width]);
        }
    }

    EPD_4IN2_V2_SendCommand(0x26);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            EPD_4IN2_V2_SendData(Image[i + j * Width]);
        }
    }
    EPD_4IN2_V2_TurnOnDisplay();
}

/******************************************************************************
function :	Sends the image buffer in RAM to e-Paper and fast displays
parameter:
******************************************************************************/
void EPD_4IN2_V2_Display_Fast(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_4IN2_V2_WIDTH % 8 == 0) ? (EPD_4IN2_V2_WIDTH / 8) : (EPD_4IN2_V2_WIDTH / 8 + 1);
    Height = EPD_4IN2_V2_HEIGHT;

    EPD_4IN2_V2_SendCommand(0x24);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            EPD_4IN2_V2_SendData(Image[i + j * Width]);
        }
    }

    EPD_4IN2_V2_SendCommand(0x26);
    for (uint16_t j = 0; j < Height; j++)
    {
        for (uint16_t i = 0; i < Width; i++)
        {
            EPD_4IN2_V2_SendData(Image[i + j * Width]);
        }
    }
    EPD_4IN2_V2_TurnOnDisplay_Fast();
}

void EPD_4IN2_V2_Display_4Gray(const uint8_t *Image)
{
    unsigned long i, j, k, m;
    uint8_t temp1, temp2, temp3;
    /****Color display description****
          white  gray2  gray1  black
    0x10|  01     01     00     00
    0x13|  01     00     01     00
    *********************************/
    EPD_4IN2_V2_SendCommand(0x24);
    // EPD_4IN2_HEIGHT
    // EPD_4IN2_WIDTH
    for (m = 0; m < EPD_4IN2_V2_HEIGHT; m++)
        for (i = 0; i < EPD_4IN2_V2_WIDTH / 8; i++)
        {
            temp3 = 0;
            for (j = 0; j < 2; j++)
            {

                temp1 = Image[(m * (EPD_4IN2_V2_WIDTH / 8) + i) * 2 + j];
                for (k = 0; k < 2; k++)
                {
                    temp2 = temp1 & 0xC0;
                    if (temp2 == 0xC0)
                        temp3 |= 0x01; // white
                    else if (temp2 == 0x00)
                        temp3 |= 0x00; // black
                    else if (temp2 == 0x80)
                        temp3 |= 0x00; // gray1
                    else               // 0x40
                        temp3 |= 0x01; // gray2
                    temp3 <<= 1;

                    temp1 <<= 2;
                    temp2 = temp1 & 0xC0;
                    if (temp2 == 0xC0) // white
                        temp3 |= 0x01;
                    else if (temp2 == 0x00) // black
                        temp3 |= 0x00;
                    else if (temp2 == 0x80)
                        temp3 |= 0x00; // gray1
                    else               // 0x40
                        temp3 |= 0x01; // gray2
                    if (j != 1 || k != 1)
                        temp3 <<= 1;

                    temp1 <<= 2;
                }
            }
            EPD_4IN2_V2_SendData(temp3);
        }
    // new  data
    EPD_4IN2_V2_SendCommand(0x26);
    for (m = 0; m < EPD_4IN2_V2_HEIGHT; m++)
        for (i = 0; i < EPD_4IN2_V2_WIDTH / 8; i++)
        {
            temp3 = 0;
            for (j = 0; j < 2; j++)
            {
                temp1 = Image[(m * (EPD_4IN2_V2_WIDTH / 8) + i) * 2 + j];
                for (k = 0; k < 2; k++)
                {
                    temp2 = temp1 & 0xC0;
                    if (temp2 == 0xC0)
                        temp3 |= 0x01; // white
                    else if (temp2 == 0x00)
                        temp3 |= 0x00; // black
                    else if (temp2 == 0x80)
                        temp3 |= 0x01; // gray1
                    else               // 0x40
                        temp3 |= 0x00; // gray2
                    temp3 <<= 1;

                    temp1 <<= 2;
                    temp2 = temp1 & 0xC0;
                    if (temp2 == 0xC0) // white
                        temp3 |= 0x01;
                    else if (temp2 == 0x00) // black
                        temp3 |= 0x00;
                    else if (temp2 == 0x80)
                        temp3 |= 0x01; // gray1
                    else               // 0x40
                        temp3 |= 0x00; // gray2
                    if (j != 1 || k != 1)
                        temp3 <<= 1;

                    temp1 <<= 2;
                }
            }
            EPD_4IN2_V2_SendData(temp3);
        }
    EPD_4IN2_V2_TurnOnDisplay_4Gray();
}

// Send partial data for partial refresh
void EPD_4IN2_V2_PartialDisplay(uint8_t *Image, uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    if ((Xstart % 8 + Xend % 8 == 8 && Xstart % 8 > Xend % 8) || Xstart % 8 + Xend % 8 == 0 || (Xend - Xstart) % 8 == 0)
    {
        Xstart = Xstart / 8;
        Xend = Xend / 8;
    }
    else
    {
        Xstart = Xstart / 8;
        Xend = Xend % 8 == 0 ? Xend / 8 : Xend / 8 + 1;
    }

    uint16_t i, Width;
    Width = Xend - Xstart;
    uint16_t IMAGE_COUNTER = Width * (Yend - Ystart);

    Xend -= 1;
    Yend -= 1;

    EPD_4IN2_V2_SendCommand(0x21);
    EPD_4IN2_V2_SendData(0x00);
    EPD_4IN2_V2_SendData(0x00);

    EPD_4IN2_V2_SendCommand(0x3C);
    EPD_4IN2_V2_SendData(0x80);

    EPD_4IN2_V2_SendCommand(0x11); // data  entry  mode
    EPD_4IN2_V2_SendData(0x03);    // X-mode

    EPD_4IN2_V2_SendCommand(0x44);              // set RAM x address start/end, in page 35
    EPD_4IN2_V2_SendData(Xstart & 0xff);        // RAM x address start at 00h;
    EPD_4IN2_V2_SendData(Xend & 0xff);          // RAM x address end at 0fh(15+1)*8->128
    EPD_4IN2_V2_SendCommand(0x45);              // set RAM y address start/end, in page 35
    EPD_4IN2_V2_SendData(Ystart & 0xff);        // RAM y address start at 0127h;
    EPD_4IN2_V2_SendData((Ystart >> 8) & 0x01); // RAM y address start at 0127h;
    EPD_4IN2_V2_SendData(Yend & 0xff);          // RAM y address end at 00h;
    EPD_4IN2_V2_SendData((Yend >> 8) & 0x01);

    EPD_4IN2_V2_SendCommand(0x4E); // set RAM x address count to 0;
    EPD_4IN2_V2_SendData(Xstart & 0xff);
    EPD_4IN2_V2_SendCommand(0x4F); // set RAM y address count to 0X127;
    EPD_4IN2_V2_SendData(Ystart & 0xff);
    EPD_4IN2_V2_SendData((Ystart >> 8) & 0x01);

    EPD_4IN2_V2_ReadBusy();

    EPD_4IN2_V2_SendCommand(0x24);
    for (uint16_t j = 0; j < IMAGE_COUNTER; j++)
    {
        EPD_4IN2_V2_SendData(Image[j]);
    }

    EPD_4IN2_V2_TurnOnDisplay_Partial();
}
/******************************************************************************
function :	Enter sleep mode
parameter:
******************************************************************************/
void EPD_4IN2_V2_Sleep(void)
{
    EPD_4IN2_V2_SendCommand(0x10); // DEEP_SLEEP
    EPD_4IN2_V2_SendData(0x01);
    sleep_ms(200);
}
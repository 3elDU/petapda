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

typedef enum
{
    INIT_NONE,
    INIT_FULL,
    INIT_FAST,
    INIT_PARTIAL,
    INIT_4GRAY
} init_mode_t;
static init_mode_t cur_init_mode = INIT_NONE;

/** Software reset */
static void eink_reset(void)
{
    gpio_put(EINK_RST_PIN, 1);
    sleep_ms(100);
    gpio_put(EINK_RST_PIN, 0);
    sleep_ms(2);
    gpio_put(EINK_RST_PIN, 1);
    sleep_ms(100);
}

/** Send command to the screen */
static void eink_send_cmd(uint8_t cmd)
{
    gpio_put(EINK_DC_PIN, 0);
    gpio_put(EINK_CS_PIN, 0);
    spi_write_blocking(SYS_BUS_PRIMARY_INST, &cmd, 1);
    gpio_put(EINK_CS_PIN, 1);
}

/** Send a byte to the screen */
static void eink_send_byte(uint8_t data)
{
    gpio_put(EINK_DC_PIN, 1);
    gpio_put(EINK_CS_PIN, 0);
    spi_write_blocking(SYS_BUS_PRIMARY_INST, &data, 1);
    gpio_put(EINK_CS_PIN, 1);
}

static void eink_send_data(const uint8_t *data, size_t len)
{
    gpio_put(EINK_DC_PIN, 1);
    gpio_put(EINK_CS_PIN, 0);
    spi_write_blocking(SYS_BUS_PRIMARY_INST, data, len);
    gpio_put(EINK_CS_PIN, 1);
}

/** Wait until the EINK_BUSY_PIN goes low */
void eink_read_busy(void)
{
    while (gpio_get(EINK_BUSY_PIN) == 1)
    { // LOW: idle, HIGH: busy
        sleep_ms(10);
    }
}

/******************************************************************************
function :	Turn On Display
parameter:
******************************************************************************/
static void eink_turn_on_full(void)
{
    eink_send_cmd(0x22);
    eink_send_byte(0xF7);
    eink_send_cmd(0x20);
    eink_read_busy();
}

static void eink_turn_on_fast(void)
{
    eink_send_cmd(0x22);
    eink_send_byte(0xC7);
    eink_send_cmd(0x20);
    eink_read_busy();
}

static void eink_turn_on_partial(void)
{
    eink_send_cmd(0x22);
    eink_send_byte(0xFF);
    eink_send_cmd(0x20);
    eink_read_busy();
}

static void eink_turn_on_4gray(void)
{
    eink_send_cmd(0x22);
    eink_send_byte(0xCF);
    eink_send_cmd(0x20);
    eink_read_busy();
}

/** Set the display window */
static void eink_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    eink_send_cmd(0x44); // SET_RAM_X_ADDRESS_START_END_POSITION
    eink_send_byte((x0 >> 3) & 0xFF);
    eink_send_byte((x1 >> 3) & 0xFF);

    eink_send_cmd(0x45); // SET_RAM_Y_ADDRESS_START_END_POSITION
    eink_send_byte(y0 & 0xFF);
    eink_send_byte((y0 >> 8) & 0xFF);
    eink_send_byte(y1 & 0xFF);
    eink_send_byte((y1 >> 8) & 0xFF);
}

static void eink_set_cursor(uint16_t x0, uint16_t y0)
{
    eink_send_cmd(0x4E); // SET_RAM_X_ADDRESS_COUNTER
    eink_send_byte((x0 >> 3) & 0xFF);

    eink_send_cmd(0x4F); // SET_RAM_Y_ADDRESS_COUNTER
    eink_send_byte(y0 & 0xFF);
    eink_send_byte((y0 >> 8) & 0xFF);
}

// LUT download
static void eink_4gray_lut()
{
    unsigned char i;

    // WS byte 0~152, the content of VS[nX-LUTm], TP[nX], RP[n], SR[nXY], FR[n] and XON[nXY]
    eink_send_cmd(0x32);
    for (i = 0; i < 227; i++)
    {
        eink_send_byte(LUT_ALL[i]);
    }
    // WS byte 153, the content of Option for LUT end
    eink_send_cmd(0x3F);
    eink_send_byte(LUT_ALL[i++]);

    // WS byte 154, the content of gate leve
    eink_send_cmd(0x03);
    eink_send_byte(LUT_ALL[i++]); // VGH

    // WS byte 155~157, the content of source level
    eink_send_cmd(0x04);
    eink_send_byte(LUT_ALL[i++]); // VSH1
    eink_send_byte(LUT_ALL[i++]); // VSH2
    eink_send_byte(LUT_ALL[i++]); // VSL

    // WS byte 158, the content of VCOM level
    eink_send_cmd(0x2c);
    eink_send_byte(LUT_ALL[i++]); // VCOM
}

/** Initialize the panel */
void eink_init()
{
    printf("dev: eink screen initilization...\n");

    gpio_init(EINK_RST_PIN);
    gpio_init(EINK_DC_PIN);
    gpio_init(EINK_CS_PIN);
    gpio_init(EINK_BUSY_PIN);

    gpio_set_dir(EINK_RST_PIN, GPIO_OUT);
    gpio_set_dir(EINK_DC_PIN, GPIO_OUT);
    gpio_set_dir(EINK_CS_PIN, GPIO_OUT);
    gpio_set_dir(EINK_BUSY_PIN, GPIO_IN);

    eink_reset();

    eink_read_busy();
    eink_send_cmd(0x12); // soft  reset
    eink_read_busy();

    // EPD_4IN2_V2_SendCommand(0x01); //Driver output control
    // EPD_4IN2_V2_SendData((EPD_4IN2_V2_HEIGHT-1)%256);
    // EPD_4IN2_V2_SendData((EPD_4IN2_V2_HEIGHT-1)/256);
    // EPD_4IN2_V2_SendData(0x00);

    eink_send_cmd(0x21); //  Display update control
    eink_send_byte(0x40);
    eink_send_byte(0x00);

    eink_send_cmd(0x3C); // BorderWavefrom
    eink_send_byte(0x05);

    eink_send_cmd(0x11);  // data  entry  mode
    eink_send_byte(0x03); // X-mode

    eink_set_window(0, 0, EINK_WIDTH - 1, EINK_HEIGHT - 1);

    eink_set_cursor(0, 0);

    eink_read_busy();

    cur_init_mode = INIT_FULL;
    printf("dev: eink screen initialized!\n");
}

/** Initialize the screen in fast mode */
void eink_fast_init()
{
    eink_reset();

    eink_read_busy();
    eink_send_cmd(0x12); // soft  reset
    eink_read_busy();

    eink_send_cmd(0x21);
    eink_send_byte(0x40);
    eink_send_byte(0x00);

    eink_send_cmd(0x3C);
    eink_send_byte(0x05);

    eink_send_cmd(0x1A); // Write to temperature register
    eink_send_byte(0x6E);

    eink_send_cmd(0x22); // Load temperature value
    eink_send_byte(0x91);
    eink_send_cmd(0x20);
    eink_read_busy();

    eink_send_cmd(0x11);  // data  entry  mode
    eink_send_byte(0x03); // X-mode

    eink_set_window(0, 0, EINK_WIDTH - 1, EINK_HEIGHT - 1);

    eink_set_cursor(0, 0);

    eink_read_busy();

    cur_init_mode = INIT_FAST;
    printf("dev: eink screen initialized in fast mode\n");
}

void eink_4gray_init(void)
{
    eink_reset();

    eink_send_cmd(0x12); // SWRESET
    eink_read_busy();

    eink_send_cmd(0x21);
    eink_send_byte(0x00);
    eink_send_byte(0x00);

    eink_send_cmd(0x3C);
    eink_send_byte(0x03);

    eink_send_cmd(0x0C);  // BTST
    eink_send_byte(0x8B); // 8B
    eink_send_byte(0x9C); // 9C
    eink_send_byte(0xA4); // 96 A4
    eink_send_byte(0x0F); // 0F

    // EPD_4IN2_V2_SendCommand(0x01);   // 驱动输出控制      drive output control
    // EPD_4IN2_V2_SendData(0x2B); //  Y 的低字节
    // EPD_4IN2_V2_SendData(0x01); //  Y 的高字节
    // EPD_4IN2_V2_SendData(0x00);

    eink_4gray_lut(); // LUT

    eink_send_cmd(0x11);  // data  entry  mode
    eink_send_byte(0x03); // X-mode

    eink_set_window(0, 0, EINK_WIDTH - 1, EINK_HEIGHT - 1);

    eink_set_cursor(0, 0);

    printf("dev: eink screen initialized in 4gray mode\n");
    cur_init_mode = INIT_4GRAY;
}

void eink_partial_init()
{
    eink_send_cmd(0x21);
    eink_send_byte(0x00);
    eink_send_byte(0x00);

    eink_send_cmd(0x3C);
    eink_send_byte(0x80);

    eink_send_cmd(0x11);  // data  entry  mode
    eink_send_byte(0x03); // X-mode

    eink_read_busy();

    cur_init_mode = INIT_PARTIAL;
    printf("dev: eink screen initialized in partial refresh mode\n");
}

void eink_display(const uint8_t *image)
{
    if (cur_init_mode != INIT_FULL)
        eink_init();

    eink_send_cmd(0x24);
    eink_send_data(image, EINK_BUFSIZE);

    eink_send_cmd(0x26);
    eink_send_data(image, EINK_BUFSIZE);

    eink_turn_on_full();
}

void eink_fast_display(const uint8_t *image)
{
    if (cur_init_mode != INIT_FAST)
        eink_fast_init();

    uint16_t Width, Height;
    Width = (EINK_WIDTH % 8 == 0) ? (EINK_WIDTH / 8) : (EINK_WIDTH / 8 + 1);
    Height = EINK_HEIGHT;

    eink_send_cmd(0x24);
    eink_send_data(image, EINK_BUFSIZE);

    eink_send_cmd(0x26);
    eink_send_data(image, EINK_BUFSIZE);

    eink_turn_on_fast();
}

void eink_4gray_display(const uint8_t *Image)
{
    unsigned long i, j, k, m;
    uint8_t temp1, temp2, temp3;
    /****Color display description****
          white  gray2  gray1  black
    0x10|  01     01     00     00
    0x13|  01     00     01     00
    *********************************/
    eink_send_cmd(0x24);
    // EPD_4IN2_HEIGHT
    // EPD_4IN2_WIDTH
    for (m = 0; m < EINK_HEIGHT; m++)
        for (i = 0; i < EINK_WIDTH / 8; i++)
        {
            temp3 = 0;
            for (j = 0; j < 2; j++)
            {

                temp1 = Image[(m * (EINK_WIDTH / 8) + i) * 2 + j];
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
            eink_send_byte(temp3);
        }
    // new  data
    eink_send_cmd(0x26);
    for (m = 0; m < EINK_HEIGHT; m++)
        for (i = 0; i < EINK_WIDTH / 8; i++)
        {
            temp3 = 0;
            for (j = 0; j < 2; j++)
            {
                temp1 = Image[(m * (EINK_WIDTH / 8) + i) * 2 + j];
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
            eink_send_byte(temp3);
        }
    eink_turn_on_4gray();
}

// Send partial data for partial refresh
void eink_partial_display(const uint8_t *image, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (cur_init_mode != INIT_PARTIAL)
        eink_partial_init();

    // Divide and round X0 and X1 to nearest number that multiplies by 8
    x0 = x0 % 8 == 0 ? x0 / 8 : x0 / 8 + 1;
    x1 = x1 % 8 == 0 ? x1 / 8 : x1 / 8 + 1;

    size_t line_length = x1 - x0;

    x1 -= 1;
    y1 -= 1;

    eink_set_window(x0 * 8, y0, x1 * 8, y1);
    eink_set_cursor(x0 * 8, y0);

    eink_read_busy();

    // Send data in lines from y0 to y1

    eink_send_cmd(0x24);
    for (uint16_t y = y0; y < y1; y++)
    {
        eink_send_data(&image[y * EINK_WIDTH / 8 + x0], line_length);
    }

    eink_turn_on_partial();
}

/** Enter sleep mode */
void eink_sleep(void)
{
    eink_send_cmd(0x10); // DEEP_SLEEP
    eink_send_byte(0x01);
    sleep_ms(200);
}
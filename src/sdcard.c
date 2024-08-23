// Sd-card communication

#include "sdcard.h"
#include "debug.h"

#include <stdio.h>
#include "pico/binary_info.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "FreeRTOS.h"
#include "task.h"

static uint8_t CRCTable[256];

// Generate the CRC7 lookup table
void gen_crc_table()
{
  uint32_t i, j;
  uint8_t CRCPoly = 0x89; // the value of our CRC-7 polynomial

  // generate a table value for all 256 possible byte values
  for (i = 0; i < 256; ++i)
  {
    CRCTable[i] = (i & 0x80) ? i ^ CRCPoly : i;
    for (j = 1; j < 8; ++j)
    {
      CRCTable[i] <<= 1;
      if (CRCTable[i] & 0x80)
        CRCTable[i] ^= CRCPoly;
    }
  }
}
// Add part of the message to the CRC code
// Return the CRC value of the previous CRC with the current message byte
uint8_t crc_add(uint8_t crc, uint8_t message_byte)
{
  return CRCTable[(crc << 1) ^ message_byte];
}

// Generate the CRC7 token for a message of "length" bytes
uint8_t get_crc(uint8_t message[], uint32_t length)
{
  uint8_t crc = 0;

  for (uint32_t i = 0; i < length; ++i)
  {
    crc = crc_add(crc, message[i]);
  }

  return crc;
}

void chip_select()
{
  // CS is active low
  gpio_put(SDCARD_CS_PIN, 0);
}
void chip_deselect()
{
  // CS is active low
  gpio_put(SDCARD_CS_PIN, 1);
}

static bool high_baud_rate = false;

int sdcard_exec_cmd(sd_card_command cmd, uint32_t arg, bool is_cmd)
{
  uint8_t CRC = 0xff;
  uint8_t data_out[6] = {
      cmd | 0x40, // 0b01XXXXXX, where XXXXXX is the command
      arg >> 24,  // Argument first byte
      arg >> 16,  // Argument second byte
      arg >> 8,   // Argument third byte
      arg,        // Argument fourth byte
      CRC,
  };
  data_out[5] = (get_crc(data_out, 5) << 1) | 1;

  if (is_cmd)
  {
    // Send an application-specific command (ACMD)
    sdcard_exec_cmd(SD_CMD_APP_CMD, 0, false);
  }

  spi_write_blocking(SDCARD_SPI, data_out, 6);

  // Wait for the card to execute the command
  uint8_t wait_byte;
  spi_read_blocking(SDCARD_SPI, 0xFF, &wait_byte, 1);

  return 0;
}

bool sdcard_read_single_block(uint32_t block, uint8_t *dst)
{
  sdcard_exec_cmd(SD_CMD_READ_SINGLE_BLOCK, block, false);

  R1 r1;
  spi_read_blocking(SDCARD_SPI, 0xFF, (uint8_t *)&r1, 1);

  if (*((uint8_t *)&r1) != 0x00)
  {
    return false;
  }

  // Wait until the SD card responds with 0xFE (block start token)
  uint8_t byte;
  uint32_t cycles = 0;
  do
  {
    spi_read_blocking(SDCARD_SPI, 0xFF, &byte, 1);
    cycles++;
  } while (byte != 0xFE && cycles < 100000);

  if (byte != 0xFE)
  {
    return false;
  }

  spi_read_blocking(SDCARD_SPI, 0xFF, dst, 512);
  return true;
}

void vSdcardTask(void *pvParameters)
{
  gen_crc_table();

  // Wait 2ms for the card to power up
  sleep_ms(2);

  // Initialize SPI
  spi_init(SDCARD_SPI, 400 * 1000); // Start with low frequency, when initializing the sdcard
  gpio_set_function(SDCARD_MISO_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SDCARD_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SDCARD_CLK_PIN, GPIO_FUNC_SPI);
  // Tell picotool about the SPI pins
  bi_decl(bi_3pins_with_func(SDCARD_MISO_PIN, SDCARD_MOSI_PIN, SDCARD_CLK_PIN, GPIO_FUNC_SPI));

  gpio_init(SDCARD_CS_PIN);
  chip_select();
  gpio_set_dir(SDCARD_CS_PIN, GPIO_OUT);
  // Make the CS pin available to picotool
  bi_decl(bi_1pin_with_name(SDCARD_CS_PIN, "SPI CS"));

  printf("SPI Initialized!\n");

begin_init:
  // SD Card initialization
  spi_read_blocking(SDCARD_SPI, 0xFF, NULL, 16); // Send 16 dummy bytes

  sdcard_exec_cmd(SD_CMD_GO_IDLE_STATE, 0x0, false);
  uint8_t response[8];
  int cycles = 0;
  do
  {
    spi_read_blocking(SDCARD_SPI, 0xFF, response, 1);
    cycles++;
  } while (response[0] != 0x01 && cycles < 1000);
  if (cycles >= 1000)
  {
    printf("SD_CMD_GO_IDLE_STATE timed out\n");
    goto begin_init;
  }

  printf("Received 0x01 from SDCard after %d cycles! Proceeding with initialization\n", cycles);

  sdcard_exec_cmd(SD_CMD_SEND_IF_COND, 0x000001AA, false);
  spi_read_blocking(SDCARD_SPI, 0xFF, response, 5);

  if (response[4] == 0xAA)
  {
    printf("SD_CMD_SEND_IF_COND success\n");
  }

  R7 r7;
  sdcard_exec_cmd(SD_CMD_READ_OCR, 0x0, false);
  spi_read_blocking(SDCARD_SPI, 0xFF, (uint8_t *)&r7, 5);

  printf("voltage ranges: 2.7-2.8=%d 2.8-2.9=%d 2.9-3.0=%d 3.0-3.1=%d 3.1-3.2=%d 3.2-3.3=%d 3.3-3.4=%d 3.4-3.5=%d 3.5-3.6=%d\n",
         r7.ocr.voltage_range_27_28, r7.ocr.voltage_range_28_29, r7.ocr.voltage_range_29_30, r7.ocr.voltage_range_30_31,
         r7.ocr.voltage_range_31_32, r7.ocr.voltage_range_32_33, r7.ocr.voltage_range_33_34, r7.ocr.voltage_range_34_35,
         r7.ocr.voltage_range_35_36);

  cycles = 0;
  do
  {
    sdcard_exec_cmd(SD_ACMD_SD_SEND_OP_COND, 0x40000000, true);
    spi_read_blocking(SDCARD_SPI, 0xFF, response, 1);
    cycles++;
    sleep_ms(1);
  } while (response[0] != 0x00 && cycles < 1000);
  if (cycles >= 1000)
  {
    printf("SD_ACMD_SD_SEND_OP_COND timed out\n");
    goto begin_init;
  }

  printf("SP_ACMD_SD_SEND_OP_COND success, in_idle_state=%x\n", response[0]);

  sdcard_exec_cmd(SD_CMD_READ_OCR, 0x0, false);
  spi_read_blocking(SDCARD_SPI, 0xFF, (uint8_t *)&r7, 5);
  printf("in_idle_state=%d\n", r7.r1.in_idle_state);
  printf("card_capacity_status=%d\n", r7.ocr.card_capacity_status);
  if (r7.ocr.card_capacity_status)
  {
    printf("  we have an SDHC/SDXC (2-32GB)/(32GB-2TB) Card\n");
  }
  else
  {
    printf("  we have an SDSC Card (<2GB)\n");
  }
  printf("card_power_up_status=%d\n", r7.ocr.card_power_up_status);

  if (r7.ocr.card_power_up_status)
  {
    printf("SDCard fully initialized!\n");
  }

  // Increase bus speed
  uint actual_baudrate = spi_set_baudrate(SDCARD_SPI, 13 * 1000 * 1000);
  printf("SPI baudrate increased to %u Hz\n", actual_baudrate);

  // Read CID (Card IDentification) register
  sdcard_exec_cmd(SD_CMD_SEND_CID, 0x0, false);
  CID cid;
  spi_read_blocking(SDCARD_SPI, 0xFF, (uint8_t *)&cid, 17);
  printf("CID\n");
  printf("  serial_number=%u\n", cid.serial_number);
  printf("  product_name=%5s\n", cid.product_name);
  printf("  oem_id=%2s\n", cid.oem_id);
  printf("  product_revision=%u\n", cid.revision);
  printf("  manufacturing_date=%0x\n", cid.manufacturing_date);

  // Set block length to 512 bytes
  sdcard_exec_cmd(SD_CMD_SET_BLOCKLEN, 0x200, false);
  R1 r1;
  spi_read_blocking(SDCARD_SPI, 0xFF, (uint8_t *)&r1, sizeof(R1));
  printf("r1 %x\n", r1);

  // Try to read a block
  uint8_t *block = pvPortMalloc(512);
  if (block == NULL)
  {
    printf("Failed to allocate memory for reading a block\n");
    goto end;
  }

  // Read the boot sector
  if (!sdcard_read_single_block(0, block))
  {
    printf("Failed to read boot sector\n");
    goto end;
  }

  if (block[510] != 0x55 || block[511] != 0xAA)
  {
    printf("Invalid boot sector\n");
    goto end;
  }

  printf("Read valid boot sector from an SD card. Can continue with the file system\n");

end:
  vTaskDelete(NULL);
}
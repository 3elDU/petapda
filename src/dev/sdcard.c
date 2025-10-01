// Sd-card communication

#include "sdcard.h"

#include <sys/bus.h>

#include <pico/printf.h>
#include <pico/time.h>
#include <hardware/gpio.h>

static uint8_t CRCTable[256];

bool sdcard_initialized = false;
bool use_block_adressing = true; // If true, use block addressing (1 block - 512 bytes), otherwise use byte addressing

static inline uint32_t sd_transform_address(uint32_t address)
{
  if (use_block_adressing)
    return address;
  else
    return address * SDCARD_BLOCK_SIZE;
}

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

uint16_t crc16_ccitt(const uint8_t *data, size_t length)
{
  uint16_t crc = 0x0000;        // Initial value
  uint16_t polynomial = 0x1021; // Polynomial

  for (size_t i = 0; i < length; i++)
  {
    crc ^= (uint16_t)data[i] << 8; // Apply byte to CRC

    for (uint8_t bit = 0; bit < 8; bit++)
    {
      if (crc & 0x8000)
      {
        crc = (crc << 1) ^ polynomial;
      }
      else
      {
        crc = crc << 1;
      }
    }
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

  chip_select();

  spi_write_blocking(SYS_BUS_PRIMARY_INST, data_out, 6);

  // Wait for the card to execute the command
  uint8_t wait_byte;
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, &wait_byte, 1);

  chip_deselect();

  return 0;
}

// Execute a command and receive an R1 response
// Handles busy state automatically
R1 sdcard_exec_cmd_r1(sd_card_command cmd, uint32_t arg, bool is_cmd)
{
  sdcard_exec_cmd(cmd, arg, is_cmd);

  chip_select();

  R1 r1;
  uint32_t cycles = 0;
  do
  {
    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, (uint8_t *)&r1, 1);
    cycles++;
  } while (*(uint8_t *)&r1 == 0xFF); // Sometimes SD card sends 0xFF's when it's still busy
  if (cycles > 1)
    printf("sdcard_exec_cmd_r1() took %u cycles\n", cycles);

  chip_deselect();

  return r1;
}

bool sdcard_read_single_block(uint32_t block, uint8_t *dst)
{
  block = sd_transform_address(block);
  printf("sdcard_read_single_block(%u, %p)\n", block, dst);

  uint8_t tries = 0;

retry:
  if (tries == SDCARD_TIMEOUT_RETRIES)
  {
    printf("  - too much retries\n");
    return false;
  }

  R1 r1 = sdcard_exec_cmd_r1(SD_CMD_READ_SINGLE_BLOCK, block, false);

  if (!sd_is_good_r1(r1))
  {
    printf("  - invalid r1 response 0x%02x\n", *((uint8_t *)&r1));
    tries++;
    goto retry;
  }

  chip_select();

  // Wait until the SD card responds with 0xFE (block start token)
  uint8_t byte;
  uint32_t cycles = 0;
  do
  {
    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, &byte, 1);
    cycles++;
    if (byte != 0xFF && byte != 0xFE && byte != 0x00)
      printf("  - byte 0x%02X\n", byte);
  } while (byte != 0xFE && cycles < SDCARD_READ_RESPONSE_CYCLES);

  if (byte != 0xFE)
  {
    printf("  - response timeout, try #%d\n", tries + 1, byte);
    tries++;
    goto retry;
  }

  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, dst, SDCARD_BLOCK_SIZE);
  printf("  - success\n");

  chip_deselect();

  return true;
}

bool sdcard_read_multiple_blocks(uint32_t block, uint8_t *dst, uint32_t count)
{
  block = sd_transform_address(block);
  printf("sdcard_read_multiple_blocks(%u, %p, %u)\n", block, dst, count);

  uint8_t tries = 0;

retry:
  if (tries > SDCARD_TIMEOUT_RETRIES)
  {
    printf("  - too much retries\n");
    return false;
  }

  R1 r1 = sdcard_exec_cmd_r1(SD_CMD_READ_MULTIPLE_BLOCK, block, false);

  if (!sd_is_good_r1(r1))
  {
    printf("  - invalid r1 response 0x%02x\n", *((uint8_t *)&r1));
    tries++;
    goto retry;
  }

  chip_select();

  uint32_t blocks_read = 0;
  do
  {
    uint8_t byte;
    uint32_t cycles = 0;

    do
    {
      spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, &byte, 1);
      cycles++;
    } while (byte != 0xFE && cycles < SDCARD_READ_RESPONSE_CYCLES); // Wait for the block start token

    if (byte != 0xFE)
    {
      printf("  - response timeout, try #%d, byte 0x%02x\n", tries + 1, byte);
      tries++;
      goto retry;
    }

    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, dst + blocks_read * SDCARD_BLOCK_SIZE, SDCARD_BLOCK_SIZE);

    uint16_t crc;
    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, (uint8_t *)&crc, 2); // CRC bytes
    uint16_t dst_crc = crc16_ccitt(dst + blocks_read * SDCARD_BLOCK_SIZE, SDCARD_BLOCK_SIZE);
    if (crc != dst_crc)
    {
      printf(" - crc doesn't match, %04x on our side, %04x on card\n", crc, crc16_ccitt(dst + blocks_read * SDCARD_BLOCK_SIZE, SDCARD_BLOCK_SIZE));
      tries++;
      goto retry;
    }

    blocks_read++;
  } while (blocks_read < count);

  chip_deselect();

  sdcard_exec_cmd(SD_CMD_STOP_TRANSMISSION, 0, false);
  printf("  - success, read %d blocks\n", blocks_read);
  return true;
}

bool sdcard_write_single_block(uint32_t block, const uint8_t *src)
{
  block = sd_transform_address(block);
  printf("sdcard_write_single_block(%u, %p)\n", block, src);

  uint8_t tries = 0;

retry:
  if (tries == SDCARD_TIMEOUT_RETRIES)
  {
    printf("  - too much retries\n");
    return false;
  }

  R1 r1 = sdcard_exec_cmd_r1(SD_CMD_WRITE_BLOCK, block, false);

  if (!sd_is_good_r1(r1))
  {
    printf("  - invalid r1 response 0x%02x, in_idle_state=%d\n", *(uint8_t *)&r1, r1.in_idle_state);
    tries++;
    goto retry;
  }

  chip_select();

  // Send start block token 0xFE
  uint8_t response = 0xFE; // Reuse response variable
  spi_write_blocking(SYS_BUS_PRIMARY_INST, &response, 1);

  // Send the data block
  spi_write_blocking(SYS_BUS_PRIMARY_INST, src, SDCARD_BLOCK_SIZE);

  // Send CRC
  uint16_t crc = crc16_ccitt(src, SDCARD_BLOCK_SIZE);
  uint8_t crc_bytes[2] = {(crc >> 8) & 0xFF, crc & 0xFF};
  spi_write_blocking(SYS_BUS_PRIMARY_INST, crc_bytes, 2);

  // Read the data response token
  uint32_t cycles = 0;
  do
  {
    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, &response, 1);
    cycles++;
    if (response != 0xFF && response != 0xE5 && response != 0x00)
      printf("  - byte 0x%02X\n", response);
  } while (response != 0xE5 && cycles < SDCARD_WRITE_RESPONSE_CYCLES);

  chip_deselect();

  if (response == 0xFF)
  {
    printf("  - response timeout, try #%d\n", tries + 1);
    tries++;
    goto retry;
  }

  printf("  - success, response 0x%02x\n", response);
  return true;
}

bool sdcard_status()
{
  return sdcard_initialized;
}

bool sdcard_init()
{
  printf("dev: sdcard initilization...\n");

  gen_crc_table();

  gpio_init(SDCARD_CS_PIN);
  gpio_set_dir(SDCARD_CS_PIN, GPIO_OUT);

begin_init:
  uint8_t tries = 0;
  if (tries > 10)
  {
    printf("Failed to initialize SD card\n");
    return false;
  }

  // SD Card initialization
  chip_select();
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, NULL, 16); // Send 16 dummy bytes
  chip_deselect();

  sdcard_exec_cmd(SD_CMD_GO_IDLE_STATE, 0x0, false);
  chip_select();
  uint8_t response[8];
  int cycles = 0;
  do
  {
    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, response, 1);
    cycles++;
  } while (response[0] != 0x01 && cycles < 1000);
  chip_deselect();
  if (cycles >= 1000)
  {
    printf("SD_CMD_GO_IDLE_STATE timed out\n");
    tries++;
    goto begin_init;
  }

  printf("Received 0x01 from SDCard after %d cycles! Proceeding with initialization\n", cycles);

mid_init:
  sdcard_exec_cmd(SD_CMD_SEND_IF_COND, 0x000001AA, false);

  chip_select();
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, response, 5);
  chip_deselect();

  if (response[4] == 0xAA)
  {
    printf("SD_CMD_SEND_IF_COND success\n");
  }

  R7 r7;
  sdcard_exec_cmd(SD_CMD_READ_OCR, 0x0, false);

  chip_select();
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, (uint8_t *)&r7, 5);
  chip_deselect();

  printf("voltage ranges: 2.7-2.8=%d 2.8-2.9=%d 2.9-3.0=%d 3.0-3.1=%d 3.1-3.2=%d 3.2-3.3=%d 3.3-3.4=%d 3.4-3.5=%d 3.5-3.6=%d\n",
         r7.ocr.voltage_range_27_28, r7.ocr.voltage_range_28_29, r7.ocr.voltage_range_29_30, r7.ocr.voltage_range_30_31,
         r7.ocr.voltage_range_31_32, r7.ocr.voltage_range_32_33, r7.ocr.voltage_range_33_34, r7.ocr.voltage_range_34_35,
         r7.ocr.voltage_range_35_36);

  cycles = 0;
  do
  {
    sdcard_exec_cmd(SD_ACMD_SD_SEND_OP_COND, 0x40000000, true);
    chip_select();
    spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, response, 1);
    chip_deselect();
    cycles++;
    sleep_ms(1);
  } while (response[0] != 0x00 && cycles < 1000);
  if (cycles >= 1000)
  {
    printf("SD_ACMD_SD_SEND_OP_COND timed out\n");
    tries++;
    goto begin_init;
  }

  printf("SP_ACMD_SD_SEND_OP_COND success, in_idle_state=%x\n", response[0]);

  sdcard_exec_cmd(SD_CMD_READ_OCR, 0x0, false);
  chip_select();
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, (uint8_t *)&r7, sizeof(R7));
  chip_deselect();
  printf("in_idle_state=%d\n", r7.r1.in_idle_state);
  printf("card_capacity_status=%d\n", r7.ocr.card_capacity_status);
  use_block_adressing = r7.ocr.card_capacity_status;
  if (r7.ocr.card_capacity_status)
  {
    printf("  we have an SDHC/SDXC (2-32GB)/(32GB-2TB) Card\n");
  }
  else
  {
    printf("  we have an SDSC Card (<2GB)\n");
  }

  printf("card_power_up_status=%d\n", r7.ocr.card_power_up_status);
  printf("sdcard initialized!\n");

  // Read CID (Card IDentification) register
  sdcard_exec_cmd(SD_CMD_SEND_CID, 0x0, false);
  CID cid;
  chip_select();
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, (uint8_t *)&cid, sizeof(CID));
  chip_deselect();
  printf("CID\n");
  printf("  serial_number=%u\n", cid.serial_number);
  printf("  product_name=%5s\n", cid.product_name);
  printf("  oem_id=%2s\n", cid.oem_id);
  printf("  product_revision=%u\n", cid.revision);
  printf("  manufacturing_date=%0x\n", cid.manufacturing_date);

  // Set block length
  sdcard_exec_cmd(SD_CMD_SET_BLOCKLEN, SDCARD_BLOCK_SIZE, false);
  R1 r1;
  chip_select();
  spi_read_blocking(SYS_BUS_PRIMARY_INST, 0xFF, (uint8_t *)&r1, sizeof(R1));
  chip_deselect();
  printf("r1 %x\n", r1);

  sdcard_initialized = true;
  return true;
}
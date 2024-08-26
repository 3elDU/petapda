#ifndef SDCARD_H
#define SDCARD_H

#include <pico.h>
#include "hardware/spi.h"

// Pinout
#define SDCARD_SPI spi_default
#define SDCARD_CS_PIN PICO_DEFAULT_SPI_CSN_PIN
#define SDCARD_CLK_PIN PICO_DEFAULT_SPI_SCK_PIN
#define SDCARD_MOSI_PIN PICO_DEFAULT_SPI_TX_PIN
#define SDCARD_MISO_PIN PICO_DEFAULT_SPI_RX_PIN

#define SDCARD_BLOCK_SIZE 512
// Retry up to 5 times when reading/writing a block and having a timeout when trying to receive
// a response
#define SDCARD_TIMEOUT_RETRIES 5
// How much (maximum) cycles to wait for a response
#define SDCARD_READ_RESPONSE_CYCLES 4192
#define SDCARD_WRITE_RESPONSE_CYCLES 16384

typedef enum
{
  SD_CMD_GO_IDLE_STATE = 0,
  SD_CMD_SEND_OP_COND = 1,
  SD_CMD_SWITCH_FUNC = 6,
  SD_CMD_SEND_IF_COND = 8,
  SD_CMD_SEND_CSD = 9,
  SD_CMD_SEND_CID = 10,
  SD_CMD_STOP_TRANSMISSION = 12,
  SD_CMD_SEND_STATUS = 13,
  SD_CMD_SET_BLOCKLEN = 16,
  SD_CMD_READ_SINGLE_BLOCK = 17,
  SD_CMD_READ_MULTIPLE_BLOCK = 18,
  SD_CMD_WRITE_BLOCK = 24,
  SD_CMD_WRITE_MULIPLE_BLOCK = 25,
  SD_CMD_PROGRAM_CSD = 27,
  SD_CMD_ERASE_WR_BLK_START_ADDR = 32,
  SD_CMD_ERASE_WR_BLK_END_ADDR = 33,
  SD_CMD_ERASE = 38,
  SD_CMD_APP_CMD = 55,
  SD_CMD_GEN_CMD = 56,
  SD_CMD_READ_OCR = 58,
  SD_CMD_CRC_ON_OFF = 59,
  SD_ACMD_SET_BUS_WIDTH = 6,
  SD_ACMD_SD_STATUS = 13,
  SD_ACMD_SEND_NUM_WR_BOCKS = 22,
  SD_ACMD_SET_WR_BLK_ERASE_COUNT = 23,
  SD_ACMD_SD_SEND_OP_COND = 41,
  SD_ACMD_SET_CLR_CARD_DETECT = 42,
  SD_ACMD_SEND_SCR = 51,
} sd_card_command;

typedef struct
{
  bool zero : 1;
  bool param_error : 1;
  bool address_error : 1;
  bool erase_seq_error : 1;
  bool cmd_crc_error : 1;
  bool illegal_cmd : 1;
  bool erase_state : 1;
  bool in_idle_state : 1;
} __packed R1;

typedef struct
{
  uint16_t reserved1 : 15;
  bool voltage_range_27_28 : 1;
  bool voltage_range_28_29 : 1;
  bool voltage_range_29_30 : 1;
  bool voltage_range_30_31 : 1;
  bool voltage_range_31_32 : 1;
  bool voltage_range_32_33 : 1;
  bool voltage_range_33_34 : 1;
  bool voltage_range_34_35 : 1;
  bool voltage_range_35_36 : 1;
  bool switching_to_1_8v_accepted : 1;
  uint8_t reserved2 : 2;
  bool over_2tb_support : 1;
  bool reserved3 : 1;
  bool uhs2_card_status : 1;
  bool card_capacity_status : 1;
  bool card_power_up_status : 1;
} __packed OCR;

// This structure represents the R7 response from the SD card
typedef struct
{
  // This is the R1 response, containing information about errors
  R1 r1;
  OCR ocr;
} __packed R7;

// Card IDentification register
typedef struct
{
  R1 r1;

  bool unused1 : 1; // Always 1
  uint8_t crc7 : 7;
  uint16_t manufacturing_date : 12;
  uint8_t reserved1 : 4;
  uint32_t serial_number : 32;
  uint8_t revision : 8;
  char product_name[5];
  char oem_id[2];
  uint8_t manufacturer_id : 8;
} __packed CID;

typedef struct
{
  bool zero : 1;

} __packed R2;

// Internal utility functions
inline bool sd_is_good_r1(R1 r1)
{
  return *(uint8_t *)&r1 == 0x00;
}
static inline uint32_t sd_transform_address(uint32_t address);

int sdcard_exec_cmd(sd_card_command cmd, uint32_t arg, bool is_cmd);
R1 sdcard_exec_cmd_r1(sd_card_command cmd, uint32_t arg, bool is_cmd);
bool sdcard_read_single_block(uint32_t block, uint8_t *dst);
bool sdcard_read_multiple_blocks(uint32_t block, uint8_t *dst, uint32_t count);
bool sdcard_write_single_block(uint32_t block, const uint8_t *src);
bool sdcard_init();
bool sdcard_status(); // true - initialized

#endif
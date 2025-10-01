#ifndef SYS_BUS_H
#define SYS_BUS_H

#include <hardware/spi.h>

/**
 * System SPI bus definition.
 *
 * Since there are two SPI instances on Raspberry Pi Pico, we are limited
 * to two spi instances at once. But this doesn't limit us to only two devices.
 *
 * By having a separate CS pin for each device we can send/receive data only
 * from that specific device.
 *
 * Since most SPI devices usually fall into one of two categories - slow and
 * fast, first instance is clocked at a higher frequency, and second instance
 * is clocked at a lower one.
 */

#define SYS_BUS_PRIMARY_INST spi0
#define SYS_BUS_PRIMARY_CLK_PIN 18
#define SYS_BUS_PRIMARY_RX_PIN 16
#define SYS_BUS_PRIMARY_TX_PIN 19
#define SYS_BUS_PRIMARY_DUMMY_CS_PIN 14
#define SYS_BUS_PRIMARY_INIT_HZ 40 * 1000  // When initializing, run at lower frequency
#define SYS_BUS_PRIMARY_HZ 8 * 1024 * 1024 // This can be made higher when everything is properly assembled

#define SYS_BUS_SECONDARY_INST spi1
#define SYS_BUS_SECONDARY_CLK_PIN 0
#define SYS_BUS_SECONDARY_RX_PIN 0
#define SYS_BUS_SECONDARY_TX_PIN 0
#define SYS_BUS_SECONDARY_HZ 100 * 1000

void sys_bus_init();
/**
 * Update the primary SPI instance to run at full speed
 */
void sys_bus_go_fullspeed();

#endif
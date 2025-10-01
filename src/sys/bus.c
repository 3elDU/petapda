#include "bus.h"

#include <pico/printf.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>

void sys_bus_init()
{
    gpio_set_function(SYS_BUS_PRIMARY_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SYS_BUS_PRIMARY_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SYS_BUS_PRIMARY_TX_PIN, GPIO_FUNC_SPI);
    // This is a dummy CS pin, actual pin has to be initialized per device
    gpio_set_function(SYS_BUS_PRIMARY_DUMMY_CS_PIN, GPIO_FUNC_SPI);

    uint baud_rate = spi_init(SYS_BUS_PRIMARY_INST, SYS_BUS_PRIMARY_INIT_HZ);
    printf("primary spi init ok: baud rate %u\n", baud_rate);

#if 0
    gpio_set_function(SYS_BUS_SECONDARY_CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SYS_BUS_SECONDARY_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SYS_BUS_SECONDARY_TX_PIN, GPIO_FUNC_SPI);
    // CS pin has to be initialized per device

    baud_rate = spi_init(SYS_BUS_SECONDARY_INST, SYS_BUS_SECONDARY_HZ);
    printf("secondary spi init ok: baud rate %u\n", baud_rate);
#endif
}

void sys_bus_go_fullspeed()
{
    uint baud_rate = spi_set_baudrate(SYS_BUS_PRIMARY_INST, SYS_BUS_PRIMARY_HZ);
    printf("primary spi bus going full speed: %u\n", baud_rate);
}
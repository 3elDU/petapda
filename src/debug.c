#include "debug.h"

#include <pico/stdio.h>
#include <pico/printf.h>

// Prints data in hex format, 8 bytes per line
void debug_hexdump(const void *data, uint32_t len)
{
  int i = 0;
  while (i < len)
  {
    for (int j = 0; j < 8 && i + j < len; j++)
    {
      printf("%02x ", ((uint8_t *)data)[i + j]);
    }
    // Print string representation of bytes
    for (int j = 0; j < 8 && i + j < len; j++)
    {
      printf("%c", ((uint8_t *)data)[i + j] >= 32 && ((uint8_t *)data)[i + j] <= 126 ? ((uint8_t *)data)[i + j] : '.');
    }
    printf("\n");
    i += 8;
  }
}
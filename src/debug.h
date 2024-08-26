#ifndef DEBUG_H
#define DEBUG_H

// Various debug procedures

#include <pico.h>

// Prints data in hex format, 8 bytes per line
void debug_hexdump(const void *data, uint32_t len);

#endif
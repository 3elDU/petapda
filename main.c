#include "pico/stdlib.h"
#include <pico/time.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 750
#endif

// Perform initialisation
int pico_led_init(void) {
  // A device like Pico that uses a GPIO for the LED will define
  // PICO_DEFAULT_LED_PIN so we can use normal GPIO functionality to turn the
  // led on and off
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  return PICO_OK;
}

// Turn the led on or off
void pico_set_led(bool led_on) {
  // Just set the GPIO on or off
  gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}

int main() {
  stdio_init_all();

  int rc = pico_led_init();
  hard_assert(rc == PICO_OK);
  while (true) {
    void *memory = malloc(1024);
    printf("Allocated 1024 bytes of memory at %p\n", memory);

    pico_set_led(true);
    sleep_ms(LED_DELAY_MS);
    pico_set_led(false);
    sleep_ms(LED_DELAY_MS);
  }
}

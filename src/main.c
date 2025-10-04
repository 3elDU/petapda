#include <sys/bus.h>
#include <dev/sdcard.h>
#include <dev/eink.h>
#include <dev/joystick.h>
#include <gfx/screen.h>
#include <lua/screen.h>
#include <lua/joystick.h>

#include <pico/printf.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>

#include <FreeRTOS.h>
#include <task.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "fs/ff.h"
#include "fs/diskio.h"

// Delay between led blinking
#define LED_DELAY_MS 1000

// Priorities of our threads - higher numbers are higher priority
#define MAIN_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)

// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE 12000
#define BLINK_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

void blink_task(__unused void *params)
{
  bool on = true;

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  while (true)
  {
    gpio_put(PICO_DEFAULT_LED_PIN, on);
    on = !on;

    sleep_ms(LED_DELAY_MS);
  }
}

static int sleep(lua_State *L)
{
  uint32_t ms = luaL_checkinteger(L, 1);
  sleep_ms(ms);
  return 0;
}

void main_task(__unused void *pvParameters)
{
  FATFS fs;
  FRESULT fr;

  fr = f_mount(&fs, "SD", 1);
  if (fr != FR_OK)
  {
    printf("failed to mount fs: %d\n", fr);
    goto end;
  }
  else
  {
    printf("filesystem mounted\n");
  }

  lua_State *lua = luaL_newstate();
  if (lua == NULL)
  {
    printf("Failed to initialize lua state\n");
    goto unmount;
  }
  printf("lua state initialized\n");

  // Init only the base and string libraries
  luaL_openlibs(lua);
  screen_lua_loadlib(lua);
  joystick_lua_loadlib(lua);

  lua_pushcfunction(lua, sleep);
  lua_setglobal(lua, "sleep");

  int err = luaL_dofile(lua, "init.lua");
  if (err)
  {
    const char *err = lua_tostring(lua, -1);

    printf("lua error: %s\n", lua_tostring(lua, -1));

    screen_clear();
    screen_set_background(FOREGROUND);
    screen_set_foreground(BACKGROUND);
    screen_fill_background(0, 0, SCREEN_COLUMNS, SCREEN_LINES);

    screen_print_text("--- lua error ---");

    screen_move_cursor(0, 2);
    screen_print_text(err);

    screen_render();

    lua_pop(lua, 1);
  }

close:
  lua_close(lua);
unmount:
  fr = f_unmount("SD");
  printf("filesystem unmount result = %d\n", fr);
end:
  fflush(stdout);
  vTaskDelete(NULL);
}

void vLaunch(void)
{
  xTaskCreate(main_task, "MainThread", MAIN_TASK_STACK_SIZE, NULL,
              MAIN_TASK_PRIORITY, NULL);

  // start the led blinking
  xTaskCreate(blink_task, "BlinkThread", BLINK_TASK_STACK_SIZE, NULL,
              BLINK_TASK_PRIORITY, NULL);

  /* Start the tasks and timer running. */
  vTaskStartScheduler();
}

int main(void)
{
  stdio_init_all();

#if DEBUG_BUILD
  // wait a second for serial monitor on my PC to pick up the connection
  sleep_ms(2500);
#endif

  // Initialize SPI bus and I/O
  sys_bus_init();

  // Initialize devices
  sdcard_init();
  EPD_4IN2_V2_Init();
  EPD_4IN2_V2_Clear();
  joystick_init();

  sys_bus_go_fullspeed();

  screen_init();
  screen_clear();

  EPD_4IN2_V2_Init_Fast(Seconds_1_5S);

  vLaunch();
  return 0;
}

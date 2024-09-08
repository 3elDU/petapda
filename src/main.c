#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
// #include "pico/multicore.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "fs/ff.h"
#include "fs/diskio.h"

#include "7segment/7segment.h"

// Which core to run on if configNUMBER_OF_CORES==1
#ifndef RUN_FREE_RTOS_ON_CORE
#define RUN_FREE_RTOS_ON_CORE 0
#endif

// Whether to flash the led
#ifndef USB_LED
#define USE_LED 1
#endif

// Whether to busy wait in the led thread
#ifndef LED_BUSY_WAIT
#define LED_BUSY_WAIT 0
#endif

// Delay between led blinking
#define LED_DELAY_MS 1000

// Priorities of our threads - higher numbers are higher priority
#define MAIN_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)

// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define BLINK_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#if USE_LED
// Turn led on or off
static void pico_set_led(bool led_on)
{
  gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}

// Initialise led
static void pico_init_led(void)
{
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

void blink_task(__unused void *params)
{
  bool on = true;
  pico_init_led();
  while (true)
  {
    pico_set_led(on);
    on = !on;

#if LED_BUSY_WAIT
    // You shouldn't usually do this. We're just keeping the thread busy,
    // experiment with BLINK_TASK_PRIORITY and LED_BUSY_WAIT to see what happens
    // if BLINK_TASK_PRIORITY is higher than TEST_TASK_PRIORITY main_task won't
    // get any free time to run unless configNUMBER_OF_CORES > 1
    busy_wait_ms(LED_DELAY_MS);
#else
    sleep_ms(LED_DELAY_MS);
#endif
  }
}
#endif // USE_LED

void main_task(__unused void *params)
{
  // start the worker running
  int count = 0;
  while (true)
  {
#if configNUMBER_OF_CORES > 1
    static int last_core_id = -1;
    if (portGET_CORE_ID() != last_core_id)
    {
      last_core_id = portGET_CORE_ID();
      printf("main task is on core %d\n", last_core_id);
    }
#endif
    printf("Hello from main task count=%u\n", count++);
    vTaskDelay(3000);
  }
}

void stats_task(__unused void *params)
{
  while (true)
  {
    printf("---------------\n");
    printf("FreeRTOS stats:\n");

    HeapStats_t heap_stats;
    vPortGetHeapStats(&heap_stats);
    printf("  heap_size=%x\n", configTOTAL_HEAP_SIZE);
    printf("  heap_free=%x\n", heap_stats.xAvailableHeapSpaceInBytes);
    printf("  heap_allocated=%u\n", heap_stats.xNumberOfSuccessfulAllocations);

    TaskStatus_t task_status[configMAX_PRIORITIES];
    unsigned long total_run_time;
    uxTaskGetSystemState(task_status, configMAX_PRIORITIES, &total_run_time);
    printf("  total_run_time=%u\n", total_run_time);
    for (int i = 0; i < uxTaskGetNumberOfTasks(); i++)
    {
      printf("  priority=%u name=%s cputime=%u\n", i, task_status[i].pcTaskName, task_status[i].ulRunTimeCounter);
    }

    printf("---------------\n");
    sleep_ms(30000);
  }
}

// demo function that sets the led with a boolean value
// that should be called from lua
int set_led(lua_State *L)
{
  if (!lua_isboolean(L, 1))
  {
    luaL_error(L, "Expected boolean");
    return 0;
  }
  if (lua_toboolean(L, 1))
  {
    pico_set_led(true);
  }
  else
  {
    pico_set_led(false);
  }

  printf("Led set by lua!\n");
  return 0;
}

int toggle_led(lua_State *L)
{
  bool led_status = gpio_get(PICO_DEFAULT_LED_PIN);
  pico_set_led(!led_status);
  printf("Led toggled by lua!\n");
  return 0;
}

int curtime(lua_State *L)
{
  uint32_t time = to_ms_since_boot(get_absolute_time());
  lua_pushinteger(L, time);
  return 1;
}

void lua_task(__unused void *params)
{
  sleep_ms(3000);

  lua_State *L = luaL_newstate();
  if (L == NULL)
  {
    printf("Failed to initialize lua state\n");
    goto endnolua;
  }
  printf("The lua state is at %p\n", L);

  // Init only the base library, since we're running in an embedded environment
  luaL_requiref(L, "_G", luaopen_base, 1);
  lua_pop(L, 1);

  pico_init_led();
  // explose our function to lua
  lua_register(L, "set_led", set_led);
  lua_register(L, "toggle_led", toggle_led);
  lua_register(L, "curtime", curtime);

  char input[64];
  // REPL
  while (true)
  {
    printf("> ");
    fflush(stdout);
    scanf("%64s", input);
    printf("\n> %s\n", input);

    if (luaL_loadstring(L, input) || lua_pcall(L, 0, 0, 0))
    {
      printf("%s\n", luaL_tolstring(L, -1, NULL));
      lua_pop(L, 1);
    }
    else
    {
      printf("OK\n");
    }
  }

end:
  lua_close(L);
endnolua:
  vTaskDelete(NULL);
}

void test_fs_task(__unused void *pvParameters)
{
  sleep_ms(2500);

  FATFS fs;
  FRESULT fr;

  fr = f_mount(&fs, "SD", 1);
  if (fr != FR_OK)
  {
    printf("Failed to mount fs: %d\n", fr);
    goto end;
  }
  else
  {
    uint32_t free_clusters;
    f_getfree("SD", &free_clusters, NULL);
    float free_mb = free_clusters * FF_MIN_SS * fs.csize / 1048576.0;
    printf("Filesystem mounted, %f free MiB\n", free_mb);
  }

  FIL file;
  UINT bytes_written;
  fr = f_open(&file, "hello-from-pico.txt", FA_WRITE | FA_CREATE_ALWAYS);
  if (fr != FR_OK)
  {
    printf("Failed to open file: %d\n", fr);
    goto unmount;
  }

  const TCHAR string[] = "Hello from Raspberry Pi Pico!\n";
  fr = f_write(&file, string, sizeof(string), &bytes_written);
  if (fr != FR_OK)
  {
    printf("Failed to write to file: %d\n", fr);
    goto close;
  }

  printf("Wrote %u bytes to file\n", bytes_written);
  f_sync(&file);

close:
  f_close(&file);
unmount:
  fr = f_unmount("SD");
  printf("filesystem unmount result = %d\n", fr);
end:
  fflush(stdout);
  vTaskDelete(NULL);
}

void vLaunch(void)
{
  // TaskHandle_t task;
  // xTaskCreate(main_task, "MainThread", MAIN_TASK_STACK_SIZE, NULL,
  //             MAIN_TASK_PRIORITY, &task);

#if USE_LED
  // start the led blinking
  xTaskCreate(blink_task, "BlinkThread", BLINK_TASK_STACK_SIZE, NULL,
              BLINK_TASK_PRIORITY, NULL);
#endif

  // xTaskCreate(stats_task, "StatsThread", configMINIMAL_STACK_SIZE, NULL,
  //             tskIDLE_PRIORITY, NULL);

  xTaskCreate(lua_task, "LuaThread", MAIN_TASK_STACK_SIZE * 8, NULL,
              MAIN_TASK_PRIORITY, NULL);

  xTaskCreate(test_fs_task, "FsThread", MAIN_TASK_STACK_SIZE * 4, NULL,
              MAIN_TASK_PRIORITY + 1, NULL);

  // xTaskCreate(vSevenSegmentDisplayTask, "7SegmentDisplayThread", MAIN_TASK_STACK_SIZE, NULL,
  //             tskIDLE_PRIORITY, NULL);

  /* Start the tasks and timer running. */
  vTaskStartScheduler();
}

int main(void)
{
  stdio_init_all();

  // Wait for serial monitor on my PC to pick up
  sleep_ms(2500);

  vLaunch();
  return 0;
}

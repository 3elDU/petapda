#include <stdio.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"

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
#define LED_DELAY_MS 50

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
  bool on = false;
  printf("blink_task starts\n");
  pico_init_led();
  while (true)
  {
    static int last_core_id = -1;
    if (portGET_CORE_ID() != last_core_id)
    {
      last_core_id = portGET_CORE_ID();
      printf("blink task is on core %d\n", last_core_id);
    }
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

void vLaunch(void)
{
  TaskHandle_t task;
  xTaskCreate(main_task, "MainThread", MAIN_TASK_STACK_SIZE, NULL,
              MAIN_TASK_PRIORITY, &task);

#if USE_LED
  // start the led blinking
  xTaskCreate(blink_task, "BlinkThread", BLINK_TASK_STACK_SIZE, NULL,
              BLINK_TASK_PRIORITY, NULL);
#endif

  xTaskCreate(stats_task, "StatsThread", configMINIMAL_STACK_SIZE, NULL,
              tskIDLE_PRIORITY, NULL);

  /* Start the tasks and timer running. */
  vTaskStartScheduler();
}

int main(void)
{
  stdio_init_all();
  sleep_ms(2000);
  vLaunch();
  return 0;
}

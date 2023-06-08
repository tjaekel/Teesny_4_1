
#include <Arduino.h>

#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include "VCP_UART.h"
#include "picoc.h"

#include "SYS_config.h"

unsigned long INTcount = 0;

static TaskHandle_t xTaskToNotify = NULL;
static volatile int sHandlerRunning = 0;

void GPIO_Interrupt() {
  INTcount++;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if ( ! sHandlerRunning)
  {
    sHandlerRunning = 1;

  /* At this point xTaskToNotify should not be NULL as a transmission was
     in progress. */
  configASSERT( xTaskToNotify != NULL );

  /* Notify the task that the transmission is complete. */
  vTaskNotifyGiveIndexedFromISR( xTaskToNotify, 0, &xHigherPriorityTaskWoken );

  /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
  should be performed to ensure the interrupt returns directly to the highest
  priority task.  The macro used for this purpose is dependent on the port in
  use and may be called portEND_SWITCHING_ISR(). */
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  }
}

unsigned long GPIO_GetINTcounter(void) {
  return INTcount;
}

void GPIO_thread(void *pvParameters) {
  uint32_t ulNotifiedValue;
  //we have to tell who is the receiver of the notifification, here: our own thread
  xTaskToNotify = xTaskGetCurrentTaskHandle();

  while (true) {
    ::xTaskNotifyWaitIndexed( 0,                /* Wait for 0th notification. */
                              0x00,             /* Don't clear any notification bits on entry. */
                              ULONG_MAX,        /* Reset the notification value to 0 on exit. */
                              &ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
                              portMAX_DELAY );  /* Block indefinitely. */

    if ( ! picoc_INThandler()) {
      print_log(UART_OUT, "GPIO INT: %ld\r\n", INTcount);
    }

    sHandlerRunning = 0;
  }
}

void GPIO_setup(void) {
  //configure GPIO pin for HW INT
  pinMode(23, arduino::INPUT_PULLUP);                //enable pull-up
  attachInterrupt(digitalPinToInterrupt(23), GPIO_Interrupt, arduino::FALLING);

  /* ATT: the stack size must be large enough: we call Pico-C when INT was triggered and Pico-C has SetINTHandler(char *) done */
  ::xTaskCreate(GPIO_thread, "GPIO_thread", THREAD_STACK_SIZE_GPIO, nullptr, 5, nullptr);
}

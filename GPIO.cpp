
#include <Arduino.h>

#include <arduino_freertos.h>
#include <avr/pgmspace.h>
#include <climits>

#include "VCP_UART.h"
#include "picoc.h"

#include "SYS_config.h"
#include "GPIO.h"

unsigned long INTCount[2] = {0, 0};
unsigned long INTHandledCount[2] = {0, 0};

static TaskHandle_t xTaskToNotify[2] = {NULL, NULL};
static volatile int sHandlerRunning[2] = {0, 0};

tGPIOcfg GPIOpins[] = {
  {28, "pin 28"},
  {29, "pin 29"},
  {30, "pin 30"},
  {31, "pin 31"},
  {32, "pin 32"},
};

void GPIO_Interrupt1() {
  INTCount[0]++;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if ( ! sHandlerRunning[0])
  {
    sHandlerRunning[0] = 1;

  /* At this point xTaskToNotify should not be NULL as a transmission was
     in progress. */
  configASSERT(xTaskToNotify[0] != NULL );

  /* Notify the task that the transmission is complete. */
  vTaskNotifyGiveIndexedFromISR(xTaskToNotify[0], 0, &xHigherPriorityTaskWoken );

  /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
  should be performed to ensure the interrupt returns directly to the highest
  priority task.  The macro used for this purpose is dependent on the port in
  use and may be called portEND_SWITCHING_ISR(). */
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken );
  }
}

void GPIO_Interrupt2() {
  INTCount[1]++;

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if ( ! sHandlerRunning[1])
  {
    sHandlerRunning[1] = 1;

  /* At this point xTaskToNotify should not be NULL as a transmission was
     in progress. */
  configASSERT(xTaskToNotify[1] != NULL );

  /* Notify the task that the transmission is complete. */
  vTaskNotifyGiveIndexedFromISR(xTaskToNotify[1], 0, &xHigherPriorityTaskWoken );

  /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
  should be performed to ensure the interrupt returns directly to the highest
  priority task.  The macro used for this purpose is dependent on the port in
  use and may be called portEND_SWITCHING_ISR(). */
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  }
}

unsigned long GPIO_GetINTcounter(int num) {
  if (num)
    return INTCount[1];
  else
    return INTCount[0];
}

unsigned long GPIO_GetINTHandledcounter(int num) {
  if (num)
    return INTHandledCount[1];
  else
    return INTHandledCount[0];
}

void GPIO_ClearCounters(int num) {
  if (num) {
    INTCount[1] = 0;
    INTHandledCount[1] = 0;
  }
  else {
    INTCount[0] = 0;
    INTHandledCount[0] = 0;
  }
}

static unsigned long sINTTick[2][2];

void GPIO_thread1(void *pvParameters) {
  uint32_t ulNotifiedValue = 1;
  //we have to tell who is the receiver of the notifification, here: our own thread
  xTaskToNotify[0] = xTaskGetCurrentTaskHandle();

  while (true) {
    ::xTaskNotifyWaitIndexed( 0,                /* Wait for 0th notification. */
                              0x00,             /* Don't clear any notification bits on entry. */
                              ULONG_MAX,        /* Reset the notification value to 0 on exit. */
                              &ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
                              portMAX_DELAY );  /* Block indefinitely. */

    sINTTick[0][0] = sINTTick[0][1];
    sINTTick[0][1] = millis();

    if ( ! picoc_INThandler()) {
      if (gCFGparams.DebugFlags & DBG_VERBOSE)
        print_log(UART_OUT, "INT0: %ld\r\n", INTCount[0]);
      picoc_DefaultINTHandlerC(0);
    }
    else {
      INTHandledCount[0]++;
    }

    sHandlerRunning[0] = 0;
  }
}

void GPIO_thread2(void *pvParameters) {
  uint32_t ulNotifiedValue;
  //we have to tell who is the receiver of the notifification, here: our own thread
  xTaskToNotify[1] = xTaskGetCurrentTaskHandle();

  while (true) {
    ::xTaskNotifyWaitIndexed( 0,                /* Wait for 0th notification. */
                              0x00,             /* Don't clear any notification bits on entry. */
                              ULONG_MAX,        /* Reset the notification value to 0 on exit. */
                              &ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
                              portMAX_DELAY );  /* Block indefinitely. */

    sINTTick[1][0] = sINTTick[1][1];
    sINTTick[1][1] = millis();

    if ( ! picoc_INThandler2()) {
      if (gCFGparams.DebugFlags & DBG_VERBOSE)
        print_log(UART_OUT, "INT1: %ld\r\n", INTCount[1]);
      picoc_DefaultINTHandlerC(1);
    }
    else {
      INTHandledCount[1]++;
    }

    sHandlerRunning[1] = 0;
  }
}

unsigned long GPIO_GetINTFreq(int num) {
  unsigned long div;
  div = sINTTick[num][1] - sINTTick[num][0];
  if (div) {
    return 1000 / div;
  }
  else
    return 1001;
}

void GPIO_configPins(void) {
  size_t i;
  unsigned long mask = 0x1;

  for (i = 0; i < (sizeof(GPIOpins) / sizeof(tGPIOcfg)); i++) {
    if (gCFGparams.GPIOdir & mask) {
      //configure as output
      if (gCFGparams.GPIOod & mask) {
        pinMode(GPIOpins[i].pin, arduino::OUTPUT_OPENDRAIN);
      }
      else {
        pinMode(GPIOpins[i].pin, arduino::OUTPUT);
      }
      //set default output value
      if (gCFGparams.GPIOval & mask) {
        digitalWrite(GPIOpins[i].pin, arduino::HIGH);
      }
      else {
        digitalWrite(GPIOpins[i].pin, arduino::LOW);
      }
    }
    else {
      //configure as input
      pinMode(GPIOpins[i].pin, arduino::INPUT);
    }

    mask <<= 1;
  }
}

void GPIO_config(unsigned long dir, unsigned long od) {
  gCFGparams.GPIOdir = dir;
  gCFGparams.GPIOod = od;

  GPIO_configPins();
}

void GPIO_setup(void) {
  //configure user GPIO pins
  GPIO_configPins();

  //configure RES output signal
  GPIO_setOutValue(2, arduino::HIGH);
  pinMode(2, arduino::OUTPUT);
  //drive high as default
  ////digitalWrite(2, arduino::HIGH);

  //configure GPIO pin for HW INT
  pinMode(23, arduino::INPUT_PULLUP);                //enable pull-up
  attachInterrupt(digitalPinToInterrupt(23), GPIO_Interrupt1, arduino::FALLING);
  pinMode(22, arduino::INPUT_PULLUP);                //enable pull-up
  attachInterrupt(digitalPinToInterrupt(22), GPIO_Interrupt2, arduino::FALLING);

  /* ATT: the stack size must be large enough: we call Pico-C when INT was triggered and Pico-C has SetINTHandler(char *) done */
  ::xTaskCreate(GPIO_thread1, "GPIO_thread1", THREAD_STACK_SIZE_GPIO, nullptr, THREAD_PRIO_GPIO, nullptr);
  ::xTaskCreate(GPIO_thread2, "GPIO_thread2", THREAD_STACK_SIZE_GPIO, nullptr, THREAD_PRIO_GPIO, nullptr);
}

void GPIO_putPins(unsigned long vals) {
  size_t i;
  unsigned long mask = 0x1;

  for (i = 0; i < (sizeof(GPIOpins) / sizeof(tGPIOcfg)); i++) {
    if (vals & mask) {
      //set high
      digitalWrite(GPIOpins[i].pin, arduino::HIGH);
    }
    else {
      digitalWrite(GPIOpins[i].pin, arduino::LOW);
    }

    mask <<= 1;
  }
}

unsigned long GPIOgetPins(void) {
  size_t i;
  unsigned long mask = 0x1;
  unsigned long vals = 0;
  uint8_t val;

  for (i = 0; i < (sizeof(GPIOpins) / sizeof(tGPIOcfg)); i++) {
    val = digitalRead(GPIOpins[i].pin);
    if (val) {
      vals |= mask;
    }

    mask <<= 1;
  }

  return vals;
}

void GPIO_resetPin(unsigned long val) {
  if (val)
    digitalWrite(2, arduino::HIGH);
  else
    digitalWrite(2, arduino::LOW);
}

/* helper function to set GPIO Output register before configuring mode */

void GPIO_setOutValue(uint8_t pin, uint8_t val)
{
	const struct digital_pin_bitband_and_config_table_struct *p;
	uint32_t mask;

	if (pin >= CORE_NUM_DIGITAL) return;
	p = digital_pin_to_info_PGM + pin;
	mask = p->mask;
	// pin is configured for output mode
	if (val) {
		*(p->reg + 0x21) = mask; // set register
	} else {
		*(p->reg + 0x22) = mask; // clear register
	}
}

void GPIO_testSpeed(void) {
#if 0
  /* this is 10x faster! assuming, this code runs on ITCM */
  while (1) {
    GPIO_setOutValue(32, arduino::HIGH);
    GPIO_setOutValue(32, arduino::LOW);
  }
#else
  /* this is 10x slower! assuming the function sits on external flash (and is not cached or running full speed) */
  while (1) {
    digitalWriteFast(32, arduino::HIGH);
    digitalWriteFast(32, arduino::LOW);
  }
#endif
}

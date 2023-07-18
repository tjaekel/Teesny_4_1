
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include "VCP_UART.h"
#include "cmd_dec.h"
#include "CMD_thread.h"

#include "SYS_config.h"

void CMD_loop(void) {
  char* UARTcmd = NULL;

  UARTcmd = VCP_UART_getString();
  if (UARTcmd) {
    CMD_DEC_execute(UARTcmd, UART_OUT);
    VCP_UART_printPrompt();
  }
  else {
    ////vTaskDelay(1);        //already part of UART_getString()
  }
}

void CMD_thread(void *pvParameters) {
  VCP_UART_putString(VERSION_STRING);
  VCP_UART_printPrompt();

  while (1) {
    CMD_loop();
  }
}

FLASHMEM void CMD_setup(void) {
#if 0
  //wait for host terminal connected
  while ( ! Serial) {
    delay(10);
  }
#endif
           
  ::xTaskCreate(CMD_thread, "CMD_thread", THREAD_STACK_SIZE_CMD, nullptr, THREAD_PRIO_CMD, nullptr);
}

/* helper functions */
void CMD_delay(int ms) {
  vTaskDelay(ms);
}

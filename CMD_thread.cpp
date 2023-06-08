
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include "VCP_UART.h"
#include "cmd_dec.h"
#include "CMD_thread.h"

#include "SYS_config.h"

void CMD_loop(void) {
#if 1
  char* UARTcmd = NULL;

  UARTcmd = VCP_UART_getString();
  if (UARTcmd) {
    CMD_DEC_execute(UARTcmd, UART_OUT);
    VCP_UART_printPrompt();
  }
  else {
    vTaskDelay(1);
  }
#else
  Serial.println("*");
#endif
}

void CMD_thread(void *pvParameters) {
  VCP_UART_putString(VERSION_STRING);
  VCP_UART_printPrompt();

  while (1) {
    CMD_loop();
  }
}

void CMD_setup(void) {
  //wait for host terminal connected
  while ( ! Serial) {
    delay(10);
  }
            
  ::xTaskCreate(CMD_thread, "CMD_thread", THREAD_STACK_SIZE_CMD, nullptr, 2, nullptr);
}

/* helper functions */
void CMD_delay(int ms) {
  vTaskDelay(ms);
}

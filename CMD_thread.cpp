
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "CMD_thread.h"
#include <TeensyThreads.h>

void CMD_loop(void) {
#if 1
  char* UARTcmd = NULL;

  UARTcmd = VCP_UART_getString();
  if (UARTcmd) {
    CMD_DEC_execute(UARTcmd, UART_OUT);
    VCP_UART_printPrompt();
  }
  else {
    threads.delay(1);
  }
#else
  Serial.println("*");
#endif
}

void CMD_thread(void) {
  VCP_UART_putString("---- Teensy FW: V1.0.0 ----");
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
            
  threads.addThread(CMD_thread, 0, THREAD_STACK_SIZE);
}

/* helper functions */
void CMD_delay(int ms) {
  threads.delay(ms);
  ////threads.yield();
}

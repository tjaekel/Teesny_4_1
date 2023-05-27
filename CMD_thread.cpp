
#include "VCP_UART.h"
#include "cmd_dec.h"

void CMD_Thread(void) {
  char* UARTcmd = NULL;

  UARTcmd = VCP_UART_getString();
  if (UARTcmd) {
    CMD_DEC_execute(UARTcmd, UART_OUT);
    VCP_UART_printPrompt();
  }
}

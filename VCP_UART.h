// VCP_UART.h

#ifndef _VCP_UART_h
#define _VCP_UART_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void VCP_UART_setup(void);
char* VCP_UART_getString(void);
void VCP_UART_putString(const char* s);
void VCP_UART_printPrompt(void);
void VCP_UART_hexDump(unsigned char* b, int len);

#define VCP_UART_printf(...)		Serial.printf(__VA_ARGS__)

#endif


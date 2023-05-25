// VCP_UART.h

#ifndef _VCP_UART_h
#define _VCP_UART_h

//#if defined(ARDUINO) && ARDUINO >= 100
#include <arduino.h>
//#else
//	#include "WProgram.h"
//#endif

#define XPRINT_LEN			(80*64)			/* max. length print buffer/strings/lines */

typedef enum {
	UART_OUT,
	HTTPD_OUT,
	HTTPD_OUT_ONLY,
	SILENT
} EResultOut;

extern char XPrintBuf[XPRINT_LEN];

#define print_log(out, ...)		do {\
									if (out != SILENT) {\
										snprintf(XPrintBuf, XPRINT_LEN - 1, __VA_ARGS__);\
										VCP_UART_putString((const char *)XPrintBuf);\
									}\
								} while(0)

#define debug_log(...)			do {\
									if (GDebug & DBG_VERBOSE) {\
										snprintf(XPrintBuf, XPRINT_LEN - 1, __VA_ARGS__);\
										VCP_UART_putString((const char *)XPrintBuf);\
									}\
								} while(0)

void VCP_UART_setup(void);
char* VCP_UART_getString(void);
void VCP_UART_putString(const char* s);
void VCP_UART_printPrompt(void);
void VCP_UART_hexDump(unsigned char* b, int len);

#endif


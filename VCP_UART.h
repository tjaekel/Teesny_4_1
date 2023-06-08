// VCP_UART.h

#ifndef _VCP_UART_h
#define _VCP_UART_h

//#if defined(ARDUINO) && ARDUINO >= 100
#include <arduino.h>
//#else
//	#include "WProgram.h"
//#endif

#define XPRINT_LEN			(80*64)			/* max. length print buffer/strings/lines */
#define HTTPD_PRINT_LEN (4*1024)    /* max. length print for HTTP */

typedef enum {
	UART_OUT,
	HTTPD_OUT,
	HTTPD_OUT_ONLY,
	SILENT
} EResultOut;

extern char XPrintBuf[XPRINT_LEN];

void VCP_UART_setup(void);
char* VCP_UART_getString(void);
void VCP_UART_putString(const char* s);
void VCP_UART_printPrompt(void);
void VCP_UART_hexDump(unsigned char* b, int len);

/* interfaces for Pico-C */
#ifdef __cplusplus
extern "C" {
#endif
void UART_printString(const char *buf, EResultOut out);
void UART_printChar(char c, EResultOut out);

int UART_getString(unsigned char *b, size_t len);
int UART_getChar(void);
int UART_getCharNW(void);
void UART_putChar(unsigned char c);
#ifdef __cplusplus
}
#endif

void HTTP_PutEOT(void);
int HTTP_GetOut(char **b);
void HTTP_ClearOut(void);

#define print_log(out, ...)		do {\
									if (out != SILENT) {\
										snprintf(XPrintBuf, XPRINT_LEN - 1, __VA_ARGS__);\
										UART_printString((char *)XPrintBuf, out);\
									}\
								} while(0)

#define debug_log(...)			do {\
									if (GDebug & DBG_VERBOSE) {\
										snprintf(XPrintBuf, XPRINT_LEN - 1, __VA_ARGS__);\
										VCP_UART_putString((const char *)XPrintBuf);\
									}\
								} while(0)

#endif


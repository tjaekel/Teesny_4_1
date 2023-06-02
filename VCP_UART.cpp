
#include "VCP_UART.h"
#include <TeensyThreads.h>

/* marked as experimental but we need */
ThreadWrap(Serial, SerialXtra);
#define Serial ThreadClone(SerialXtra)

static char UARTbuffer[80];
static size_t numAvail = 0;

char XPrintBuf[XPRINT_LEN];
char HTTPDPrintBuf[HTTPD_PRINT_LEN];

static int HTTPDOutIdx = 0;

void VCP_UART_setup(void)
{
	////Serial.begin(1843200);                        //baudrate does not matter: VCP UART via USB
}

char* VCP_UART_getString(void)
{
    int incomingByte;

    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        
        if (incomingByte == 0x08) {
            if (numAvail) {
                Serial.write(0x08);
                Serial.write(' ');
                Serial.write(0x08);
                numAvail--;
            }
            return NULL;
        }

        if (incomingByte == '\r')
        {
            if (numAvail) {
                Serial.write('\r');
                Serial.write('\n');
            }
            UARTbuffer[numAvail] = '\0';
            numAvail = 0;
            return UARTbuffer;
        }

        UARTbuffer[numAvail++] = (char)incomingByte;
        //local echo
        Serial.write(incomingByte);

        if (numAvail > (sizeof(UARTbuffer) - 2))
        {
            Serial.write('\n');
            UARTbuffer[numAvail] = '\0';
            numAvail = 0;

            Serial.write('\r');
            Serial.write('\n');
            return UARTbuffer;
        }
    }

    threads.delay(1);
    ////threads.yield();
    return NULL;
}

void VCP_UART_putString(const char* s)
{
    Serial.print(s);
}

void VCP_UART_printPrompt(void) {
    Serial.print("\r\n> ");
}

void VCP_UART_hexDump(unsigned char* b, int len) {
    while (len--) {
        Serial.print(*b++, HEX);
        Serial.print(" ");
    }
}

void UART_printString(const char *s, EResultOut out) {
  if (out == UART_OUT) {
    while (*s)
      Serial.print(*s++);

    return;
  }
  if (out == HTTPD_OUT) {
    int l;
    l = strlen(s);
    if ((HTTPDOutIdx + l) >= (int)(sizeof(HTTPDPrintBuf) - 1))
      l = sizeof(HTTPDPrintBuf) - HTTPDOutIdx - 1;
    if (l)
      strncat(HTTPDPrintBuf, s, l);
    HTTPDOutIdx += l;
  }
}

void UART_printChar(char c, EResultOut out) {
  if (out == UART_OUT) {
      Serial.write(c);

    return;
  }
  if (out == HTTPD_OUT) {
    if ((HTTPDOutIdx + 1) >= (int)(sizeof(HTTPDPrintBuf) - 1))
      return;
    
    HTTPDPrintBuf[HTTPDOutIdx++] = c;
    HTTPDPrintBuf[HTTPDOutIdx] = '\0';
  }
}

int HTTP_GetOut(char **b) {
  *b = HTTPDPrintBuf;
  return HTTPDOutIdx;
}

void HTTP_ClearOut(void) {
  HTTPDOutIdx = 0;
  HTTPDPrintBuf[0] = '\0';
}

int UART_getString(unsigned char *b, size_t l)
{
    int incomingByte;
    int x = 0;

    if (Serial.available() > 0) {
        incomingByte = Serial.read();
        
        if (incomingByte == 0x08) {
            if (numAvail) {
                Serial.write(0x08);
                Serial.write(' ');
                Serial.write(0x08);
                numAvail--;
            }
            return 0;
        }

        if (incomingByte == '\r')
        {
            if (numAvail) {
                Serial.write('\r');
                Serial.write('\n');
            }
            b[numAvail++] = '\r';
            b[numAvail] = '\0';
            x = numAvail;
            numAvail = 0;
            return x;
        }

        b[numAvail++] = (unsigned char)incomingByte;
        //local echo
        Serial.write(incomingByte);

        if (numAvail > (l- 2))
        {
            Serial.write('\n');
            b[numAvail] = '\0';
            x = numAvail;
            numAvail = 0;

            Serial.write('\r');
            Serial.write('\n');
            return x;
        }
    }

    threads.delay(1);
    ////threads.yield();
    return 0;
}

int UART_getChar(void) {
  while (1) {
    if (Serial.available() > 0) {
      return Serial.read();
    }
    threads.delay(1);
    ////threads.yield();
  }
}

void UART_putChar(unsigned char c) {
  Serial.write(c);
  if (c == '\n')
    Serial.write('\r');
}

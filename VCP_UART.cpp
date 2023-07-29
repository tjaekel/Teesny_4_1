
#include "VCP_UART.h"
#include "cmd_dec.h"
#include <arduino_freertos.h>
#include <avr/pgmspace.h>

/* TODO:
 * ad a semaphore/mutex around Serial.print
 */

static char UARTbuffer[LINE_BUFFER_LEN];
static size_t numAvail = 0;

char XPrintBuf[XPRINT_LEN];
char HTTPDPrintBuf[HTTPD_PRINT_LEN];

static int HTTPDOutIdx = 0;

static char lastUARTStr[LINE_BUFFER_LEN] = "";

FLASHMEM void VCP_UART_setup(void)
{
	////Serial.begin(1843200);                        //baudrate does not matter: VCP UART via USB
}

FLASHMEM char* VCP_UART_getString(void)
{
    int incomingByte;
    static int available = 0;
    static int ignoreNext = 0;

    if ( ! available)
      available = Serial.available();

    if (available > 0) {
        incomingByte = Serial.read();
        available--;
        
        if (ignoreNext) {
          ignoreNext--;
          if (ignoreNext == 0) {
            if (incomingByte == 'A')
            {
              //arrow up
              if (numAvail == 0) {
                //do just on empty command line - otherwise we had to remove all first
                numAvail = strlen(lastUARTStr);
                VCP_UART_putString(lastUARTStr);
                strcpy(UARTbuffer, lastUARTStr);
              }
              return NULL;
            }
            if (incomingByte == 'B') {
              //arrow down
            }
          }
          return NULL;
        }

        if (incomingByte == 0x1B) {
          //ESC [ A, or ESC [ B
          ignoreNext = 2;
          return NULL;
        }

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
            if (numAvail)
              //keep a real input, not an empty line
              strcpy(lastUARTStr, UARTbuffer);
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
            strcpy(lastUARTStr, UARTbuffer);
            return UARTbuffer;
        }
    }
    else {
      ////vTaskDelay(1);
      taskYIELD();
    }

    return NULL;
}

void VCP_UART_putString(const char* s)
{
    Serial.print(s);
}

void VCP_UART_printPrompt(void) {
    Serial.print("\r\n> \003");               //with EOT for easier parsing
}

void VCP_UART_hexDump(unsigned char* b, int len) {
    while (len--) {
        Serial.print(*b++, arduino::HEX);
        Serial.print(" ");
    }
}

void UART_printString(const char *s, EResultOut out) {
  if (out == UART_OUT) {
    Serial.print(s);

    return;
  }
  if (out == HTTPD_OUT) {
    int l;
    l = strlen(s);
    if ((HTTPDOutIdx + l) >= (int)(sizeof(HTTPDPrintBuf) - 2))
      l = sizeof(HTTPDPrintBuf) - HTTPDOutIdx - 2;
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
    if ((HTTPDOutIdx + 1) >= (int)(sizeof(HTTPDPrintBuf) - 2))
      return;
    
    HTTPDPrintBuf[HTTPDOutIdx++] = c;
    HTTPDPrintBuf[HTTPDOutIdx] = '\0';
  }
}

void HTTP_PutEOT(void) {
  HTTPDPrintBuf[HTTPDOutIdx++] = 0x03;          //EOT
  HTTPDPrintBuf[HTTPDOutIdx] = '\0';
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
    else {
      ////vTaskDelay(1);
      taskYIELD();
    }

    return 0;
}

int UART_getChar(void) {
  while (1) {
    if (Serial.available() > 0) {
      return Serial.read();
    }

    ////vTaskDelay(1);
    taskYIELD();
  }
}

int UART_getCharNW(void) {
  if (Serial.available() > 0) {
    return Serial.read();
  }

  return 0;
}

void UART_putChar(unsigned char c) {
  Serial.write(c);
  if (c == '\n')
    Serial.write('\r');
}


#include "VCP_UART.h"

static char UARTbuffer[80];
static size_t numAvail = 0;

char XPrintBuf[XPRINT_LEN];

void VCP_UART_setup(void)
{
	Serial.begin(1843200);                        //baudrate does not matter: VCP UART via USB
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
  while (*s)
    Serial.print(*s++);
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

    return 0;
}

int UART_getChar(void) {
  while (1) {
    if (Serial.available() > 0) {
      return Serial.read();
    }
    delay(10);
  }
}

void UART_putChar(unsigned char c) {
  Serial.write(c);
  if (c == '\n')
    Serial.write('\r');
}

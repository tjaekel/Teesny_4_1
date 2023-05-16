// 
// 
// 

#include "VCP_UART.h"

static char UARTbuffer[80];
static size_t numAvail = 0;

void VCP_UART_setup(void)
{
	////Serial.begin(9600);     //it is already initialized
    //while (!Serial);          // wait, until host terminal avaialble
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

/*
 Name:		Teensy_SPIder.ino
 Created:	5/5/2023 12:21:04 PM
 Author:	tj
*/

#include "SPI_dev.h"
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "HTTP_server.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "TFTP.h"
#include "MEM_Pool.h"

char* UARTcmd = NULL;

#if 0
//LED cannot be used anymore if we use SPI (SCK becomes the same pin)
const int ledPin = LED_BUILTIN;  // the number of the LED pin
int ledState = LOW;  // ledState used to set the LED
unsigned long previousMillis = 0;   // will store last time LED was updated
const long interval = 200;          // interval at which to blink (milliseconds)
#endif

/* test external PSRAM */
#if 0
EXTMEM unsigned char psiRAM[40];
extern "C" uint8_t external_psram_size;
#endif

////unsigned char SPIbufTx[20];
////unsigned char SPIbufRx[20];

void setup() {
    // set the digital pin as output:
    ////pinMode(ledPin, OUTPUT);

    Serial.begin(115200);       //baudrate does not matter: it is VCP UART, any baudrate works
#if 1
    while (!Serial) {}
    delay(200);
#endif

    CFG_Read();

    ////TFTP_setup();

    VCP_UART_setup();

    HTTPD_setup();

    SPI_setup();

    MEM_PoolInit();

#if 0
    memset(psiRAM, 0x56, sizeof(psiRAM));
#endif
}

void loop() {
    static int firstLoop = 0;
#if 0
    unsigned long currentMillis = millis();
#endif

    if ( ! firstLoop) {
        if (Serial) {
            //wait for host terminal connected
            VCP_UART_putString("---- Teensy FW: V1.0.0 ----");
            VCP_UART_printPrompt();
            firstLoop = 1;
        }
    }

    UARTcmd = VCP_UART_getString();
    if (UARTcmd) {
#if 0
        //PSRAM test
        {
            VCP_UART_printf("\r\nEXTMEM Memory Test, %d Mbyte\r\n", external_psram_size);
            if (external_psram_size) {
                char t[80];
                sprintf(t, "\r\nPSRM: 0x%08lx\r\n", (unsigned long)psiRAM);
                VCP_UART_putString(t);
                VCP_UART_hexDump(psiRAM, sizeof(psiRAM));

                unsigned char* p = (unsigned char *)(0x70000000 + external_psram_size * 1048576 - 16);
                memset(p, 0x76, 16);
                sprintf(t, "\r\nPSRM: 0x%08lx\r\n", (unsigned long)p);
                VCP_UART_putString(t);
                VCP_UART_hexDump(p, 16);
                VCP_UART_putString("\r\n");
            }
        }
#else
      CMD_DEC_execute(UARTcmd, UART_OUT);
      VCP_UART_printPrompt();
#endif
    }

#if 0
    if (currentMillis - previousMillis >= interval) {
        // save the last time you blinked the LED
        previousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if (ledState == LOW) {
            ledState = HIGH;
        }
        else {
            ledState = LOW;
        }

        // set the LED with the ledState of the variable:
        digitalWrite(ledPin, ledState);
    }
#endif
}

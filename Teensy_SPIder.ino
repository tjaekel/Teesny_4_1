/*
 Name:		Teensy_SPIder.ino
 Created:	5/5/2023 12:21:04 PM
 Updated: 5/31/2023
 Author:	tj (tjaekel, Torsten Jaekel)
*/

#include "SPI_dev.h"
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "HTTP_server.h"
#include "TCP_Server.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "TFTP.h"
#include "MEM_Pool.h"
#include "CMD_thread.h"

/* use LED as DEBUG to see if loop() is called and still running */
//#define LED_DEBUG           /* instead of SPI - toggle LED, SPI unusable! LED conflicts with SPI SCLK */

/* test external PSRAM */
#if 0
EXTMEM unsigned char psiRAM[40];
extern "C" uint8_t external_psram_size;
#endif

uint8_t ledPin = 13;

void setup(void) {
#ifdef LED_DEBUG
    // set the digital pin as output: conflicts with SPI SCLK
    pinMode(ledPin, OUTPUT);
#endif

    Serial.begin(115200);       //baudrate does not matter: it is VCP UART, any baudrate works

#if 1
    //optional: wait for UART available - to see all early messages
    while (!Serial) {}

    if (CrashReport)
      Serial.print(CrashReport);
#endif

    CFG_Read();

    VCP_UART_setup();

    HTTPD_setup();
    TCP_Server_setup();

#ifndef LED_DEBUG
    SPI_setup();
#endif

    MEM_PoolInit();

#if 0
    memset(psiRAM, 0x56, sizeof(psiRAM));
#endif

    CMD_setup();              //launch CMD interpreter as a thread
}

void loop() {

#ifdef LED_DEBUG
    #define LED_INTERVAL 1000
    static unsigned long currentMillis = millis();
    static unsigned long previousMillis = millis();
    static int ledState = LOW;
#endif

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
#endif

#ifdef LED_DEBUG
    currentMillis = millis();
    if (currentMillis - previousMillis >= LED_INTERVAL) {
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
#else
    delay(1000);
    ////TCP_Server_loop();
#endif
}

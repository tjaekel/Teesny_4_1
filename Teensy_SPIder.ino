/*
 Name:		Teensy_SPIder.ino
 Created:	5/5/2023 12:21:04 PM
 Author:	tj (tjaekel, Torsten Jaekel)
*/

#include "SPI_dev.h"
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "HTTP_server.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "TFTP.h"
#include "MEM_Pool.h"
#include "CMD_thread.h"

/* test external PSRAM */
#if 0
EXTMEM unsigned char psiRAM[40];
extern "C" uint8_t external_psram_size;
#endif

void setup(void) {
    // set the digital pin as output:
    ////pinMode(ledPin, OUTPUT);

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

    //do it once here, after setup() was completed, otherwise too early
    if ( ! firstLoop) {
        if (Serial) {
            //wait for host terminal connected
            VCP_UART_putString("---- Teensy FW: V1.0.0 ----");
            VCP_UART_printPrompt();
            firstLoop = 1;
        }
    }

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
      CMD_Thread();
#endif

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

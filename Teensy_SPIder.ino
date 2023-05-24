/*
 Name:		Teensy_SPIder.ino
 Created:	5/5/2023 12:21:04 PM
 Author:	tj
*/

#ifdef SD
#include <SD.h>

// change this to match your SD shield or module;
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20
// Wiz820+SD board: pin 4
// Teensy audio board: pin 10
// Teensy 3.5 & 3.6 & 4.1 on-board: BUILTIN_SDCARD
const int chipSelect = BUILTIN_SDCARD;  //10;

void setup()
{
  //Uncomment these lines for Teensy 3.x Audio Shield (Rev C)
  //SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
  //SPI.setSCK(14);  // Audio shield has SCK on pin 14  
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect.
  }

  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  File root = SD.open("/");
  
  printDirectory(root, 0);
  
  Serial.println("done!");
}

void loop()
{
  // nothing happens after setup finishes.
}

void printDirectory(File dir, int numSpaces) {
   while(true) {
     File entry = dir.openNextFile();
     if (! entry) {
       //Serial.println("** no more files **");
       break;
     }
     printSpaces(numSpaces);
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numSpaces+2);
     } else {
       // files have sizes, directories do not
       int n = log10f(entry.size());
       if (n < 0) n = 10;
       if (n > 10) n = 10;
       printSpaces(50 - numSpaces - strlen(entry.name()) - n);
       Serial.print("  ");
       Serial.print(entry.size(), DEC);
       DateTimeFields datetime;
       if (entry.getModifyTime(datetime)) {
         printSpaces(4);
         printTime(datetime);
       }
       Serial.println();
     }
     entry.close();
   }
}

void printSpaces(int num) {
  for (int i=0; i < num; i++) {
    Serial.print(" ");
  }
}

void printTime(const DateTimeFields tm) {
  const char *months[12] = {
    "January","February","March","April","May","June",
    "July","August","September","October","November","December"
  };
  if (tm.hour < 10) Serial.print('0');
  Serial.print(tm.hour);
  Serial.print(':');
  if (tm.min < 10) Serial.print('0');
  Serial.print(tm.min);
  Serial.print("  ");
  Serial.print(tm.mon < 12 ? months[tm.mon] : "???");
  Serial.print(" ");
  Serial.print(tm.mday);
  Serial.print(", ");
  Serial.print(tm.year + 1900);
}
#endif

#ifndef MAIN
#include "SPI_dev.h"
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "HTTP_server.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "TFTP.h"

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
#endif

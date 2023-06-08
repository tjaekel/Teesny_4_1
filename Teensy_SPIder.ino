/*
 Name:		Teensy_SPIder.ino
 Created:	5/5/2023 12:21:04 PM
 Updated: 06/07/2023
 Author:	tj (tjaekel, Torsten Jaekel)
 Comment: we use now FreeRTOS (import as ZIP)
*/

#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include "SPI_dev.h"
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "TCP_Server.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "TFTP.h"
#include "MEM_Pool.h"
#include "CMD_thread.h"
#include "GPIO.h"

void setup(void) {

    Serial.begin(115200);       //baudrate does not matter: it is VCP UART, any baudrate works

#if 1
    //optional: wait for UART available - to see all early messages
    while (!Serial) {}

    if (CrashReport)
      Serial.print(CrashReport);
#endif

    CFG_Read();

    MEM_PoolInit();

    VCP_UART_setup();

    /* start in thios order: network openened first and reused afterwards */
    TCP_Server_setup();   //works with FreeRTOS

    TFTP_setup(UART_OUT);

    SPI_setup();

    GPIO_setup();

    CMD_setup();              //launch CMD interpreter as a thread

    ::Serial.println("setup(): starting scheduler...");
    ::Serial.flush();

    ::vTaskStartScheduler();
}

void loop() {
  /* this is empty now and never called again because of using FreeRTOS */
}

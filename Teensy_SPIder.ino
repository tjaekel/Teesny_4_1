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
#include "UDP_send.h"

#ifdef __cplusplus
extern "C" {
#endif
uint32_t set_arm_clock(uint32_t frequency);
#ifdef __cplusplus
}
#endif

uint32_t MCUCoreFrequency = 600000000;

FLASHMEM void setup(void) {
    /* overclocking:
     * ATTENTION:
     * 1. beyond 600 MHz GPIO signals do not reach anymore 3V3 level, the faster the lower the peak amplitude!
     * 2. if you make too fast: the bootloader starts to fail, e.g. 900 MHz is unreliable, 1000 MHz bricks the board!
     * actually, you cannot overclock really the MCU, 600 MHz seems to be the limit, if you want to have good signals (e.g. GPIOs)
     */
    MCUCoreFrequency = set_arm_clock(620000000);

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

    /* ---- start in this order: network opened first and reused afterwards */
    TCP_Server_setup();
    TFTP_setup(UART_OUT);
    UDP_setup();
    /* ---- */

    SPI_setup();

    GPIO_setup();

    CMD_setup();              //launch CMD interpreter as a thread

    ::Serial.flush();

    ::vTaskStartScheduler();
}

FLASHMEM void loop() {
  /* this is empty now and never called again because of using FreeRTOS */
}

void PrintRAMUsage(void) {
  freertos::print_ram_usage();
}

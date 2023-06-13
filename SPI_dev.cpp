// 
// SPI_dev.cpp
// Torsten Jaekel (tjaekel)
// 06/13/2023
// 

#include <arduino_freertos.h>
#include <semphr.h>
#include <avr/pgmspace.h>

#include "VCP_UART.h"
#include "SPI_dev.h"

#ifdef SPI_DMA_MODE
/* ATT: include just once, only here, not again through SPI_dev.h! */
#include "TsyDMASPI.h"
#endif

static SemaphoreHandle_t xSemaphore;

uint32_t SPIClkSpeed = 10000000;
#define ssPin   10    //SPI = 10; SPI1 = 9, pin 13 is LED and SPI SCLK!
#define ssPin2   9
SPISettings SPIsettings(16000000, arduino::LSBFIRST, SPI_MODE3);

void SPI_setup(void) {
	pinMode(ssPin, arduino::OUTPUT);
	digitalWrite(ssPin, arduino::HIGH);
  pinMode(ssPin2, arduino::OUTPUT);
	digitalWrite(ssPin2, arduino::HIGH);

  xSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive( xSemaphore );

#ifdef SPI_DMA_MODE
  /* Remark: if we do all the time the .begin(...) again on each transaction - we do not need here to do */
  TsyDMASPI0.begin(ssPin,  SPIsettings);
#else
	SPI1.begin();			//this kills the LED: LED, pin 13, is also SPI SCK!
#endif
}

int SPI_setClock(int clkspeed) {
  if (clkspeed)
    SPIClkSpeed = (uint32_t)clkspeed;

  return (int)SPIClkSpeed;
}

#ifdef SPI_DMA_MODE
int SPI_transaction(int num, unsigned char *tx, unsigned char *rx, int len) {
  //place semaphore around it, so we can use in current INT handlers
  if( xSemaphoreTake( xSemaphore, ( TickType_t ) 1000 ) == pdTRUE ) {
    if (num) {
      TsyDMASPI0.begin(ssPin2, SPISettings(SPIClkSpeed, arduino::LSBFIRST, SPI_MODE3));
    } else {
      TsyDMASPI0.begin(ssPin, SPISettings(SPIClkSpeed, arduino::LSBFIRST, SPI_MODE3));
    }
    TsyDMASPI0.transfer(tx, rx, len);
    ////TsyDMASPI0.end();

    xSemaphoreGive( xSemaphore );
  }
  else {
    print_log(UART_OUT, "*E: SPI semaphore timeout\r\n");
  }

	return 0;		//no error
}
#else
#if 0
int SPI_transaction(int num, unsigned char *tx, unsigned char *rx, int len) {
	SPI1.beginTransaction(SPIsettings);
	digitalWrite(ssPin, LOW);
	while (len--) {
		*rx++ = SPI1.transfer(*tx++);
	}
	digitalWrite(ssPin, HIGH);
	SPI1.endTransaction();

	return 0;		//no error
}
#else
int SPI_transaction(int num, unsigned char *tx, unsigned char *rx, int len) {
	SPI1.beginTransaction(SPIsettings);
	digitalWrite(ssPin, LOW);

	SPI1.transfer(tx, rx, (uint32_t)len);

	digitalWrite(ssPin, HIGH);
	SPI1.endTransaction();

	return 0;		//no error
}
#endif
#endif

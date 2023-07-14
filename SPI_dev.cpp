// 
// SPI_dev.cpp
// Torsten Jaekel (tjaekel)
// 06/13/2023
// 

#include <arduino_freertos.h>
#include <semphr.h>
#include <avr/pgmspace.h>

#include "SYS_config.h"
#include "SYS_error.h"
#include "VCP_UART.h"
#include "SPI_dev.h"
#include "GPIO.h"

#ifdef SPI_DMA_MODE
/* ATT: include just once, only here, not again through SPI_dev.h! */
#include "TsyDMASPI.h"
#else
#include <SPI.h>
#endif

static SemaphoreHandle_t xSemaphore;

#define ssPin   10    //SPI = 10; SPI1 = 9, pin 13 is LED and SPI SCLK!
#define ssPin2   9
SPISettings SPIsettings(16000000, arduino::LSBFIRST, SPI_MODE0);

FLASHMEM void SPI_setup(void) {
  
  GPIO_setOutValue(ssPin, arduino::HIGH);
	pinMode(ssPin, arduino::OUTPUT);
  ////digitalWrite(ssPin, arduino::HIGH);
	
  GPIO_setOutValue(ssPin2, arduino::HIGH);
  pinMode(ssPin2, arduino::OUTPUT);
  ////digitalWrite(ssPin2, arduino::HIGH);
	
  xSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive( xSemaphore );

#ifdef SPI_DMA_MODE
  /* Remark: if we do all the time the .begin(...) again on each transaction - we do not need here to do */
  ////TsyDMASPI0.begin(ssPin, SPISettings(gCFGparams.SPI1br, (gCFGparams.SPI1mode & 0xF0) >> 4, (gCFGparams.SPI1mode & 0xF) << 2));
#else
	SPI.begin();			//this kills the LED: LED, pin 13, is also SPI SCK!
#endif
}

int SPI_setClock(int clkspeed) {
  if (clkspeed) {
    gCFGparams.SPI1br = (unsigned long)clkspeed;
    gCFGparams.SPI2br = (unsigned long)clkspeed;
  }

  return (int)gCFGparams.SPI1br;
}

#ifdef SPI_DMA_MODE
int SPI_transaction(int num, const unsigned char *tx, unsigned char *rx, int len) {
  //place semaphore around it, so we can use in concurrent INT handlers
  if( xSemaphoreTake( xSemaphore, ( TickType_t ) 1000 ) == pdTRUE ) {
    if (num) {
      TsyDMASPI0.begin(ssPin2, SPISettings(gCFGparams.SPI1br, (gCFGparams.SPI1mode & 0xF0) >> 4, (gCFGparams.SPI1mode & 0xF) << 2));
    } else {
      TsyDMASPI0.begin(ssPin, SPISettings(gCFGparams.SPI1br, (gCFGparams.SPI1mode & 0xF0) >> 4, (gCFGparams.SPI1mode & 0xF) << 2));
    }
    TsyDMASPI0.transfer(tx, rx, len);
    ////TsyDMASPI0.end();

    xSemaphoreGive( xSemaphore );

    //TODO: SPI debug
  }
  else {
    print_log(UART_OUT, "*E: SPI semaphore timeout\r\n");
    SYSERR_Set(UART_OUT, SYSERR_SPI);
  }

	return 0;		//no error
}
#else
#if 0
int SPI_transaction(int num, const unsigned char *tx, unsigned char *rx, int len) {
	SPI.beginTransaction(SPIsettings);
	digitalWrite(ssPin, LOW);
	while (len--) {
		*rx++ = SPI.transfer(*tx++);
	}
	digitalWrite(ssPin, HIGH);
	SPI.endTransaction();

	return 0;		//no error
}
#else
int SPI_transaction(int num, const unsigned char *tx, unsigned char *rx, int len) {
  if (num) {
	  SPI.beginTransaction(SPISettings(gCFGparams.SPI1br, (gCFGparams.SPI1mode & 0xF0) >> 4, (gCFGparams.SPI1mode & 0xF) << 2));
	  digitalWrite(ssPin2, arduino::LOW);

	  SPI.transfer(tx, rx, (uint32_t)len);

	  digitalWrite(ssPin2, arduino::HIGH);
	  SPI.endTransaction();
  }
  else {
    SPI.beginTransaction(SPISettings(gCFGparams.SPI1br, (gCFGparams.SPI1mode & 0xF0) >> 4, (gCFGparams.SPI1mode & 0xF) << 2));
	  digitalWrite(ssPin, arduino::LOW);

	  SPI.transfer(tx, rx, (uint32_t)len);

	  digitalWrite(ssPin, arduino::HIGH);
	  SPI.endTransaction();
  }

	return 0;		//no error
}
#endif
#endif

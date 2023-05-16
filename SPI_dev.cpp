// 
// 
// 

#include "SPI_dev.h"

#ifdef SPI_DMA_MODE
/* ATT: include just once, only here, not again through SPI_dev.h! */
#include "TsyDMASPI.h"
#endif

uint32_t SPIClkSpeed = 16000000;
const int ssPin = 10;    //SPI = 10; SPI1 = 0
SPISettings SPIsettings(16000000, LSBFIRST, SPI_MODE3);

void SPI_setup(void) {
	pinMode(ssPin, OUTPUT);
	digitalWrite(ssPin, HIGH);

#ifdef SPI_DMA_MODE
  /* Remark: if we do all the time the .begin(...) again on each transaction - we do not need here to do */
  TsyDMASPI0.begin(ssPin, SPIsettings);
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
  TsyDMASPI0.begin(ssPin, SPISettings(SPIClkSpeed, LSBFIRST, SPI_MODE3));
  TsyDMASPI0.transfer(tx, rx, len);
  ////TsyDMASPI0.end();

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

// SPI_dev.h

#ifndef _SPI_dev_h
#define _SPI_dev_h

#define SPI_DMA_MODE

#ifdef SPI_DMA_MODE
/* ATT: include just once, not here, not again through SPI_dev.h loaded in sketch *.ino! */
////#include "TsyDMASPI.h"
#else
//#if defined(ARDUINO) && ARDUINO >= 100
#include <arduino.h>
//#else
//#include "WProgram.h"
////#endif
#endif

void SPI_setup(void);

int  SPI_setClock(int clkspeed);

/* used also by Pico-C as C code */
#ifdef __cplusplus
extern "C" {
#endif
int  SPI_transaction(int num, unsigned char *tx, unsigned char *rx, int len);
#ifdef __cplusplus
}
#endif

#endif


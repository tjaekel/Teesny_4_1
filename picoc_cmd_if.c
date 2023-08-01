/*
 * picoc_cmd_if.c
 *
 *  Created on: Aug 19, 2022
 *      Author: tj
 */

#include "VCP_UART.h"
#include "cmd_dec.h"
//#include "GPIO_user.h"
#include "SPI_dev.h"
//#include "I2C3_user.h"
#include "picoc.h"
//#include <cmsis_os2.h>
#include "MEM_Pool.h"
#include "UDP_send.h"
#include "CHIP_spi_cmd.h"

static int sPicoC_Stopped = 0;

void picoc_ClearStopped(void)
{
	sPicoC_Stopped = 0;
}

void picoc_SetStopped(void)
{
	sPicoC_Stopped = 1;
}

int picoc_CheckForStopped(void)
{
    return sPicoC_Stopped;
}

int picoc_CheckForAbort(void)
{
	/* read UART not-waiting and check for CTRL-C */
	int c;
	c = UART_getCharNW();
  if (c)
	{
		if (c == 0x03)
		{
			GpicocAbort = 1;
			longjmp(ExitBuf, 1);
			return 1;
		}
	}
    return 0;
}

int picoc_ExecuteCommand(char *s)
{
	////extern ECMD_DEC_Status CMD_DEC_execute(const char *cmd, EResultOut out);
	CMD_DEC_execute(s, UART_OUT);
    return 0;
}

int picoc_SpiTransaction(unsigned char *tx, unsigned char *rx, int bytes)
{
  /* ATTENTION:
   * our Pico-C scripts, with buffers, variables ... are located on EXTMEM!
   * a SPI DMA to/from there is not possible!
   * we need a temporary buffer just for the SPI_transaction
   */
  unsigned char *SPItx;
  unsigned char *SPIrx;
  int r;

  SPItx = (unsigned char *)MEM_PoolAlloc(bytes);
  if ( ! SPItx)
    return 0;
  SPIrx = (unsigned char *)MEM_PoolAlloc(bytes);
  if ( ! SPIrx) {
    MEM_PoolFree(SPItx);
    return 0;
  }

  memcpy(SPItx, tx, bytes);
  //memset(SPIrx, 0, bytes);      //just to avoid warning
	r = SPI_transaction(0, SPItx, SPIrx, bytes);
  memcpy(rx, SPIrx, 10);

  MEM_PoolFree(SPItx);
  MEM_PoolFree(SPIrx);
  
  return r;
}

static unsigned char txBuf[1024 + 4];
static unsigned char rxBuf[1024 + 4];

static unsigned short sINTStatus[2];

unsigned short picoc_GetINTfifo(int num, unsigned char *rx, int bytes)
{
  /* do a rreg INT_STATUS plus RBLK FIFO (0xF0),
   * return all in buffer rx, INT_STATUS as return value
   */
  int i;
  unsigned short r;

  if (bytes > (1024 - 4))
    bytes = (1024 - 4);

  txBuf[0] = 0x02;        //rreg 0  INT_STATUS
  txBuf[1] = 0x00;
  if (bytes) {
    txBuf[2] = 0x06;       //rblk 0xF0
    txBuf[3] = 0xF0;
    //fill noops
    for (i = 0; i < bytes; i++)
      txBuf[4 + i] = 0x00;
  }
  else {
    txBuf[2] = 0x00;
    txBuf[3] = 0x00;
  }

  SPI_transaction(num, txBuf, rxBuf, bytes + 4);
  if (bytes) {
    //we have to copy: EXTMEM is not possible for SPI DMA
    memcpy(rx, &rxBuf[4], bytes);
  }
  //get INT_STATUS
  r = rxBuf[2];
  r |= (rxBuf[3] << 8);       //little endian

  sINTStatus[num] = r;

  return r;
}

unsigned short picoc_GetINTStatus(int num) {
  return sINTStatus[num];
}

static const unsigned char cSpiTx[960 + 4] PROGMEM = { 0x02, 0x00, 0x06, 0xF0 /*rest zero*/};
static const unsigned char cSpiTx2[4] PROGMEM = { 0x02, 0x00, 0x00, 0x00};

void picoc_DefaultINTHandlerC(int num) {
  unsigned short r;
  if (num)
    SPI_transaction(num, cSpiTx2, rxBuf, 4);
  else
    SPI_transaction(num, cSpiTx, rxBuf, 960 + 4);

  //get INT_STATUS
  r = rxBuf[2];
  r |= (rxBuf[3] << 8);       //little endian

  sINTStatus[num] = r;

  if (num)
    UDP_send(8082, rxBuf, 4);
  else
    UDP_send(8081, rxBuf, 960 + 4);
}

void picoc_decodePRIfifo(void) {
  CHIP_decodePRI_FIFO(rxBuf, 960 + 4, UART_OUT, 0);
}

#if 0
int picoc_I2CRead(unsigned char slaveAddr, unsigned char regAddr, unsigned char *data, int bytes, int flags)
{
	return I2CUser_MemReadEx(slaveAddr, regAddr, data, (uint16_t)bytes);
}

int picoc_I2CWrite(unsigned char slaveAddr, unsigned char *data, int bytes, int flags)
{
	return I2CUser_Write((uint16_t)slaveAddr, data, (uint16_t)bytes);
}

void picoc_WriteGPIO(unsigned long val)
{
	GPIO_Set(val, 0xFFFFFFFF);
}

unsigned long picoc_ReadGPIO(void)
{
	return GPIO_Get();
}
#endif

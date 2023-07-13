
#include "SYS_config.h"
#include "SYS_error.h"
#include "cmd_dec.h"
#include "SPI_dev.h"
#include "CHIP_spi_cmd.h"
#include "MEM_Pool.h"
#include <SD.h>
#include "SD_Card.h"

TSPIBuffer SPIbuf = {
		.concurFlag = 0,
		.wasWriteBefore = 0,
		.inIdx = 0,
		.endIdx = 0,
		.chsIdx = 0,
		.chksumTx = 0,
		.chksumRx = 0
};

void print_SPIbuf(uint8_t *tx, uint8_t *rx, unsigned int bytes, EResultOut out)
{
	/* print the Tx and Rx SPI buffers */
	unsigned int i;

	print_log(out, (const char *)"SPI Tx | ");
	for (i = 0; i < bytes; i += 2)
	{
		print_log(out, (const char *)"0x%02x%02x ", (int)*(tx + 1), (int)*tx);
		tx += 2;
	}
	print_log(out, (const char *)"\r\nSPI Rx | ");
	for (i = 0; i < bytes; i += 2)
	{
		print_log(out, (const char *)"0x%02x%02x ", (int)*(rx + 1), (int)*rx);
		rx += 2;
	}

	UART_Send((const char *)"\r\n", 2, out);
}

ECMD_DEC_Status CHIP_cid(TCMD_DEC_Results *res, EResultOut out)
{
	(void)res;
	unsigned int inIdx;
	int device;				//default is 0 [-P] if not specified or -A

	if (SPIbuf.concurFlag)
	{
		if ( ! (gCFGparams.CfgFlags & 0x00000020))
		{
			if (SPIbuf.endIdx >= 2)
			{
				if (SPIbuf.wasWriteBefore)
					inIdx = SPIbuf.endIdx;
				else
					inIdx = SPIbuf.endIdx -2;
				SPIbuf.inIdx = inIdx;
			}
			else
				inIdx = SPIbuf.inIdx;		//actually, it should be 0 then
		}
		else
		{
			inIdx = SPIbuf.inIdx;
			SPIbuf.endIdx = inIdx;			//any read breaks NOOPs of previous read
		}
	}
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;

	device = CMD_getSPIoption(res->opt);

	SPIbuf.spiTx[inIdx +  0] = 0x02;
	SPIbuf.spiTx[inIdx +  1] = 0xE0;		//CHIP_ID address
	SPIbuf.spiTx[inIdx +  2] = 0x0D;
	SPIbuf.spiTx[inIdx +  3] = 0x00;		//TIR
	SPIbuf.spiTx[inIdx +  4] = 0;
	SPIbuf.spiTx[inIdx +  5] = 0;
	SPIbuf.spiTx[inIdx +  6] = 0;
	SPIbuf.spiTx[inIdx +  7] = 0;
	SPIbuf.spiTx[inIdx +  8] = 0;
	SPIbuf.spiTx[inIdx +  9] = 0;
	SPIbuf.spiTx[inIdx + 10] = 0;
	SPIbuf.spiTx[inIdx + 11] = 0;
	SPIbuf.spiTx[inIdx + 12] = 0;
	SPIbuf.spiTx[inIdx + 13] = 0;			//5x NOOP code

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 12;							//1x NOOP left
		if (SPIbuf.endIdx < (SPIbuf.inIdx + 2))
			SPIbuf.endIdx = SPIbuf.inIdx + 2;		//end after NOOP

		return CMD_DEC_OK;
	}

	/* not just on PRI channel - allow on -T and -A for SPI testing, but the
	 * register address is the same as for -P channel
	 */
	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, 14);

	//check TIR
	{
		uint16_t txCS, rxCS, rxtxCS, rxrxCS;
		int i;
		txCS = 0;
		//build CS for Tx
		for (i = 0; i < (14/2); i++)
		{
			txCS += (uint16_t)SPIbuf.spiTx[2*i + 0] | ((uint16_t)SPIbuf.spiTx[2*i + 1] << 8);
		}
		txCS = ~txCS;			//invert

		rxCS = 0;
		//build CS for Rx
		for (i = 0; i < (14/2 -1); i++)
		{
			rxCS += (uint16_t)SPIbuf.spiRx[2*i + 0] | ((uint16_t)SPIbuf.spiRx[2*i + 1] << 8);
		}
		rxCS = ~rxCS;			//invert

		rxtxCS = (uint16_t)SPIbuf.spiRx[10] | ((uint16_t)SPIbuf.spiRx[11] << 8);
		rxrxCS = (uint16_t)SPIbuf.spiRx[12] | ((uint16_t)SPIbuf.spiRx[13] << 8);

		if (gCFGparams.DebugFlags & DBG_VERBOSE)
			print_log(out, (const char *)"TX: tx: 0x%04x rx: 0x%04x | RX: tx: 0x%04x rx: 0x%04x\r\n", txCS, rxCS, rxtxCS, rxrxCS);

		//check the CS
		if ((txCS != rxtxCS) || (rxCS != rxrxCS))
		{
			SYSERR_Set(out, SYSERR_TIR);
			print_log(out, (const char *)"*E: TIR invalid\r\n");
		}
		else
		{
      //TIR is OK
		}
	}

	/* print the received bytes */
	print_log(out, (const char *)"0x%02x%02x\r\n", SPIbuf.spiRx[3], SPIbuf.spiRx[2]);		//little endian

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_rreg(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int i, j;
	int device;				//default is 0 [-P] if not specified or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
	{
		if ( ! (gCFGparams.CfgFlags & 0x00000020))
		{
			if (SPIbuf.endIdx >= 2)
			{
				if (SPIbuf.wasWriteBefore)
					inIdx = SPIbuf.endIdx;
				else
					inIdx = SPIbuf.endIdx -2;	//ATT: on a write before it is not right
				SPIbuf.inIdx = inIdx;
			}
			else
				inIdx = SPIbuf.inIdx;		//actually, it should be 0 then
		}
		else
		{
			inIdx = SPIbuf.inIdx;
			SPIbuf.endIdx = inIdx;			//any read breaks NOOPs of previous read
		}
	}
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;	//we need at least address, no number is assumed as 1

	/* check if number of registers is valid, make it valid, if omitted we assume one */
	if (res->val[1] > 32)
    	res->val[1] = 32;
	if (res->val[1] == 0)
    	res->val[1] = 1;		//assume as 1 word

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet for read
	 */
	SPIbuf.spiTx[inIdx + 0] = (uint8_t)(0x02 | ((res->val[1] - 1) << 3));	//CMD + length, as N+1
	SPIbuf.spiTx[inIdx + 1] = (uint8_t)res->val[0];				//ADDR
	for (i = inIdx; i < (res->val[1] * 2 + inIdx); i += 2)
	{
		SPIbuf.spiTx[i + 2] = 0x00;
		SPIbuf.spiTx[i + 3] = 0x00;								//NOOP
	}

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2;										//NOOPs left
		if (SPIbuf.endIdx < (SPIbuf.inIdx + (res->val[1]) * 2))
			SPIbuf.endIdx = SPIbuf.inIdx + res->val[1] * 2;		//end after NOOP

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + (res->val[1] * 2)));

	/* print the received bytes */
	j = res->val[1];
	for (i = 0; i < j; i++)
	{
		uint16_t val;

		val = (uint16_t)((SPIbuf.spiRx[3 + 2*i] << 8) | SPIbuf.spiRx[2 + 2*i]);	//little endian

		print_log(out, (const char *)"0x%04x ", val);

		/* fill the res->val[] with the read values - we need 'rreg' also for other purposes, e.g. 'preg' */
		res->val[i] = (unsigned long)val;
	}
	UART_Send((const char *)"\r\n", 2, out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_wreg(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	unsigned int i;
	int device;				//default is 0 [-P] if not specified or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	if (res->num < 2)
		return CMD_DEC_INVALID;	//we need at least address and one value to write
	if (res->num > 33)
		res->num = 33;			//just 1x address and 32 values as max.

	SPIbuf.wasWriteBefore = 1;

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet for write
	 */
	SPIbuf.spiTx[inIdx + 0] = (uint8_t)(0x04 | ((res->num - 1 - 1) << 3));	//CMD + length, as N+1, w/o ADDR parameter
	SPIbuf.spiTx[inIdx + 1] = (uint8_t)res->val[0];							//ADDR
	for (i = 0; i < (res->num - 1); i++)									//prepare value words to write
	{
		SPIbuf.spiTx[inIdx + 2 + 2*i] = (uint8_t)(res->val[1 + i] & 0xFF);
		SPIbuf.spiTx[inIdx + 3 + 2*i] = (uint8_t)((res->val[1 + i] >> 8) & 0xFF);		//16bit word as little endian
	}

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2 + i * 2;					//no NOOPs left on wreg
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx;				//end after all for wreg

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + (res->num - 1) * 2));

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_peek(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int i, j;
	int device;				//default is 0 [-P] if not specified or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
	{
		if ( ! (gCFGparams.CfgFlags & 0x00000020))
		{
			if (SPIbuf.endIdx >= 2)
			{
				if (SPIbuf.wasWriteBefore)
					inIdx = SPIbuf.endIdx;
				else
					inIdx = SPIbuf.endIdx -2;
				SPIbuf.inIdx = inIdx;
			}
			else
				inIdx = SPIbuf.inIdx;		//actually, it should be 0 then
		}
		else
		{
			inIdx = SPIbuf.inIdx;
			SPIbuf.endIdx = inIdx;			//any read breaks NOOPs of previous read
		}
	}
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;	//we need at least address, no number is assumed as 1

	/* check if number of registers is valid, make it valid, if omitted we assume one */
	if (res->val[1] > 32)
    	res->val[1] = 32;
	if (res->val[1] == 0)
    	res->val[1] = 1;		//assume as 1 word

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	/* PGSET */
	SPIbuf.spiTx[inIdx + 0] = 0x05;				//SYSC_PGSET code, b[6:3] is 0000
	SPIbuf.spiTx[inIdx + 1] = 0x01;				//SYSC PGSET code

	/* possible to set page as 0..255 */
	SPIbuf.spiTx[inIdx + 2] = (uint8_t)(res->val[0] >>  8);		//add page number, low part
	SPIbuf.spiTx[inIdx + 3] = (uint8_t)(res->val[0] >> 16);		//little endian, high part

	SPIbuf.spiTx[inIdx + 4] = (uint8_t)(0x02 | ((res->val[1] - 1) << 3));	//CMD + length, as N+1
	SPIbuf.spiTx[inIdx + 5] = (uint8_t)res->val[0];				//ADDR
	/* it can be one NOOP less: we use PGSET command as one NOOP */
	for (i = inIdx; i < (res->val[1] * 2 + inIdx - 2); i += 2)
	{
		SPIbuf.spiTx[i + 6] = 0x00;
		SPIbuf.spiTx[i + 7] = 0x00;				//NOOP
	}

	/* PGSET to 0 again */
	SPIbuf.spiTx[i + 6] = 0x05;					//SYSC_PGSET code, b[6:3] is 0000
	SPIbuf.spiTx[i + 7] = 0x01;					//SYSC PGSET code

	/* back to bank 0 */
	SPIbuf.spiTx[i + 8] = (uint8_t)(0);			//add page number, low part
	SPIbuf.spiTx[i + 9] = (uint8_t)(0);			//little endian, high part

	if (SPIbuf.concurFlag)
	{
#ifdef NOT_THIS_WAY_ANYMORE
		SPIbuf.inIdx += 6;						//NOOPs left
		if (SPIbuf.endIdx < (SPIbuf.inIdx + (int)(res->val[1]) * 2))
			SPIbuf.endIdx = SPIbuf.inIdx + res->val[1] * 2;			//end after NOOP
#else
		//SPIbuf.inIdx += i + 10;
		SPIbuf.inIdx = i + 10;					//i has already inIdx
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx + 2;
#endif

		SPIbuf.wasWriteBefore = 1;

		/* add TIR, also in concur */
		if ( ! (gCFGparams.CfgFlags & 0x00000004))
		{
			inIdx = SPIbuf.inIdx;

			/* add TIR command */
			SPIbuf.spiTx[inIdx + 0] = 0x0D;
			SPIbuf.spiTx[inIdx + 1] = 0x00;
			for (i = 0; i < (2*5); i++)
			{
				SPIbuf.spiTx[inIdx + i + 2] = 0x00;
			}

			SPIbuf.inIdx += i /*+ 2*/;			//we can reuse one NOOP for new command
			if (SPIbuf.endIdx < SPIbuf.inIdx)
				SPIbuf.endIdx = SPIbuf.inIdx + 2;

			SPIbuf.wasWriteBefore = 0;
		}

		return CMD_DEC_OK;
	}

	if ( ! (gCFGparams.CfgFlags & 0x00000004))
	{
		/* add TIR command */
		SPIbuf.spiTx[i + 10] = 0x0D;
		SPIbuf.spiTx[i + 11] = 0x00;
		for (j = 0; j < 10; j++)
		{
			SPIbuf.spiTx[i + 12 + j] = 0x00;
		}

		//* PGSET is one NOOP already
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(6 + res->val[1] * 2 + 4 + 12 - 2));

		/* check the TIR response */
#if 0
		j = 6 + (res->val[1] * 2) + 4 - 2;
		for (i = 0; i < 12; i++)
		{
			if ((SPIbuf.spiRx[j + i] != 0x00) && (SPIbuf.spiRx[j + i] != 0xFF))
			{
				break;
			}
		}
		if (i == 12)
		{
			/* TIR was all 0x00 or 0xFF - send error message */
			SYSERR_Set(out, SYSERR_TIR);
			print_log(out, (const char *)"*E: TIR invalid\r\n");
		}
		else
		{
			/* OK, TIR is not all 0x00 or 0xFF */
			/* toggle LED blue and turn red LED off */
		}
#else
		{
			uint16_t txCS, rxCS, rxtxCS, rxrxCS;
			unsigned long i;
			txCS = 0;
			//build CS for Tx
			for (i = 0; i < ((6 + (res->val[1] * 2) + 4 + 12 - 2)/2); i++)
			{
				txCS += (uint16_t)SPIbuf.spiTx[2*i + 0] | ((uint16_t)SPIbuf.spiTx[2*i + 1] << 8);
			}
			txCS = ~txCS;			//invert

			rxCS = 0;
			//build CS for Rx
			for (i = 0; i < ((6 + (res->val[1] * 2) + 4 + 12 - 2)/2 -1); i++)
			{
				rxCS += (uint16_t)SPIbuf.spiRx[2*i + 0] | ((uint16_t)SPIbuf.spiRx[2*i + 1] << 8);
			}
			rxCS = ~rxCS;			//invert

			rxtxCS = (uint16_t)SPIbuf.spiRx[6 + res->val[1] * 2 + 4 + 12 - 2 -4] | ((uint16_t)SPIbuf.spiRx[6 + res->val[1] * 2 + 4 + 12 - 2 -3] << 8);
			rxrxCS = (uint16_t)SPIbuf.spiRx[6 + res->val[1] * 2 + 4 + 12 - 2 -2] | ((uint16_t)SPIbuf.spiRx[6 + res->val[1] * 2 + 4 + 12 - 2 -1] << 8);

			if (gCFGparams.DebugFlags & DBG_VERBOSE)
				print_log(out, (const char *)"TX: tx: 0x%04x rx: 0x%04x | RX: tx: 0x%04x rx: 0x%04x\r\n", txCS, rxCS, rxtxCS, rxrxCS);

			//check the CS
			if ((txCS != rxtxCS) || (rxCS != rxrxCS))
			{
				SYSERR_Set(out, SYSERR_TIR);
				print_log(out, (const char *)"*E: TIR invalid\r\n");
			}
			else
			{
				////BSP_LED_Off(LED_RED);
			}
		}
#endif
	}
	else
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(6 + res->val[1] * 2 + 4));

	/* print the received bytes */
	j = res->val[1];
	for (i = 0; i < j; i++)
	{
		uint16_t val;

		val = (uint16_t)((SPIbuf.spiRx[7 + 2*i] << 8) | SPIbuf.spiRx[6 + 2*i]);	//little endian

		print_log(out, (const char *)"0x%04x ", (int)val);

		/* fill the res->val[] with the read values - we need 'rreg' also for other purposes, e.g. 'preg' */
		res->val[i] = (unsigned long)val;
	}
	UART_Send((const char *)"\r\n", 2, out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_poke(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	unsigned int i, j;
	int device;				//default is 0 [-P] if not specified or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	if (res->num < 2)
		return CMD_DEC_INVALID;	//we need at least address and one value to write
	if (res->num > 33)
		res->num = 33;			//just 1x address and 32 values as max.

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	/* PGSET */
	SPIbuf.spiTx[inIdx + 0] = 0x05;						//SYSC_PGSET code, b[6:3] is 0000
	SPIbuf.spiTx[inIdx + 1] = 0x01;						//SYSC PGSET code

	/* possible to set page as 0..255 */
	SPIbuf.spiTx[inIdx + 2] = (uint8_t)(res->val[0] >>  8);		//add page number, low part
	SPIbuf.spiTx[inIdx + 3] = (uint8_t)(res->val[0] >> 16);		//little endian, high part

	SPIbuf.spiTx[inIdx + 4] = (uint8_t)(0x04 | ((res->num - 1 - 1) << 3));	//CMD + length, as N+1, w/o ADDR parameter
	SPIbuf.spiTx[inIdx + 5] = (uint8_t)res->val[0];							//ADDR
	for (i = 0; i < (res->num - 1); i++)									//prepare value words to write
	{
		SPIbuf.spiTx[inIdx + 6 + 2*i] = (uint8_t)(res->val[1 + i] & 0xFF);
		SPIbuf.spiTx[inIdx + 7 + 2*i] = (uint8_t)((res->val[1 + i] >> 8) & 0xFF);		//16bit word as little endian
	}

	/* PGSET to bank 0*/
	SPIbuf.spiTx[inIdx + 6 + 2*i] = 0x05;				//SYSC_PGSET code, b[6:3] is 0000
	SPIbuf.spiTx[inIdx + 7 + 2*i] = 0x01;				//SYSC PGSET code

	/* possible to set page as 0..255 */
	SPIbuf.spiTx[inIdx + 8 + 2*i] = (uint8_t)(0);		//add page number, low part
	SPIbuf.spiTx[inIdx + 9 + 2*i] = (uint8_t)(0);		//little endian, high part

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 6 + (res->num - 1) * 2 + 4;		//no NOOPs left on wreg and pgset
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx;				//end after all for wreg

		SPIbuf.wasWriteBefore = 1;

		/* add TIR, also in concur */
		if ( ! (gCFGparams.CfgFlags & 0x00000004))
		{
			inIdx = SPIbuf.inIdx;

			/* add TIR command */
			SPIbuf.spiTx[inIdx + 0] = 0x0D;
			SPIbuf.spiTx[inIdx + 1] = 0x00;
			for (i = 0; i < (2*5); i++)
			{
				SPIbuf.spiTx[inIdx + i + 2] = 0x00;
			}

			SPIbuf.inIdx += i /*+ 2*/;			//we can reuse one NOOP for new command
			if (SPIbuf.endIdx < SPIbuf.inIdx)
				SPIbuf.endIdx = SPIbuf.inIdx + 2;

			SPIbuf.wasWriteBefore = 0;
		}

		return CMD_DEC_OK;
	}

	if ( ! (gCFGparams.CfgFlags & 0x00000004))
	{
		/* add TIR command */
		SPIbuf.spiTx[inIdx + 10 + 2*i] = 0x0D;
		SPIbuf.spiTx[inIdx + 11 + 2*i] = 0x00;
		for (j = 0; j < 10; j++)
		{
			SPIbuf.spiTx[inIdx + 12 + j + 2*i] = 0x00;
		}

		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(6 + (res->num - 1) * 2 + 4 + 12));

		/* check the TIR response */
#if 0
		j = 6 + ((res->num - 1) * 2) + 4;
		for (i = 0; i < 12; i++)
		{
			if ((SPIbuf.spiRx[j + i] != 0x00) && (SPIbuf.spiRx[j + i] != 0xFF))
			{
				break;
			}
		}
		if (i == 12)
		{
			/* TIR was all 0x00 or 0xFF - send error message */
			SYSERR_Set(out, SYSERR_TIR);
			print_log(out, (const char *)"*E: TIR invalid\r\n");
		}
		else
		{
			/* OK, TIR is not all 0x00 or 0xFF */
			return CMD_DEC_OK;
		}
#else
		{
			uint16_t txCS, rxCS, rxtxCS, rxrxCS;
			unsigned long i;
			txCS = 0;
			//build CS for Tx
			for (i = 0; i < ((6 + (res->num - 1) * 2 + 4 + 12)/2); i++)
			{
				txCS += (uint16_t)SPIbuf.spiTx[2*i + 0] | ((uint16_t)SPIbuf.spiTx[2*i + 1] << 8);
			}
			txCS = ~txCS;			//invert

			rxCS = 0;
			//build CS for Rx
			for (i = 0; i < ((6 + (res->num - 1) * 2 + 4 + 12)/2 -1); i++)
			{
				rxCS += (uint16_t)SPIbuf.spiRx[2*i + 0] | ((uint16_t)SPIbuf.spiRx[2*i + 1] << 8);
			}
			rxCS = ~rxCS;			//invert

			rxtxCS = (uint16_t)SPIbuf.spiRx[6 + (res->num - 1) * 2 + 4 + 12 -4] | ((uint16_t)SPIbuf.spiRx[6 + (res->num - 1) * 2 + 4 + 12 -3] << 8);
			rxrxCS = (uint16_t)SPIbuf.spiRx[6 + (res->num - 1) * 2 + 4 + 12 -2] | ((uint16_t)SPIbuf.spiRx[6 + (res->num - 1) * 2 + 4 + 12 -1] << 8);

			if (gCFGparams.DebugFlags & DBG_VERBOSE)
				print_log(out, (const char *)"TX: tx: 0x%04x rx: 0x%04x | RX: tx: 0x%04x rx: 0x%04x\r\n", txCS, rxCS, rxtxCS, rxrxCS);

			//check the CS
			if ((txCS != rxtxCS) || (rxCS != rxrxCS))
			{
				SYSERR_Set(out, SYSERR_TIR);
				print_log(out, (const char *)"*E: TIR invalid\r\n");
			}
			else
			{
				//OK
			}
		}
#endif
	}
	else
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(6 + (res->num - 1) * 2 + 4));

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_sysc(TCMD_DEC_Results *res, EResultOut out)
{
	(void) out;
	unsigned int i;
	int device;				//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 1;

	device = CMD_getSPIoption(res->opt);

	if (res->num > 6)
		res->num = 6;			//allow up to 1 + 5 (NOOP) words for SYSC_TIR
	if (res->num <= 0)
		res->num = 1;			//make sure we need at least one sysc_code
								//res->val[0] should be 0 already
	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = 0x05;

	SPIbuf.spiTx[inIdx + 0] |= (uint8_t)((res->val[0] & 0x001F) << 3);		//CMD + SYSC code, low part
	SPIbuf.spiTx[inIdx + 1]  = (uint8_t)((res->val[0] & 0x1FE0) >> 5);		//SYSC code, little endian, high part

	/*
	 * add optional words in SYSC SPI transactions, e.g. NOOPS (for TIR etc.)
	 * or for PGSET a page number, for PGGET a NOOP
	 * ATT: we do not check if such mandatory parameter is provided !
	 */
	for (i = 1; i < res->num; i++)
	{
		SPIbuf.spiTx[inIdx + 2*i + 0] = (uint8_t)(res->val[i] & 0xFF);
		SPIbuf.spiTx[inIdx + 2*i + 1] = (uint8_t)((res->val[i] >> 8) & 0xFF);	//little endian
	}

	if (SPIbuf.concurFlag)
	{
		if ((SPIbuf.spiTx[inIdx + 0] == 0x05) && (SPIbuf.spiTx[inIdx + 1] == 0x01))
		{
			/* we have PGSET - one additional word! */
			SPIbuf.inIdx += 2 + 2;									//SYSC PGSET has one word at least
			/**
			 * ATT: if no additional word is provided - it can be random value for page!
			 */
			if (res->num >= 2)
			{
				if (SPIbuf.endIdx < (SPIbuf.inIdx + (res->num - 2) * 2))
						SPIbuf.endIdx = SPIbuf.inIdx + (res->num - 2) * 2;	//end after all for SYSC
			}
			else
			{
				if (SPIbuf.endIdx < SPIbuf.inIdx)
					SPIbuf.endIdx = SPIbuf.inIdx;
			}
		}
		else
		{
			/* on all other SYSC we assume NOOPs to have */
			SPIbuf.inIdx += 2;										//SYSC command plus (optional) NOOPs
			if (SPIbuf.endIdx < (SPIbuf.inIdx + (res->num - 1) * 2))
				SPIbuf.endIdx = SPIbuf.inIdx + (res->num - 1) * 2;	//end after all (optional) NOOPs
		}

		/**
		 * ATT: we assume NOOPS as optional values after SYSC, except for PGSET
		 * ATT: we do not check here if SYSC PGSET has a mandatory word for page, or
		 * PGSET a mandatory NOOP !
		 * Make sure on command usage to provide mandatory words !
		 */
		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + 2*(res->num - 1)));

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_concur(TCMD_DEC_Results *res, EResultOut out)
{
	int device;				//default is 2 [-P] if not specified via -T or -A
	ECMD_DEC_Status err = CMD_DEC_OK;
	char *cmdStr;

	device = CMD_getSPIoption(res->opt);

	/*
	 * cross-check: if concurFlag is already set - a concur is there, finish it
	 * and start a new concur
	 */
	if (SPIbuf.concurFlag)
	{
		if (SPIbuf.endIdx > 0)
		{
			/* send the entire SPI transaction */
			SPI_transaction(SPIbuf.device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)SPIbuf.endIdx);

			/* print the SPI Tx and Rx */
			print_SPIbuf(SPIbuf.spiTx, SPIbuf.spiRx, SPIbuf.endIdx, out);
		}

		/* reset all flags and indexes */
		SPIbuf.concurFlag = 0;
		SPIbuf.inIdx = SPIbuf.endIdx = 0;
	}

	/* store the device for sub-commands */
	SPIbuf.device = device;

	/* set a flag for running in concur sub-command-mode */
	SPIbuf.concurFlag = 1;

	/* call the command interpreter again for all the sub-commands */
	cmdStr = res->str;
	if (cmdStr)
		err = CMD_DEC_execute(cmdStr, out);

	/* concur was there - break the outer loop */
	res->ctl = 1;

	/* if all successful - send the generated SPI buffer */
	if (SPIbuf.endIdx > 0)
	{
		/* send the entire SPI transaction */
		SPI_transaction(SPIbuf.device, SPIbuf.spiTx, SPIbuf.spiRx, SPIbuf.endIdx);

		/* print the SPI Tx and Rx */
		print_SPIbuf(SPIbuf.spiTx, SPIbuf.spiRx, SPIbuf.endIdx, out);
	}

	/* reset all flags and indexes */
	SPIbuf.concurFlag = 0;
	SPIbuf.inIdx = SPIbuf.endIdx = 0;
	SPIbuf.wasWriteBefore = 0;

	return err;
}

ECMD_DEC_Status CHIP_rblk(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int i;
	int device;				//default is 0 [-P] if not specified or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
	{
		if ( ! (gCFGparams.CfgFlags & 0x00000020))
		{
			if (SPIbuf.endIdx >= 2)
			{
				if (SPIbuf.wasWriteBefore)
					inIdx = SPIbuf.endIdx;
				else
					inIdx = SPIbuf.endIdx -2;
				SPIbuf.inIdx = inIdx;
			}
			else
				inIdx = SPIbuf.inIdx;		//actually, it should be 0 then
		}
		else
		{
			inIdx = SPIbuf.inIdx;
			SPIbuf.endIdx = inIdx;			//any read breaks NOOPs of previous read
		}
	}
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;

	if (res->num < 1)
		return CMD_DEC_INVALID;	//we need at least address, no number is assumed as 1

	if (res->val[0] != 0xF2)
	{
		if (res->val[1] > ((SPI_MAX_LEN-2) / 2))
			res->val[1] = (SPI_MAX_LEN-2) / 2;
	}
	else
	{
		if (res->val[1] > ((SPI_MAX_LEN-2) / 4))
			res->val[1] = (SPI_MAX_LEN-2) / 4;
	}
	if (res->val[1] == 0)
    	res->val[1] = 1;		//assume as 1 word

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = (uint8_t)0x06;					  //CMD, no length (end of transaction)
	SPIbuf.spiTx[inIdx + 1] = (uint8_t)res->val[0];			//ADDR
	if (res->val[0] != 0xF2)
		for (i = 0; i < res->val[1]; i++)						      //number of words to read
		{
			SPIbuf.spiTx[inIdx + 2 + 2*i] = 0x00;
			SPIbuf.spiTx[inIdx + 3 + 2*i] = 0x00;				    //NOOP
		}
	else
		for (i = 0; i < res->val[1]; i++)						      //number of words to read
		{
			SPIbuf.spiTx[inIdx + 2 + 4*i] = 0x00;
			SPIbuf.spiTx[inIdx + 3 + 4*i] = 0x00;				    //NOOP
			SPIbuf.spiTx[inIdx + 4 + 4*i] = 0x00;				    //NOOP
			SPIbuf.spiTx[inIdx + 5 + 4*i] = 0x00;				    //NOOP
		}

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2;										//NOOPs left
		if (res->val[0] != 0xF2)
		{
			if (SPIbuf.endIdx < (SPIbuf.inIdx + res->val[1]) * 2)
				SPIbuf.endIdx = (SPIbuf.inIdx + res->val[1] * 2);	//end after NOOP
		}
		else
		{
			if (SPIbuf.endIdx < (SPIbuf.inIdx + res->val[1]) * 4)
				SPIbuf.endIdx = (SPIbuf.inIdx + res->val[1] * 4);	//end after NOOP
		}

		return CMD_DEC_OK;
	}

	if (res->val[0] != 0xF2)
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + res->val[1] * 2));
	else
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + res->val[1] * 4));

	/* print the received bytes */
	if (res->val[0] != 0xF2)
		for (i = 0; i < res->val[1]; i++)
		{
			print_log(out, (const char *)"0x%02x%02x ", SPIbuf.spiRx[3 + 2*i], SPIbuf.spiRx[2 + 2*i]);	//little endian
		}
	else
		for (i = 0; i < res->val[1]; i++)
		{
			print_log(out, (const char *)"0x%02x%02x%02x ", SPIbuf.spiRx[4 + 4*i], SPIbuf.spiRx[3 + 4*i], SPIbuf.spiRx[2 + 4*i]);	//little endian
		}
	UART_Send((const char *)"\r\n", 2, out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_wblk(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	unsigned int i;
	int device;				//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	if (res->num < 2)
		return CMD_DEC_INVALID;	//we need at least address and one value to write

	////if (res->num > (CMD_DEC_NUM_VAL + 1))
	////	res->num = (CMD_DEC_NUM_VAL + 1);			//1x address and max FIFO word size

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = (uint8_t)0x01;			//CMD, no length (end of transaction)
	SPIbuf.spiTx[inIdx + 1] = (uint8_t)res->val[0];		//ADDR
	if (res->val[0] != 0xF2)
		for (i = 0; i < (res->num - 1); i++)			//prepare value words to write
		{
			SPIbuf.spiTx[inIdx + 2 + 2*i] = (uint8_t)(res->val[1 + i] & 0xFF);
			SPIbuf.spiTx[inIdx + 3 + 2*i] = (uint8_t)((res->val[1 + i] >> 8) & 0xFF);		//16bit word as little endian
		}
	else
		for (i = 0; i < (res->num - 1); i++)			//prepare value words to write
		{
			SPIbuf.spiTx[inIdx + 2 + 4*i] = (uint8_t)(res->val[1 + i] & 0xFF);
			SPIbuf.spiTx[inIdx + 3 + 4*i] = (uint8_t)((res->val[1 + i] >>  8) & 0xFF);		//24bit word as little endian (32bit)
			SPIbuf.spiTx[inIdx + 4 + 4*i] = (uint8_t)((res->val[1 + i] >> 16) & 0xFF);
			SPIbuf.spiTx[inIdx + 5 + 4*i] = 0;
		}

	if (SPIbuf.concurFlag)
	{
		if (res->val[0] != 0xF2)
			SPIbuf.inIdx += 2 + i * 2;					//no NOOPs left on wreg
		else
			SPIbuf.inIdx += 2 + i * 4;
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx;				//end after all for wreg

		SPIbuf.wasWriteBefore = 1;

		return CMD_DEC_OK;
	}

	if (res->val[0] != 0xF2)
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + (res->num - 1) * 2));
	else
		SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2 + (res->num - 1) * 4));

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_noop(TCMD_DEC_Results *res, EResultOut out)
{
#ifndef VERBOSE
	(void)out;
#endif

	int device;				//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx, num, i;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;	//????

	device = CMD_getSPIoption(res->opt);

	/* generate N numbers of NOOP */
	if (res->val[1] != 0)
		num = res->val[1];
	else
		num = 1;

	/*
	 * prepare the SPI command packet
	 */
	for (i = 0; i < num; i++)
	{
		switch (res->val[0] & 0x0F00)							//we can have 0x0, 0x3, 0x7 as NOOP
		{
			case 0x0300 : SPIbuf.spiTx[inIdx + 0 + 2*i] = 0x03; break;
			case 0x0700 : SPIbuf.spiTx[inIdx + 0 + 2*i] = 0x07; break;
			default     : SPIbuf.spiTx[inIdx + 0 + 2*i] = 0x00; break;				//default
		}

		SPIbuf.spiTx[inIdx + 0 + 2*i] |= (uint8_t)((res->val[0] >> (8-3)) & 0xF8);	//CMD + NOOP code
		SPIbuf.spiTx[inIdx + 1 + 2*i]  = (uint8_t)(res->val[0] & 0xFF);				//NOOP code
	}

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2*num;									//just CMD
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx;						//end after CMD

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, (uint16_t)(2*num));

#if 0
	print_log(out, (const char *)"0x%02x%02x\r\n", SPIbuf.spiRx[1], SPIbuf.spiRx[0]);		//little endian
#endif

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_tir(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int i;
	int device;				//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
	{
		if ( ! (gCFGparams.CfgFlags & 0x00000020))
		{
			if (SPIbuf.endIdx >= 2)
			{
				inIdx = SPIbuf.endIdx -2;
				SPIbuf.inIdx = inIdx;
			}
			else
				inIdx = SPIbuf.inIdx;		//actually, it should be 0 then
		}
		else
		{
			inIdx = SPIbuf.inIdx;
			SPIbuf.endIdx = inIdx;			//any read breaks NOOPs of previous read
		}
	}
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = 0x0D;							//SYSC_TIR code
	SPIbuf.spiTx[inIdx + 1] = 0x00;							//SYSC code 0 for TIR

	/*
	 * add NOOPS
	 */
	for (i = 1; i < (5+1); i++)
	{
		SPIbuf.spiTx[inIdx + 2*i    ] = (uint8_t)0x00;		//fill with NOOPs
		SPIbuf.spiTx[inIdx + 2*i + 1] = (uint8_t)0x00;
	}

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2;									//TIR plus 5 NOOPs
		if (SPIbuf.endIdx < (SPIbuf.inIdx + 10))
			SPIbuf.endIdx = SPIbuf.inIdx + 10;				//end after CMD with 5 NOOPs

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, 2 + 2*5);

	for (i = 0; i < 6; i++)
	{
		/* including the first word received, as SPI Status, SMP_IDX etc. */
		if (i == 2)
		{
			/* print the 32bit Time Stamp */
			print_log(out, (const char *)"0x%02x%02x%02x%02x ", SPIbuf.spiRx[3 + 2*i], SPIbuf.spiRx[2 + 2*i], SPIbuf.spiRx[1 + 2*i], SPIbuf.spiRx[0 + 2*i]);	//little endian
			i++;		//done, jump ahead
			continue;
		}
		print_log(out, (const char *)"0x%02x%02x ", SPIbuf.spiRx[1 + 2*i], SPIbuf.spiRx[0 + 2*i]);	//little endian
	}
	UART_Send((const char *)"\r\n", 2, out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_pgqset(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	int device;								//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 1;

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = 0x85;			//SYSC_PGQSET code, b[6:3] is pppp number
	SPIbuf.spiTx[inIdx + 1] = 0x00;			//SYSC code other

	SPIbuf.spiTx[inIdx + 0] |= (uint8_t)((res->val[0] & 0x0F) << 3);	//add pppp number

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2;					//SYSC PGQSET
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx;	//end after CMD w/o NOOPs

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, 2);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_pgset(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;

	int device;								//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
		inIdx = SPIbuf.inIdx;
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 1;

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = 0x05;			//SYSC_PGSET code, b[6:3] is 0000
	SPIbuf.spiTx[inIdx + 1] = 0x01;			//SYSC PGSET code

	/* possible to set page as 0..255 */
	SPIbuf.spiTx[inIdx + 2] = (uint8_t)(res->val[0] & 0xFF);	//add page number
	SPIbuf.spiTx[inIdx + 3] = (uint8_t)(res->val[0] >> 8);		//little endian, high part

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 4;					//SYSC PGSET
		if (SPIbuf.endIdx < SPIbuf.inIdx)
			SPIbuf.endIdx = SPIbuf.inIdx;	//end after CMD w/o NOOPs

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, 4);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CHIP_pgget(TCMD_DEC_Results *res, EResultOut out)
{
	int device;				//default is 2 [-P] if not specified via -T or -A
	unsigned int inIdx;

	if (SPIbuf.concurFlag)
	{
		if ( ! (gCFGparams.CfgFlags & 0x00000020))
		{
			if (SPIbuf.endIdx >= 2)
			{
				inIdx = SPIbuf.endIdx -2;
				SPIbuf.inIdx = inIdx;
			}
			else
				inIdx = SPIbuf.inIdx;		//actually, it should be 0 then
		}
		else
		{
			inIdx = SPIbuf.inIdx;
			SPIbuf.endIdx = inIdx;			//any read breaks NOOPs of previous read
		}
	}
	else
		inIdx = 0;

	SPIbuf.wasWriteBefore = 0;

	device = CMD_getSPIoption(res->opt);

	/*
	 * prepare the SPI command packet
	 */
	SPIbuf.spiTx[inIdx + 0] = 0x85;			//SYSC_PGGET code
	SPIbuf.spiTx[inIdx + 1] = 0x01;			//SYSC_PGGET code
	SPIbuf.spiTx[inIdx + 2] = 0x00;			//NOOP
	SPIbuf.spiTx[inIdx + 3] = 0x00;			//NOOP

	if (SPIbuf.concurFlag)
	{
		SPIbuf.inIdx += 2;						//SYSC PGGET
		if (SPIbuf.endIdx < (SPIbuf.inIdx + 2))
			SPIbuf.endIdx = SPIbuf.inIdx + 2;	//end after CMD with 1x NOOP

		return CMD_DEC_OK;
	}

	SPI_transaction(device, SPIbuf.spiTx, SPIbuf.spiRx, 4);

	/* print the received bytes as page number */
#ifdef VERBOSE
	print_log(out, (const char *)"PGNUM: | ");
#endif

	print_log(out, (const char *)"0x%02x%02x\r\n", SPIbuf.spiRx[3], SPIbuf.spiRx[2]);		//little endian

	return CMD_DEC_OK;
}

/* ------------------------------------------------------------------- */

#define TEXT_VAL_LEN		32
#define SPI_CHUNK_LEN		1024  //(2*16*1024)		//16K chunks, 16bit words
#if 0
unsigned long GMaxFileLen = SPI_CHUNK_LEN;
#endif

ECMD_DEC_Status CHIP_sdload(TCMD_DEC_Results *res, EResultOut out)
{
	int device;					        //default is 0 [-P] if not specified or -A
	const char *fname;			    //pointer to filename after <IDX spaces>
	File MyFile;     				    /* File object */
	int val, eof;
	unsigned long chunkLen;
	uint8_t *spiTx, *spiRx;			/* buffer for SPI transaction */
	int wordOff;					      /* for ADC value shift to adjust clock offset */
	////int halfOff;
	int prefix;						      /* to control if we insert ADC=0 in front */
	int totalWords;					    /* control max. size */

	static char fileLine[TEXT_VAL_LEN];	/* buffer for single line */

	device = CMD_getSPIoption(res->opt);

	/*
	 * syntax: sdload [-P|-A|-T] <IDX> <filename>
	 * set the pointer fname to start of filename
	 *
	 * NEW: sdload [-P|-A|-T] <IDX> [CLKOFF] <filename>
	 * specify an optional clock offset to shift the ADC stimulus (1/2 clock vise)
	 */

	fname = CMD_nextStr(res->str);

	/* open the ASCII file with values to write */
	if (SDCARD_GetStatus() && fname)
	{
    MyFile = SD.open(fname);
    if ( ! MyFile)
		{
			UART_Send((const char *)"*E: no file\r\n", 11, out);
			SYSERR_Set(out, SYSERR_FILE);

			return CMD_DEC_ERROR;			//error
		}
		else
		{
			spiTx = (uint8_t *)MEM_PoolAlloc(SPI_CHUNK_LEN + 2);
			if ( ! spiTx)
			{
				return CMD_DEC_OOMEM;			//error
			}
			spiRx = (uint8_t *)MEM_PoolAlloc(SPI_CHUNK_LEN + 2);
			if ( ! spiRx)
			{
				MEM_PoolFree(spiTx);
				return CMD_DEC_OOMEM;			//error
			}

			chunkLen = 0;
			*(spiTx + 0) = (uint8_t)0x01;			    //WRBLK command
			*(spiTx + 1) = (uint8_t)res->val[0];	//IDX into command
			eof = 0;

			wordOff = 0;                          //TODO: config
			if (wordOff > (SPI_CHUNK_LEN / 2))
				wordOff = SPI_CHUNK_LEN / 2 - 1;
			////halfOff = (int)(gCFGparams.TestFlags % 2);

			prefix = 0;
			totalWords = 0;

			while (1)
			{
				/* loop over 16K words as one chunk, increment IDX on chunks */

				/* NEW: should we add zero samples in the beginning? */
				if (prefix == 0)
				{
					if (wordOff)
					{
						/* insert dummy ADC=0 values in front */
						memset(spiTx + 2, 0, wordOff * 2);
						chunkLen   = wordOff * 2;
						totalWords = wordOff;
					}
					prefix = 1;
				}

				while (chunkLen < SPI_CHUNK_LEN)
				{
					if (SDCARD_fgets(fileLine, TEXT_VAL_LEN -1, &MyFile) != 0)
					{
						/* allow to have commented lines - must be a '#' at first position in line */
						if (fileLine[0] == '#')
							continue;
						/* check if the value is there and it could be converted, handle empty lines */
						if (sscanf(fileLine, "%i", &val) > 0)
						{
							*(spiTx + 2 + chunkLen) = (uint8_t)(val & 0xFF);		//LO, little endian values
							*(spiTx + 3 + chunkLen) = (uint8_t)((val >> 8) & 0xFF);	//HI (or middle)
							if (res->val[0] == 0xF2)
							{
								//load 24bit values as LE
								*(spiTx + 4 + chunkLen) = (uint8_t)((val >> 16) & 0xFF);	//MSB, little endian values
								*(spiTx + 5 + chunkLen) = 0;								//HI as 0
								chunkLen += 2;
								totalWords++;
							}
							chunkLen += 2;
							totalWords++;
#if 0
							if (totalWords >= MAX_ADC_MEM_SIZE)
							{
								eof = 1;
								break;
							}
#endif

#if 0
							//for testing - load just part of the file
							if ((chunkLen / 2) >= GMaxFileLen)
							{
								eof = 1;
								break;
							}
#endif
						}
					}
					else
					{
						eof = 1;
						break;				//end of file
					}
				}

				/* send SPI transaction but just if at least one value is there */
				if (chunkLen)
				{
#if 0
					if (gCFGparams.DebugFlags & DBG_VERBOSE)
#endif
					  /* do it always, also on HTTPD_OUT */
					  print_log(out, (const char *)"*I: WBLK | IDX: 0x%02x bytes: %ld\r\n", (unsigned int)res->val[0], (long)chunkLen);

					SPI_transaction(device, spiTx, spiRx, chunkLen + 2);
					chunkLen = 0;							//next chunk
					res->val[0]++;							//increment IDX
					*(spiTx + 1) = (uint8_t)res->val[0];	//update in SPI command
				}

				if (eof)
					break;
			}

			MyFile.close();
			MEM_PoolFree(spiTx);
			MEM_PoolFree(spiRx);

			return CMD_DEC_OK;
		}
	}
	else
		return CMD_DEC_ERROR;
}

void CHIP_decodePRI_FIFO(uint8_t *ptr, uint16_t len, EResultOut out, int par)
{
#define PRI_FIFO_1ST_WORD_OFF	2		  //skip the first two words for RREG and RBLK command
#define PRI_FIFO_MAX_ENTRIES	80		//max. number of valid entries
#define PRI_FIFO_ENTRY_BYTES	12		//12 bytes, 6 words per entry

	int numEntries = 0;
	int i = 0;
	int xLen = (int)len;

	if (len < (PRI_FIFO_1ST_WORD_OFF * 2))
		return;

	ptr += PRI_FIFO_1ST_WORD_OFF * 2;
	xLen -= (uint16_t)(PRI_FIFO_1ST_WORD_OFF * 2);

	while ((xLen > 0) && (i < PRI_FIFO_MAX_ENTRIES))
	{
		if (*(ptr + (PRI_FIFO_ENTRY_BYTES-1)) & 0x80)
		{
			//valid bit is set
			if (par)
				//DV format like
				print_log(out, (const char *)"[%d]=%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", numEntries,
						*(ptr+11), *(ptr+10), *(ptr+9), *(ptr+8), *(ptr+7), *(ptr+6), *(ptr+5), *(ptr+4), *(ptr+3), *(ptr+2), *(ptr+1), *(ptr+0));
			else
				print_log(out, (const char *)"%01x %01x %01x %01x %01x %02x %01x %02x%02x%02x %02x%02x%02x %02x%02x%02x\r\n",
					(*(ptr+11) >> 4),										//flags
					(*(ptr+11) & 0xF),										//SLOT
					(*(ptr+10) >> 6),										//RX_IDX
					((*(ptr+10) & 0x38) >> 3),								//flags
					((*(ptr+10) & 0x7) << 1) | ((*(ptr+9) & 0x80) >> 7),	//CFG_ID
					((*(ptr+9) & 0x7C) >> 2),								//BEAT_IDX
					(*(ptr+9) & 0x3),										//flags
					*(ptr+8), *(ptr+7), *(ptr+6),							//DARK_DATA
					*(ptr+5), *(ptr+4), *(ptr+3),							//DATA_DQ
					*(ptr+2), *(ptr+1), *(ptr+0));							//DATA_DI
			numEntries++;
		}
		ptr  += PRI_FIFO_ENTRY_BYTES;
		xLen -= PRI_FIFO_ENTRY_BYTES;
		i++;
	}

	print_log(out, (const char *)"Entries: %d\r\n", numEntries);
}

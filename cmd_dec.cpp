/**
 * command decoder
 */

#include <string.h>

#include "MEM_Pool.h"
#include "cmd_dec.h"
#include "debug_sys.h"
#include "CMD_thread.h"

#include "SPI_dev.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "TFTP.h"
#include "picoc.h"

/* prototypes */
ECMD_DEC_Status CMD_help(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_print(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_sdinit(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results* res, EResultOut out);
#ifdef WITH_SDCARD
ECMD_DEC_Status CMD_sdexec(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sdformat(TCMD_DEC_Results* res, EResultOut out);
#endif
#ifdef WITH_I2C
ECMD_DEC_Status CMD_i2cwrr(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_i2crdr(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_i2cclk(TCMD_DEC_Results* res, EResultOut out);
#endif

ECMD_DEC_Status CMD_rawspi(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_spiclk(TCMD_DEC_Results* res, EResultOut out);

ECMD_DEC_Status CMD_syscfg(TCMD_DEC_Results* res, EResultOut out);

ECMD_DEC_Status CMD_tftp(TCMD_DEC_Results* res, EResultOut out);

ECMD_DEC_Status CMD_ipaddr(TCMD_DEC_Results* res, EResultOut out);

ECMD_DEC_Status CMD_picoc(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_delay(TCMD_DEC_Results *res, EResultOut out);

const TCMD_DEC_Command Commands[] = {
		{
				.cmd = (const char *)"help",
				.help = (const char *)"[cmd] list of all defined commands or help for cmd",
				.func = CMD_help
		},
		{
				.cmd = (const char *)"print",
				.help = (const char *)"print [-n] [rest of cmd]",
				.func = CMD_print
		},
    {
				.cmd = (const char *)"delay",
				.help = (const char *)"delay [ms]",
				.func = CMD_delay
		},
    {
				.cmd = (const char*)"sdinit",
				.help = (const char*)"initialize <1> or deinit SDCard",
				.func = CMD_sdinit
		},
    {
				.cmd = (const char*)"sddir",
				.help = (const char*)"print SD Card content",
				.func = CMD_sddir
		},
		{
				.cmd = (const char*)"sdprint",
				.help = (const char*)"sdprint <fname> as text file",
				.func = CMD_sdprint
		},
#ifdef WITH_SDCARD
		{
				.cmd = (const char*)"sdexec",
				.help = (const char*)"sdexec <fname> execute as script file",
				.func = CMD_sdexec
		},
		{
				.cmd = (const char*)"sdformat",
				.help = (const char*)"format SD card - content lost",
				.func = CMD_sdformat
		},
#endif
#ifdef WITH_I2C
		{
				.cmd = (const char*)"i2cwrr",
				.help = (const char*)"I2C write register <slaveAddr> <regAddr> <byte ...>",
				.func = CMD_i2cwrr
		},
		{
				.cmd = (const char*)"i2crdr",
				.help = (const char*)"I2C read register <slaveAddr> <regAddr> <num>",
				.func = CMD_i2crdr
		},
		{
				.cmd = (const char*)"i2cclk",
				.help = (const char*)"set I2C clock speed [0..3]",
				.func = CMD_i2cclk
		},
#endif

    {
				.cmd = (const char*)"spiclk",
				.help = (const char*)"set SPI clock <hz>, 0: display current",
				.func = CMD_spiclk
		},
		{
				.cmd = (const char*)"rawspi",
				.help = (const char*)"send bytes via SPI <byte ...>",
				.func = CMD_rawspi
		},
		{
				.cmd = (const char*)"syscfg",
				.help = (const char*)"print sys config parameters",
				.func = CMD_syscfg
		},
    {
				.cmd = (const char*)"tftp",
				.help = (const char*)"start/stop TFTP server [1|0]",
				.func = CMD_tftp
		},
    {
				.cmd = (const char*)"ipaddr",
				.help = (const char*)"display the MCU IP address",
				.func = CMD_ipaddr
		},
    {
				.cmd = (const char*)"picoc",
				.help = (const char*)"start Pico-C interpreter",
				.func = CMD_picoc
		},
};

static unsigned int CMD_DEC_findCmd(const char *cmd, unsigned int len)
{
	size_t i;
	for (i = 0; i < (sizeof(Commands)/sizeof(TCMD_DEC_Command)); i++)
	{
		if (Commands[i].func == NULL)
			continue;
		/* if length does not match - keep going */
		if (len != (unsigned int)strlen(Commands[i].cmd))
			continue;
		if (strncmp(cmd, Commands[i].cmd, (size_t)len) == 0)
			return (unsigned int)i;
	}

	return (unsigned int)-1;
}

static void CMD_DEC_DecodeError(ECMD_DEC_Status err, EResultOut out)
{
	/*
	 * TODO: decode command errors - empty for now, silently ignored on errors
	 */
	(void)err;
	(void)out;

	return;
}

static unsigned int CMD_DEC_decode(char *cmdStr, TCMD_DEC_Results *res)
{
	unsigned int i;
	int state;
	char *xCmdStr = cmdStr;

	/* set default results */
	res->cmd = NULL;
	res->cmdLen = 0;
	res->offset = 0;
	res->opt = NULL;
	res->str = NULL;
	res->ctl = 0;
	res->num = 0;
	/* set all values to 0 as default, if omitted but used */
	for (i = 0; i < CMD_DEC_NUM_VAL; i++)
		res->val[i] = 0L;

	state = 0;
	i = 0;
	while ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
	{
		/* skip leading spaces, tabs */
		while ((*cmdStr == ' ') || (*cmdStr == '\t'))
		{
			if ((state == 1) || (state == 3))
			{
				////*cmdStr = '\0';
				state++;
			}
			if (state == 5)
				state--;

			cmdStr++;
		}

		switch (state)
		{
		case 0:	/* find command keyword	*/
			res->cmd = cmdStr;
			////res->cmdLen = 0;
			state = 1;
			break;
		case 1:	/* find end of keyword */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
			{
				cmdStr++;
				res->cmdLen++;
			}
			break;
		case 2:	/* check if option is there */
			if (*cmdStr == CMD_DEC_OPT_SIGN)
			{
				res->opt = cmdStr;
				state = 3;
			}
			else
				state = 4;
			break;
		case 3:	/* find end of option string */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
				cmdStr++;
			break;
		case 4: /* now we scan just values or option value */
			if (i < CMD_DEC_NUM_VAL)
			{
				if (i == 0)
					if ( ! res->str)
						res->str = cmdStr;
				/*
				 * ATTENTION: this sscanf just signed values (decimal or hex), values above
				 * 0x8000000 results in a wrong value!
				 */

				////fixed, and add '$' for user variable
				if (*cmdStr == '$')
				{
					//res->val[i] = usrVar;
					i++;
				}
				else
				{
					if ((cmdStr[1] == 'x') || (cmdStr[1] == 'X'))
					{
						if (sscanf(cmdStr, "%lx", (unsigned long *)(&res->val[i])) == 1)
							i++;
					}
					else
					{
						if (sscanf(cmdStr, "%lu", (unsigned long *)(&res->val[i])) == 1)
							i++;
					}
				}

				res->num = i;
			}
			state = 5;
			break;
		case 5:	/* skip value characters */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != CMD_DEC_SEPARATOR) && (*cmdStr != CMD_DEC_COMMENT))
				cmdStr++;
			break;
		}
	} /* end while */

	if (*cmdStr == CMD_DEC_SEPARATOR)
	{
		////*cmdStr++ = '\0';
		cmdStr++;
		res->offset = (unsigned long)(cmdStr - xCmdStr);
		return (unsigned int)(cmdStr - xCmdStr);
	}

	if (*cmdStr == CMD_DEC_COMMENT)
		*cmdStr = '\0';

	if (res->cmd)
		if (*res->cmd == '\0')
			res->cmd = NULL;

	if ((*cmdStr == '\r') || (*cmdStr == '\n'))
		*cmdStr = '\0';

	return 0;
}

ECMD_DEC_Status CMD_DEC_execute(char *cmd, EResultOut out)
{
	TCMD_DEC_Results *cmdRes;
	unsigned int res, offset, idx;

	cmdRes = (TCMD_DEC_Results *)MEM_PoolAlloc(sizeof(TCMD_DEC_Results));
	if ( ! cmdRes)
	{
		return CMD_DEC_OOMEM;
	}

	offset = 0;
	do
	{
		res = CMD_DEC_decode(cmd + offset, cmdRes);
		offset += res;

		if (cmdRes->cmd)
			if (*cmdRes->cmd != '\0')	/* ignore empty line, just spaces */
			{
				idx = CMD_DEC_findCmd(cmdRes->cmd, cmdRes->cmdLen);
				if (idx != (unsigned int)-1)
				{
					ECMD_DEC_Status err;

					err = Commands[idx].func(cmdRes, out);

					/* decode the err code */
					CMD_DEC_DecodeError(err, out);
				}
				else
				{
					UART_Send((const char *)"*E: unknown >", 13, out);
					UART_Send((const char *)cmdRes->cmd, cmdRes->cmdLen, out);
					UART_Send((const char *)"<", 1, out);
					UART_Send((const char *)"\r\n", 2, out);
					//SYS_SetError(SYS_ERR_INVALID_CMD);
				}
			}

		if (cmdRes->ctl == 1)
			break;

	} while (res);

	MEM_PoolFree(cmdRes);
	return CMD_DEC_OK;
}

/* helper function for help - keyword length */
static unsigned int CMD_keywordLen(const char *str)
{
	unsigned int l = 0;

	while (*str)
	{
		if ((*str == ';') || (*str == ' ') || (*str == '\r') || (*str == '\n'))
			break;
		str++;
		l++;
	}

	return l;
}

/* helper function for print - string length up to ';' or '#' */
static unsigned int CMD_lineLen(const char *str)
{
	unsigned int l = 0;

	while (*str)
	{
		if ((*str == ';') || (*str == '#') || (*str == '\r') || (*str == '\n'))
			break;
		str++;
		l++;
	}

	return l;
}

#if 0
/* helper function to find string <filename> after a value <IDX> */
static const char * CMD_nextStr(const char *str)
{
	const char *resStr = NULL;		/* set to invalid result */

	/* find space after <IDX> */
	while (*str)
	{
		if ((*str == ' ') || (*str == '\r') || (*str == '\n'))
			break;
		str++;
	}

	/* find start of <filename> as not a space */
	while (*str)
	{
		if (*str != ' ')
		{
			resStr = str;
			break;
		}
		str++;
	}

	return resStr;
}
#endif

/* other helper functions */

void hex_dump(unsigned char *ptr, int len, int mode, EResultOut out)
{
	int i = 0;
	int xLen = len;

#if 0
	if (ptr == NULL)
		return;
#endif
	if (len == 0)
		////return;
		len = 1;

	////if (out == UART_OUT)
	{
		print_log(UART_OUT, "%08lx | ", (unsigned long)ptr);
		while (xLen > 0)
		{
			if (mode == 1)
			{
				//bytes
				//TODO: we could an ASCII character view
				print_log(out, (const char *)"%02X ", (int)*ptr);
				ptr++;
				xLen--;
				i++;
			}
			else
			if (mode == 2)
			{
				//short words, little endian
				print_log(out, (const char *)"%04X ", (int)(*ptr | (*(ptr + 1) << 8)));
				ptr += 2;
				xLen -= 2;
				i += 2;
			}
			else
			if (mode == 4)
			{
				//32bit words, little endian
				print_log(out, (const char *)"%08lX ", (unsigned long)(*ptr |
																	  (*(ptr + 1) << 8) |
																	  (*(ptr + 2) << 16) |
																	  (*(ptr + 3) << 24)
																	 ));
				ptr += 4;
				xLen -= 4;
				i += 4;
			}
			else
				break;		//just for now to make it safe

			if ((i % 16) == 0)
			{
#if 1
				{
					//optional: print ASCII characters
					int j;
					print_log(UART_OUT, "  ");
					for (j = 16; j > 0; j--)
					{
						if ((*(ptr - j) >= ' ') && (*(ptr - j) < 0x7F))
							print_log(UART_OUT, "%c", *(ptr - j));
						else
							print_log(UART_OUT, ".");
					}
				}
#endif
				if (xLen)
					print_log(UART_OUT, "\r\n%08lx | ", (unsigned long)ptr);
				else
					UART_Send((const char*)"\r\n", 2, out);
			}
		}

		if ((i % 16) != 0)
			UART_Send((const char *)"\r\n", 2, out);
	}
}

/**
 * ----------------------------------------------------------------------------
 */

ECMD_DEC_Status CMD_help(TCMD_DEC_Results *res, EResultOut out)
{
	unsigned int idx;

	if (res->str)
	{
		/* we have 'help <CMD>' - print just the single line help */
		idx = CMD_DEC_findCmd(res->str, CMD_keywordLen(res->str));
		if (idx != (unsigned int)-1)
			print_log(out, (const char *)"%-10s: %s\r\n", Commands[idx].cmd, Commands[idx].help);
		else
		{
			UART_Send((const char *)"*E: unknown\r\n", 13, out);
			return CMD_DEC_UNKNOWN;
		}
	}
	else
	{
		/* just 'help' - print entire list */
		for (idx = 0; (size_t)idx < (sizeof(Commands)/sizeof(TCMD_DEC_Command)); idx++)
			print_log(out, (const char *)"%-10s: %s\r\n", Commands[idx].cmd, Commands[idx].help);
	}

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_print(TCMD_DEC_Results *res, EResultOut out)
{
	if (res->str)
		UART_Send((const char *)res->str, CMD_lineLen(res->str), out);
	if (res->opt)
	{
		if (strncmp(res->opt, "-n", 2) == 0)
		{
			/* no newline to add */
		}
		else
			UART_Send((const char *)"\r\n", 2, out);
	}
	else
		UART_Send((const char *)"\r\n", 2, out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_delay(TCMD_DEC_Results *res, EResultOut out)
{
  if (res->val[0])
    CMD_delay((int)res->val[0]);

  return CMD_DEC_OK;
}

#ifdef WITH_SDCARD
ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results* res, EResultOut out)
{
	(void)res;

	SDCARD_ShowDir(out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results* res, EResultOut out)
{
	SDCARD_PrintFile(out, res->str);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdexec(TCMD_DEC_Results* res, EResultOut out)
{
	SDCARD_Exec(out, res->str);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdformat(TCMD_DEC_Results* res, EResultOut out)
{
	(void)res;
	SDCARD_Format(out);

	return CMD_DEC_OK;
}
#endif

#ifdef WITH_I2C
ECMD_DEC_Status CMD_i2cwrr(TCMD_DEC_Results* res, EResultOut out)
{
	(void)out;
	int i;

	if (res->num < 3)
		return CMD_DEC_INVPARAM;

	for (i = 0; (i < sizeof(I2CBuffer)) && (i < (res->num - 1)); i++)
		I2CBuffer[i] = (uint8_t)res->val[1 + i];

	I2C_Write((uint8_t)res->val[0], I2CBuffer, i);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2crdr(TCMD_DEC_Results* res, EResultOut out)
{
	int i, l;

	if (res->num < 2)
		return CMD_DEC_INVPARAM;

	i = res->val[2];
	if (i > sizeof(I2CBuffer))
		i = sizeof(I2CBuffer);
	if (i == 0)
		i = 1;
	l = I2C_MemRead((uint8_t)res->val[0], (uint8_t)res->val[1], I2CBuffer, (size_t)i);
	for (i = 0; i < l; i++)
		print_log(out, "%02x ", I2CBuffer[i]);
	print_log(out, "\r\n");

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_i2cclk (TCMD_DEC_Results* res, EResultOut out)
{
	(void)out;

	I2C_ClockSpeed(res->val[0]);

	return CMD_DEC_OK;
}
#endif

/* mapping for needed functions */
void  UART_Send(const char* str, int chrs, EResultOut out) {
  while (chrs--) {
    Serial.write(*str++);
  }
}

void  UART_SendStr(const char* str, EResultOut out) {
  Serial.print(str);
}

ECMD_DEC_Status CMD_rawspi(TCMD_DEC_Results* res, EResultOut out) {

  unsigned char *SPIbufTx, *SPIbufRx;
  unsigned long i;

  SPIbufTx = (unsigned char *)MEM_PoolAlloc(CMD_DEC_NUM_VAL);
  if (SPIbufTx) {
    SPIbufRx = (unsigned char *)MEM_PoolAlloc(CMD_DEC_NUM_VAL);
    if ( ! SPIbufRx) {
      MEM_PoolFree(SPIbufTx);
      return CMD_DEC_OOMEM;
    }
  }
  else {
    return CMD_DEC_OOMEM;
  }

  //convert the SPI words to a byte array;
  for (i = 0; i < res->num; i++)
    *(SPIbufTx + i) = (unsigned char)res->val[i];

  SPI_transaction(0, SPIbufTx, SPIbufRx, res->num);

  hex_dump((unsigned char *)SPIbufRx, res->num, 1, UART_OUT);

  MEM_PoolFree(SPIbufTx);
  MEM_PoolFree(SPIbufRx);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_spiclk(TCMD_DEC_Results* res, EResultOut out) {
  int spiclk;
  if (res->val[0]) {
    SPI_setClock((int)res->val[0]);
  }
  else {
    spiclk = SPI_setClock(0);
    print_log(out, (const char *)"SPI clock: %d\r\n", spiclk);
  }
  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdinit(TCMD_DEC_Results* res, EResultOut out) {
  if (res->val[0])
    SDCARD_setup();
  else
    SDCARD_deinit();

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results* res, EResultOut out) {
  SDCARD_printDirectory("/", 0);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_syscfg(TCMD_DEC_Results* res, EResultOut out) {
  if (res->opt) {
    if (strncmp(res->opt, "-d", 2) == 0) {
      CFG_SetDefault();
      return CMD_DEC_OK;
    }
    if (strncmp(res->opt, "-w", 2) == 0) {
      CFG_Write();
      return CMD_DEC_OK;
    }
  }

  CFG_Print();

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_tftp(TCMD_DEC_Results* res, EResultOut out) {
  if (res->val[0])
    TFTP_setup();
  else
    TFTP_kill();

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_ipaddr(TCMD_DEC_Results* res, EResultOut out) {
  extern uint32_t HTTPD_GetIPAddress(void);
  uint32_t ipAddr;

  ipAddr = HTTPD_GetIPAddress();
  print_log(out, (const char *)"IP address: %ld.%ld.%ld.%ld\r\n",
    (ipAddr >> 0) & 0xFF, (ipAddr >> 8) & 0xFF, (ipAddr >> 16) & 0xFF, (ipAddr >> 24) & 0xFF);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_picoc(TCMD_DEC_Results *res, EResultOut out)
{
	pico_c_main_interactive(0, NULL);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results *res, EResultOut out)
{
	SDCARD_PrintFile(res->str);

	return CMD_DEC_OK;
}

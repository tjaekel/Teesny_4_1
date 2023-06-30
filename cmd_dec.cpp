/**
 * command decoder
 */

#include <string.h>
////#include <core_cm7.h>       //for "fwreset" - does not work! - files and defines for FPU missing, compile error!

#include "MEM_Pool.h"
#include "cmd_dec.h"
#include "define_sys.h"
#include "CMD_thread.h"
#include "SPI_dev.h"
#include "SD_Card.h"
#include "SYS_config.h"
#include "picoc.h"
#include "GPIO.h"
#include "UDP_send.h"
#include "TCP_Server.h"
#include "SYS_error.h"

/* prototypes */
ECMD_DEC_Status CMD_help(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_sysinfo(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_syserr(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_debug(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_print(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_repeat(TCMD_DEC_Results *res, EResultOut out);

#ifdef WITH_SDCARD
ECMD_DEC_Status CMD_sdinit(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sdexec(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_sdformat(TCMD_DEC_Results* res, EResultOut out);
#endif
#ifdef WITH_I2C
ECMD_DEC_Status CMD_i2cwrr(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_i2crdr(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_i2cclk(TCMD_DEC_Results* res, EResultOut out);
#endif

ECMD_DEC_Status CMD_rawspi(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_spitr(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_spiclk(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_syscfg(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_setcfg(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_fwreset(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_ipaddr(TCMD_DEC_Results* res, EResultOut out);
ECMD_DEC_Status CMD_picoc(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_picocExec(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_delay(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_udptest(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_udpip(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_pstat(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_cstat(TCMD_DEC_Results *res, EResultOut out);

ECMD_DEC_Status CMD_cgpio(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_pgpio(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_ggpio(TCMD_DEC_Results *res, EResultOut out);
ECMD_DEC_Status CMD_res(TCMD_DEC_Results *res, EResultOut out);

const TCMD_DEC_Command Commands[] = {
		{
				.cmd = (const char *)"help",
				.help = (const char *)"list of all defined commands or help for [cmd]",
				.func = CMD_help
		},
    {
				.cmd = (const char *)"sysinfo",
				.help = (const char *)"display version and systrem info",
				.func = CMD_sysinfo
		},
    {
				.cmd = (const char *)"syserr",
				.help = (const char *)"display sys error [-d]",
				.func = CMD_syserr
		},
    {
				.cmd = (const char *)"debug",
				.help = (const char *)"set debug flags <val>",
				.func = CMD_debug
		},
		{
				.cmd = (const char *)"print",
				.help = (const char *)"print [-n] [rest of cmd]",
				.func = CMD_print
		},
    {
				.cmd = (const char *)"repeat",
				.help = (const char *)"repeat cmd [-0|-n] endless or n times",
				.func = CMD_repeat
		},
    {
				.cmd = (const char *)"delay",
				.help = (const char *)"delay [ms]",
				.func = CMD_delay
		},
#ifdef WITH_SDCARD
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
				.help = (const char*)"print <fname> as text file",
				.func = CMD_sdprint
		},
		{
				.cmd = (const char*)"sdexec",
				.help = (const char*)"execute <fname> as script file",
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
				.help = (const char*)"send bytes via SPI [-P|-A] <byte ...>",
				.func = CMD_rawspi
		},
    {
				.cmd = (const char*)"spitr",
				.help = (const char*)"send words via SPI [-P|-A] <word ...>",
				.func = CMD_spitr
		},
		{
				.cmd = (const char*)"syscfg",
				.help = (const char*)"print sys config, [-d] set default, [-w] write current",
				.func = CMD_syscfg
		},
		{
				.cmd = (const char*)"fwreset",
				.help = (const char*)"reset MCU and restart",
				.func = CMD_fwreset
		},
		{
				.cmd = (const char*)"setcfg",
				.help = (const char*)"set config <idx> [val]",
				.func = CMD_setcfg
		},
    {
				.cmd = (const char*)"pstat",
				.help = (const char*)"display system counters [-P|-A]",
				.func = CMD_pstat
		},
    {
				.cmd = (const char*)"cstat",
				.help = (const char*)"clear system counters [-P|-A]",
				.func = CMD_cstat
		},
    {
				.cmd = (const char*)"ipaddr",
				.help = (const char*)"display the MCU IP address",
				.func = CMD_ipaddr
		},
    {
				.cmd = (const char*)"udpip",
				.help = (const char*)"set host UDP address",
				.func = CMD_udpip
		},
    {
				.cmd = (const char*)"picoc",
				.help = (const char*)"init/delete Pico-C interpreter, [-i|-d]",
				.func = CMD_picoc
		},
    {
				.cmd = (const char*)"c:",
				.help = (const char*)"execute Pico-C statement from shell",
				.func = CMD_picocExec
		},
    {
				.cmd = (const char*)"udptest",
				.help = (const char*)"test: send UDP packet",
				.func = CMD_udptest
		},
    {
				.cmd = (const char*)"cgpio",
				.help = (const char*)"configure GPIOs <dirmask> [odmask]",
				.func = CMD_cgpio
		},
    {
				.cmd = (const char*)"ggpio",
				.help = (const char*)"get GPIO inputs",
				.func = CMD_ggpio
		},
    {
				.cmd = (const char*)"pgpio",
				.help = (const char*)"set GPIO output <mask>",
				.func = CMD_pgpio
		},
    {
				.cmd = (const char*)"res",
				.help = (const char*)"set RESET pin <0|1>",
				.func = CMD_res
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
  char cmd_separator = CMD_DEC_SEPARATOR;

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

  if (*cmdStr == '|') {
    cmd_separator = 0xff;
    cmdStr++;
    xCmdStr = cmdStr;
    res->ctl = 1;
  }

	while ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != cmd_separator) && (*cmdStr != CMD_DEC_COMMENT))
	{
		/* skip leading spaces, tabs */
		while ((*cmdStr == ' ') || (*cmdStr == '\t'))
		{
			if ((state == 1) || (state == 3))
			{
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
			state = 1;
			break;
		case 1:	/* find end of keyword */
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != cmd_separator) && (*cmdStr != CMD_DEC_COMMENT))
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
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != cmd_separator) && (*cmdStr != CMD_DEC_COMMENT))
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
			if ((*cmdStr != '\0') && (*cmdStr != '\r') && (*cmdStr != '\n') && (*cmdStr != cmd_separator) && (*cmdStr != CMD_DEC_COMMENT))
				cmdStr++;
			break;
		}
	} /* end while */

	if (*cmdStr == cmd_separator)
	{
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
					
          SYSERR_Set(out, SYSERR_CMD);
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
static unsigned int CMD_lineLen(const char *str, int entireLine)
{
	unsigned int l = 0;
  char cmdDelimiter = CMD_DEC_SEPARATOR;

  if (entireLine)
    cmdDelimiter = 0xff;

	while (*str)
	{
		if ((*str == cmdDelimiter) || (*str == '#') || (*str == '\r') || (*str == '\n'))
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
		print_log(out, "%08lx | ", (unsigned long)ptr);
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
					print_log(out, "  ");
					for (j = 16; j > 0; j--)
					{
						if ((*(ptr - j) >= ' ') && (*(ptr - j) < 0x7F))
							print_log(out, "%c", *(ptr - j));
						else
							print_log(out, ".");
					}
				}
#endif
				if (xLen)
					print_log(out, "\r\n%08lx | ", (unsigned long)ptr);
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

ECMD_DEC_Status CMD_sysinfo(TCMD_DEC_Results *res, EResultOut out)
{
  int inUse, watermark, max;
  print_log(out, "FW version   : %s\r\n", VERSION_NUMBER);
  MEM_PoolCounters(&inUse, &watermark, &max);
  print_log(out, "MEMPool      : %d | %d | %d\r\n", inUse, watermark, max);
  print_log(out, "HTTP clients : %d\r\n", HTTPD_GetClientNumber());
  print_log(out, "ETH link     : %d\r\n", HTTPD_GetETHLinkState());

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_syserr(TCMD_DEC_Results *res, EResultOut out)
{
  unsigned long err;

  if (res->opt) {
    if (strncmp(res->opt, "-d", 2) == 0)
      SYSERR_Get(out, 1);
  }
  else {
    err = SYSERR_Get(out, 0);
    print_log(out, "*I: SYS error: %lx\r\n", err);
  }

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_debug(TCMD_DEC_Results *res, EResultOut out) {
  (void)out;
  gCFGparams.DebugFlags = res->val[0];

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_print(TCMD_DEC_Results *res, EResultOut out)
{
	if (res->str)
		UART_Send((const char *)res->str, CMD_lineLen(res->str, res->ctl), out);
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
void UART_Send(const char* str, int chrs, EResultOut out) {
  while (chrs--) {
    UART_printChar(*str++, out);
  }
}

void UART_SendStr(const char* str, EResultOut out) {
  UART_printString(str, out);
}

ECMD_DEC_Status CMD_rawspi(TCMD_DEC_Results* res, EResultOut out) {
  unsigned char *SPIbufTx, *SPIbufRx;
  unsigned long i;
  int dev = 0;

  if (res->num == 0)
		return CMD_DEC_INVALID;	//we need at least one byte

  if (res->opt) {
    if (strncmp(res->opt, "-A", 2) == 0)
      dev = 1;
  }

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

  SPI_transaction(dev, SPIbufTx, SPIbufRx, res->num);

  ////hex_dump((unsigned char *)SPIbufRx, res->num, 1, out);
  {
    unsigned long i;
    for (i = 0; i < res->num; i++) {
      print_log(out, "%02x ", *(SPIbufRx + i));
    }
    UART_Send((const char *)"\r\n", 2, out);
  }

  MEM_PoolFree(SPIbufTx);
  MEM_PoolFree(SPIbufRx);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_spitr(TCMD_DEC_Results* res, EResultOut out) {
  unsigned char *SPIbufTx, *SPIbufRx;
  unsigned long i;
  int dev = 0;

  if (res->num == 0)
		return CMD_DEC_INVALID;	//we need at least one byte

  if (res->opt) {
    if (strncmp(res->opt, "-A", 2) == 0)
      dev = 1;
  }

  /* we allocate maximum for 32bit words, even used shorter later */
  SPIbufTx = (unsigned char *)MEM_PoolAlloc(4 * CMD_DEC_NUM_VAL);
  if (SPIbufTx) {
    SPIbufRx = (unsigned char *)MEM_PoolAlloc(4 *CMD_DEC_NUM_VAL);
    if ( ! SPIbufRx) {
      MEM_PoolFree(SPIbufTx);
      return CMD_DEC_OOMEM;
    }
  }
  else {
    return CMD_DEC_OOMEM;
  }

  /* check word size, endian and generate SPI packet */
  {
    int wordSize;
    int endian;

    if (dev == 1)
	  {
		  wordSize = gCFGparams.SPI1words & 0xF;
		  endian = (gCFGparams.SPI1words >> 4) & 0x1;
	  }
	  else
	  {
		  wordSize = gCFGparams.SPI2words & 0xF;
		  endian = (gCFGparams.SPI2words >> 4) & 0x1;
	  }

	  for (i = 0; i < (res->num * wordSize); i += wordSize)
	  {
		  //set spiTx buffer - here it depends on WordSize and Endian from SYS config
		  switch (wordSize)
		  {
		  case 1: //CFG_SPI_WS_8
				SPIbufTx[i] = (uint8_t)res->val[i];
				break;
		  case 2: //CFG_SPI_WS_16
				if (endian == 1)
				{
					SPIbufTx[i + 0] = (uint8_t)(res->val[i / wordSize] >> 8);
					SPIbufTx[i + 1] = (uint8_t)(res->val[i / wordSize] >> 0);
				}
				else
				{
					SPIbufTx[i + 0] = (uint8_t)(res->val[i / wordSize] >> 0);
					SPIbufTx[i + 1] = (uint8_t)(res->val[i / wordSize] >> 8);
				}
				break;
		  default: //CFG_SPI_WS_32
				if (endian == 1)
				{
					SPIbufTx[i + 0] = (uint8_t)(res->val[i / wordSize] >> 24);
					SPIbufTx[i + 1] = (uint8_t)(res->val[i / wordSize] >> 16);
					SPIbufTx[i + 2] = (uint8_t)(res->val[i / wordSize] >>  8);
					SPIbufTx[i + 3] = (uint8_t)(res->val[i / wordSize] >>  0);
				}
				else
				{
					SPIbufTx[i + 0] = (uint8_t)(res->val[i / wordSize] >>  0);
					SPIbufTx[i + 1] = (uint8_t)(res->val[i / wordSize] >>  8);
					SPIbufTx[i + 2] = (uint8_t)(res->val[i / wordSize] >> 16);
					SPIbufTx[i + 3] = (uint8_t)(res->val[i / wordSize] >> 24);
				}
				break;
		  }
	  }

    /* do SPI transaction */
    SPI_transaction(dev, SPIbufTx, SPIbufRx, res->num * wordSize);

    /* decode results */
    {
      for (i = 0; i < (res->num * wordSize); i += wordSize)
	    {
		    switch (wordSize)
		    {
		      case 1: //CFG_SPI_WS_8 :
				    print_log(out, (const char *)"0x%02x ", SPIbufRx[i]);
				    break;
		      case 2: //CFG_SPI_WS_16 :
				    if (endian == 1)
				    {
					    print_log(out, (const char *)"0x%02x%02x ", SPIbufRx[i + 0], SPIbufRx[i + 1]);
				    }
				    else
				    {
					    print_log(out, (const char *)"0x%02x%02x ", SPIbufRx[i + 1], SPIbufRx[i + 0]);
				    }
				    break;
		      default: //CFG_SPI_WS_32
				    if (endian == 1)
				    {
					    print_log(out, (const char *)"0x%02x%02x%02x%02x ", SPIbufRx[i + 0], SPIbufRx[i + 1], SPIbufRx[i + 2], SPIbufRx[i + 3]);
				    }
				    else
				    {
					    print_log(out, (const char *)"0x%02x%02x%02x%02x ", SPIbufRx[i + 3], SPIbufRx[i + 2], SPIbufRx[i + 1], SPIbufRx[i + 0]);
				    }
				    break;
		    }
	    }
	    UART_Send((const char *)"\r\n", 2, out);
    }
  }

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
    SDCARD_setup(out);
  else
    SDCARD_deinit();

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sddir(TCMD_DEC_Results* res, EResultOut out) {
  SDCARD_printDirectory("/", 0, out);

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

  CFG_Print(out);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_setcfg(TCMD_DEC_Results* res, EResultOut out) {
  (void)out;

  CFG_Set(res->val[0], res->val[1]);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_pstat(TCMD_DEC_Results* res, EResultOut out) {
  int dev = 0;

  if (res->opt) {
    if (strncmp(res->opt, "-A", 2) == 0)
      dev = 1;
  }

  print_log(out, "INT cnt        : %ld\r\n", GPIO_GetINTcounter(dev));
  print_log(out, "INT handled cnt: %ld\r\n", GPIO_GetINTHandledcounter(dev));

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_cstat(TCMD_DEC_Results* res, EResultOut out) {
  int dev = 0;

  if (res->opt) {
    if (strncmp(res->opt, "-A", 2) == 0)
      dev = 1;
  }

  GPIO_ClearCounters(dev);

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
  if (res->opt) {
    if (strncmp(res->opt, "-i", 2) == 0) {
      pico_c_init();
      return CMD_DEC_OK;
    }
    if (strncmp(res->opt, "-d", 2) == 0) {
      if (pico_c_isRunning()) {
        pico_c_deinit();
      }
      return CMD_DEC_OK;
    }
  }

	pico_c_main_interactive(0, NULL);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_picocExec(TCMD_DEC_Results *res, EResultOut out) {
  if (pico_c_isRunning()) {
    int strLen = (int)strlen(res->str);
    res->ctl = 1;             //break entire command line, take as one line
    Parse("PICOC", res->str, strLen, TRUE);
  }

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_sdprint(TCMD_DEC_Results *res, EResultOut out)
{
	SDCARD_PrintFile(res->str, out);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_udptest(TCMD_DEC_Results *res, EResultOut out)
{
	UDP_test();

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_udpip(TCMD_DEC_Results *res, EResultOut out)
{
	(void)out;
	unsigned long ip[4] = {0, 0, 0, 0};
	unsigned long ipAddr = 0;

  if (res->str) {
	  sscanf(res->str, "%lu.%lu.%lu.%lu", &ip[0], &ip[1], &ip[2], &ip[3]);
	  ipAddr  = ip[0];
	  ipAddr |= ip[1] << 8;
	  ipAddr |= ip[2] << 16;
	  ipAddr |= ip[3] << 24;
  }
	UDP_setHostIP(ipAddr);

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_repeat(TCMD_DEC_Results *res, EResultOut out)
{
	int numRepeat = 1;			//default is 1
	int endLess = 0;
	char *keepStr;				  //remember the command(s) to repeat

	if (res->opt)
	{
		if (*res->opt == '-')
		{
			sscanf((res->opt + 1), (const char *)"%i", &numRepeat);
		}
	}

	if (numRepeat == 0)
	{
		endLess = 1;
		numRepeat = 1;
	}

	keepStr = res->str;			//remember the rest of string as commands(s)

	/* loop numRepeat times to repeat the commands(s) */
	do
	{
		/* check UART reception - any received character will break the loop */
		if (UART_getCharNW())
			break;				//break loop with any UART Rx received */

		/* call the command interpreter - one recursive step deeper */
		CMD_DEC_execute(keepStr, out);

		/* if numRepeat is 0 - we do it endless */
		if (endLess)
			continue;
		else
			numRepeat--;
	} while (numRepeat);

	res->ctl = 1;			//break the outer command interpreter, we can have ';', done here

	return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_fwreset(TCMD_DEC_Results *res, EResultOut out) {
  (void)res;
  (void)out;

  ////NVIC_SystemReset();     //CMSIS files do not work!
  SCB_AIRCR = 0x05FA0004;

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_cgpio(TCMD_DEC_Results *res, EResultOut out) {
  (void)out;

  GPIO_config(res->val[0], res->val[1]);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_pgpio(TCMD_DEC_Results *res, EResultOut out) {
  (void)out;

  GPIO_putPins(res->val[0]);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_ggpio(TCMD_DEC_Results *res, EResultOut out) {
  unsigned long val;

  val = GPIOgetPins();
  print_log(out, "GPIOs: %08lx\r\n", val);

  return CMD_DEC_OK;
}

ECMD_DEC_Status CMD_res(TCMD_DEC_Results *res, EResultOut out) {
  (void)out;

  GPIO_resetPin(res->val[0]);

  return CMD_DEC_OK;
}


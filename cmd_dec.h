#ifndef CMD_DEC_H_
#define CMD_DEC_H_

////using namespace std;

#define VERSION_INFO		"V1.00"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "debug_sys.h"
#include "VCP_UART.h"

#define CMD_DEC_NUM_VAL		(480 +1)		/* maximum number of value parameters */
#define LINE_BUFFER_LEN		(CMD_DEC_NUM_VAL * 7 + 16)

#define CMD_DEC_OPT_SIGN	'-'				/* option starts with '-' */
#define CMD_DEC_SEPARATOR	';'				/* different commands in single line */
#define CMD_DEC_COMMENT		'#'				/* comment after command - rest of line ignored */

#define XPRINT_LEN			(80*64)			/* max. length print buffer/strings/lines */

////#define UART_BAUDRATE		6000000			/* 1843200 UART baud rate */

typedef enum {
	CMD_DEC_OK = 0,							/* all OK */
	CMD_DEC_UNKNOWN,						/* command does not exist */
	CMD_DEC_INVALID,						/* wrong command syntax */
	CMD_DEC_ERROR,							/* error on command execution */
	CMD_DEC_EMPTY,							/* empty command, no keyword, e.g. just ENTER */
	CMD_DEC_OOMEM,							/* out of memory to get buffer from mempool */
	CMD_DEC_INVPARAM,						/* invalid parameter, e.g. length too large */
	CMD_DEC_TIMEOUT							/* time out on command */
} ECMD_DEC_Status;

typedef struct {
	char *cmd;								            /* command key word */
	char *opt;								            /* if option starting with '-' - the string afterwards */
	char *str;								            /* rest of line as string */
	unsigned long cmdLen;			            /* character length of CMD keyword */
	unsigned long  offset;		            /* end of command string, or next semicolon */
	unsigned long  num;				            /* number of specified values */
  unsigned long  ctl;				            /* break outer command interpreter loop, 'concur' seen */
  unsigned long val[CMD_DEC_NUM_VAL];		/* index 0 can be an optVal */
} TCMD_DEC_Results;

typedef ECMD_DEC_Status (*TCmdFunc)(TCMD_DEC_Results *res, EResultOut out);

typedef struct {
	const char *cmd;						/* command key word */
	const char *help;						/* help text */
	const TCmdFunc func;				/* the command handler function */
} TCMD_DEC_Command;

void  UART_Send(const char* str, int chrs, EResultOut out);
void  UART_SendStr(const char* str, EResultOut out);
char* UART_GetString(void);

#define UART_GetString    VCP_UART_getString

void MEM_PoolInit(void);
void hex_dump(char* ptr, int len, int mode, EResultOut out);

#ifdef __cplusplus
extern "C" {
#endif
ECMD_DEC_Status CMD_DEC_execute(char *cmd, EResultOut out);
#ifdef __cplusplus
}
#endif

#endif /* CMD_DEC_H_ */

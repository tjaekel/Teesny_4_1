#ifndef SYS_CONFIG_H__
#define SYS_CONFIG_H__

////using namespace std;

/* define system macros, stack size etc. */
#define VERSION_NUMBER  "V1.1.1"
#define VERSION_STRING  "---- Teensy FW: " VERSION_NUMBER " ----"

/* thread definitions */
#define THREAD_STACK_SIZE_CMD         (2*1024)
#define THREAD_STACK_SIZE_GPIO        (2*1024)
#define THREAD_STACK_SIZE_HTTPD       (1*1024)
#define THREAD_STACK_SIZE_TFTP        (1*1024)
#define THREAD_STACK_SIZE_UDP         (1*1024)

#define THREAD_PRIO_CMD               1
#define THREAD_PRIO_GPIO              3
#define THREAD_PRIO_HTTPD             2
#define THREAD_PRIO_TFTP              1
#define THREAD_PRIO_UDP               1

#include "VCP_UART.h"

/* ---------- flash sys_cfg ---------------- */

typedef struct structCFGParams {
  unsigned long key;
  unsigned long SPI1br;
  unsigned long SPI2br;
  unsigned long SPI1mode;           //bit[3:1] = SPIMode (0..3), bit[7:4]=LSB (=0) vs. MSB (=1)
  unsigned long SPI2mode;
  unsigned long IPaddress;
  unsigned long GPIOdir;
  unsigned long GPIOod;             //if output: as Open Drain
  unsigned long GPIOval;            //default values at startup
  unsigned long DebugFlags;
} tCFGParams;

extern tCFGParams gCFGparams;

#define DBG_VERBOSE               (1 <<  0)
#define DBG_SPI                   (1 <<  1)
#define DBG_NETWORK               (1 << 31)

int CFG_Read(void);
int CFG_Write(void);
void CFG_Print(EResultOut out);
void CFG_SetDefault(void);
void CFG_Set(unsigned long idx, unsigned long val);

#endif

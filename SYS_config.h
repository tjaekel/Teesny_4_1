#ifndef SYS_CONFIG_H__
#define SYS_CONFIG_H__

////using namespace std;

/* define system macros, stack size etc. */

#define VERSION_STRING  "---- Teensy FW: V1.1.1 ----"
#define THREAD_STACK_SIZE_CMD         (2*1024)
#define THREAD_STACK_SIZE_GPIO        (2*1024)
#define THREAD_STACK_SIZE_HTTPD       (1*1024)
#define THREAD_STACK_SIZE_TFTP        (1*1024)

/* ---------- flash sys_cfg ---------------- */

typedef struct structCFGParams {
  unsigned long key;
  unsigned long SPI1br;
  unsigned long SPI2br;
  unsigned long SPI1mode;
  unsigned long SPI2mode;
} tCFGParams;

extern tCFGParams gCFGparams;

int CFG_Read(void);
int CFG_Write(void);
void CFG_Print(EResultOut out);
void CFG_SetDefault(void);

#endif

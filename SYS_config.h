#ifndef SYS_CONFIG_H__
#define SYS_CONFIG_H__

////using namespace std;

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

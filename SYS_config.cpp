#include <EEPROM.h>

#include "VCP_UART.h"
#include "SYS_config.h"
#include "TCP_Server.h"

tCFGParams gCFGparams;

static tCFGParams sCFGparamsDefault = {
  .key = 0x11223344,          //key: verify if programmed and reading works
  .SPI1br = 10000000,
  .SPI2br = 12000000,
  .IPaddress = 0,
};

int CFG_Read(void) {
  EEPROM.get(0, gCFGparams);

  //set the TCP server IP address
  TCP_Server_setIPaddress(gCFGparams.IPaddress);

  return 0;
}

int CFG_Write(void) {
  EEPROM.put(0, gCFGparams);

  return 0;
}

void CFG_Print(EResultOut out) {
  size_t i;
  unsigned long *ptr = (unsigned long *)&gCFGparams;
  for (i = 0; i < (sizeof(tCFGParams) / sizeof(unsigned long)); i++) {
    print_log(out, "%2d : %08lx\r\n", i, *ptr++);
  }
}

void CFG_SetDefault(void) {
  memcpy(&gCFGparams, &sCFGparamsDefault, sizeof(tCFGParams));
}

void CFG_Set(unsigned long idx, unsigned long val) {
  if (idx < (sizeof(tCFGParams) / sizeof(unsigned long))) {
    ((unsigned long *)&gCFGparams)[idx] = val;
  }
}
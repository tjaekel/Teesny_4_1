#include <EEPROM.h>

#include "VCP_UART.h"
#include "SYS_config.h"

tCFGParams gCFGparams;

static tCFGParams sCFGparamsDefault = {
  .key = 0x11223344,
  .SPI1br = 10000000,
  .SPI2br = 12000000,
};

int CFG_Read(void) {
  EEPROM.get(0, gCFGparams);

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

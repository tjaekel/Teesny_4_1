#include <EEPROM.h>

#include "VCP_UART.h"
#include "SYS_config.h"
#include "TCP_Server.h"

tCFGParams gCFGparams;

static tCFGParams sCFGparamsDefault = {
  .key = 0x11223344,          //key: verify if programmed and reading works
  .SPI1br = 10000000,
  .SPI2br = 12000000,
  .SPI1mode = 0x0003,         //LSB, Mode 3, MSB is: 0x13
  .SPI2mode = 0x0003,
  .IPaddress = 0,
  .GPIOdir = 0,
  .GPIOod = 0,
  .GPIOval = 0,
  .DebugFlags = 0,
};

static const char *CFG_string(int i) {
  switch(i) {
    case 0: return "key";
    case 1: return "SPI1 bitrate";
    case 2: return "SPI2 bitrate";
    case 3: return "SPI1 Mode";
    case 4: return "SPI2 Mode";
    case 5: return "IP Address";
    case 6: return "GPIO direction";
    case 7: return "GPIO open drain";
    case 8: return "GPIO out default";
    case 9: return "Debug flags";

    default: return "unknown";
  }
}

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
    print_log(out, "[%2d] %-20s : %08lx\r\n", i, CFG_string(i), *ptr++);
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

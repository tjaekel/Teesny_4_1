
#include "VCP_UART.h"
#include "SYS_config.h"
#include "SYS_error.h"

unsigned long SYS_Error = 0;

/* SYS_Error decoded values */
static void SYSERR_printDecoded(EResultOut out, unsigned long err) {
  const char *s;
  unsigned long sMask = 0x1;
  int i;

  for (i = 0; i < 32; i++) {
    s = NULL;
    if ((err & sMask) == SYSERR_CMD)
      s = "CMD error";
    if ((err & sMask) == SYSERR_SPI)
      s = "SPI error";
    if ((err & sMask) == SYSERR_ETH)
      s = "ETH link";
    if ((err & sMask) == SYSERR_FATAL)
      s = "TATAL error";
    if (s)
      print_log(out, "*E: %-20s\r\n", s);

    sMask <<= 1;
  }
}

void SYSERR_Set(EResultOut out, unsigned long mask) {
  /* optional: debug verbose shows immediately the error set on UART */
  if (gCFGparams.DebugFlags & DBG_VERBOSE)
    print_log(out, "\r\n*E: SYS Error: %lx\r\n", mask);

  SYS_Error |= mask;
}

unsigned long SYSERR_Get(EResultOut out, int printFlag) {
  /* clear all and return before cleared, printFlag decodes all */
  unsigned long err;
  err = SYS_Error;

  if (printFlag)
    SYSERR_printDecoded(out, err);

  SYS_Error = 0;

  return err;
}
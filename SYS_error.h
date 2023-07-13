#ifndef __SYS_ERROR_H__
#define __SYS_ERROR_H__

/* SYS_error values (bit encoded) */
#define SYSERR_CMD          (unsigned long)(1 <<  0)
#define SYSERR_SPI          (unsigned long)(1 <<  1)
#define SYSERR_MEM          (unsigned long)(1 <<  2)      /* Out of Memory */
#define SYSERR_TFTP         (unsigned long)(1 <<  3)
#define SYSERR_TIR          (unsigned long)(1 <<  4)
#define SYSERR_FILE         (unsigned long)(1 <<  5)

#define SYSERR_UDP          (unsigned long)(1 << 29)     /* destination IP not set */
#define SYSERR_ETH          (unsigned long)(1 << 30)     /* ETH link */
#define SYSERR_FATAL        (unsigned long)(1 << 31)     /* any other, fatal error */

extern unsigned long SYS_Error;

void SYSERR_Set(EResultOut out, unsigned long mask);
unsigned long SYSERR_Get(EResultOut out, int printFlag);

#endif

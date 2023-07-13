#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#include "cmd_dec.h"

void SDCARD_setup(EResultOut out);
void SDCARD_deinit(void);
void SDCARD_printDirectory(const char *str, int numSpaces, EResultOut out);
int  SDCARD_PrintFile(char *file, EResultOut out);
int  SDCARD_GetStatus(void);
int  SDCARD_fgets(char *p, int len, void *file);
void SDCARD_ShowDir(EResultOut out);

ECMD_DEC_Status SDCARD_Exec(TCMD_DEC_Results *res, EResultOut out);

/* used by Pico-C, as C-code, not CPP */
#ifdef __cplusplus
extern "C" {
#endif
int  SDCARD_ReadFile(const char *file, unsigned char *b);
#ifdef __cplusplus
}
#endif

#endif

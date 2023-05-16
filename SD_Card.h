#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#include <SD.h>

void SDCARD_setup(void);
void SDCARD_deinit(void);
void SDCARD_printDirectory(const char *str, int numSpaces);

#endif

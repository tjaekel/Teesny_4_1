#ifndef __SD_CARD_H__
#define __SD_CARD_H__

void SDCARD_setup(void);
void SDCARD_deinit(void);
void SDCARD_printDirectory(const char *str, int numSpaces);
int  SDCARD_PrintFile(char *file);

/* used by Pico-C, as C-code, not CPP */
#ifdef __cplusplus
extern "C" {
#endif
int  SDCARD_ReadFile(const char *file, unsigned char *b);
#ifdef __cplusplus
}
#endif

#endif

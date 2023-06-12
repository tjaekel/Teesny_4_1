#ifndef __GPIO_H__
#define __GPIO_H__

unsigned long GPIO_GetINTcounter(int num);
unsigned long GPIO_GetINTHandledcounter(int num);

void GPIO_setup(void);

#endif

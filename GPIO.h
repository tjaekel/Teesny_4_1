#ifndef __GPIO_H__
#define __GPIO_H__

typedef struct structGPIOconfig {
  int pin;
  const char *desc;
} tGPIOcfg;

unsigned long GPIO_GetINTcounter(int num);
unsigned long GPIO_GetINTHandledcounter(int num);

void GPIO_setup(void);

void GPIO_putPins(unsigned long vals);
unsigned long GPIOgetPins(void);

#endif

#ifndef __GPIO_H__
#define __GPIO_H__

#include "VCP_UART.h"

/* define the pinMode: */
#define GPIO_RD   (1 << 0)
#define GPIO_WR   (1 << 1)
#define GPIO_INT  (1 << 2)        //it is not touched (not configured here)

typedef struct structGPIOconfig {
  int pin;
  int pinMode;
  const char *desc;
} tGPIOcfg;

unsigned long GPIO_GetINTcounter(int num);
unsigned long GPIO_GetINTHandledcounter(int num);
void GPIO_ClearCounters(int num);

void GPIO_setup(void);

void GPIO_putPins(unsigned long vals);
unsigned long GPIOgetPins(void);
void GPIOgetPinsDisplay(EResultOut out);
void GPIO_config(unsigned long dir, unsigned long od);
void GPIO_resetPin(unsigned long val);

void GPIO_setOutValue(uint8_t pin, uint8_t val);
void GPIO_testSpeed(void);

unsigned long GPIO_GetINTFreq(int num);

#endif

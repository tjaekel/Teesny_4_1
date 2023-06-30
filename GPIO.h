#ifndef __GPIO_H__
#define __GPIO_H__

typedef struct structGPIOconfig {
  int pin;
  const char *desc;
} tGPIOcfg;

unsigned long GPIO_GetINTcounter(int num);
unsigned long GPIO_GetINTHandledcounter(int num);
void GPIO_ClearCounters(int num);

void GPIO_setup(void);

void GPIO_putPins(unsigned long vals);
unsigned long GPIOgetPins(void);
void GPIO_config(unsigned long dir, unsigned long od);
void GPIO_resetPin(unsigned long val);

void GPIO_setOutValue(uint8_t pin, uint8_t val);

#endif

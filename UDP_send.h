#ifndef __UDP_SEND_H__
#define __UDP_SEND_H__

#include <IPAddress.h>

void UDP_setup(void);
bool UDP_send(int port, uint8_t *pout, size_t n);
void UDP_setHostIP(unsigned long ipAddr);

void UDP_test(void);

#endif

#ifndef __UDP_SEND_H__
#define __UDP_SEND_H__

void UDP_setup(void);

void UDP_setHostIP(unsigned long ipAddr);

void UDP_test(void);

//needed by Pico-C : as C-code
#ifdef __cplusplus
extern "C" {
#endif
int  UDP_send(int port, uint8_t *pout, size_t n);
#ifdef __cplusplus
}
#endif

#endif

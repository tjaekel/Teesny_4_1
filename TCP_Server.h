#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

void TCP_Server_setIPaddress(int32_t ip);
void TCP_Server_setup(void);
unsigned int HTTPD_GetClientNumber(void);
int HTTPD_GetETHLinkState(void);

#endif

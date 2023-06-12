
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include <QNEthernet.h>
#include <QNEthernetUDP.h>

#include "VCP_UART.h"
#include "UDP_send.h"

using namespace qindesign::network;

EthernetUDP UdpSend;
IPAddress udpHost;

uint16_t UDPcnt = 0;

void UDP_setup(void) {
  /* nothing to do QNEthernet, TFTP, already up and running */
}

int UDP_send(int port, uint8_t *pout, size_t n) {
  int r = 0;
  uint32_t ip = udpHost;

  if (ip) {
    UdpSend.beginPacket(udpHost, port);
    UdpSend.write((const char *)&UDPcnt, sizeof(UDPcnt));
    UDPcnt++;     //increment the counter
    r = (int)UdpSend.write(pout, n);
    UdpSend.endPacket();

    return r;
  }

  return r;
}

void UDP_setHostIP(unsigned long ipAddr) {
  //ipAddr is BIG ENDIAN!
  udpHost = (uint8_t *)&ipAddr;
}

//data for UDP_test:
static uint8_t d[10] = {1,2,3,4,5,6,7,8,9,10};

void UDP_test(void) {
  bool r;
  r = UDP_send(8080, d, sizeof(d));
  if (r)
    print_log(UART_OUT, "OK\r\n");
  else
    print_log(UART_OUT, "FAIL\r\n");
}

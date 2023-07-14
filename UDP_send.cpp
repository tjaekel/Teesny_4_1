
#include "arduino_freertos.h"
#include <semphr.h>
#include "avr/pgmspace.h"
#include <climits>

#include <QNEthernet.h>
#include <QNEthernetUDP.h>

#include "SYS_config.h"
#include "VCP_UART.h"
#include "SYS_error.h"
#include "UDP_send.h"

using namespace qindesign::network;

EthernetUDP UdpSend;
IPAddress udpHost;

uint16_t UDPcnt = 0;

//thread parameters
static int Tport;
static uint8_t *Tpout;
static size_t Tn;
static SemaphoreHandle_t xSemaphore;

void UDP_thread(void *pvParameters) {
  while (1) {
    xSemaphoreTake( xSemaphore, portMAX_DELAY );

    UdpSend.beginPacket(udpHost, Tport);
    taskYIELD();
    UdpSend.write((const char *)&UDPcnt, sizeof(UDPcnt));
    taskYIELD();
    UDPcnt++;     //increment the counter
    UdpSend.write(Tpout, Tn);
    taskYIELD();
    UdpSend.endPacket();
  }
}

FLASHMEM void UDP_setup(void) {
  /* nothing to do QNEthernet, TFTP, already up and running */
  /* just create a thread and we do UDP send in background */
  xSemaphore = xSemaphoreCreateBinary();
  ::xTaskCreate(UDP_thread, "UDP_thread", THREAD_STACK_SIZE_UDP, nullptr, THREAD_PRIO_UDP, nullptr);
}

int UDP_send(int port, uint8_t *pout, size_t n) {
  int r = 0;
  uint32_t ip = udpHost;

  if (ip) {
    Tport = port;
    Tpout = pout;
    Tn = n;

    //release thread
    xSemaphoreGive( xSemaphore );
    r = n;        //ATT: asynchronous - we return from here immediately
    return r;
  }

  //set syserr
  SYSERR_Set(UART_OUT, SYSERR_UDP);
  return r;
}

FLASHMEM void UDP_setHostIP(unsigned long ipAddr) {
  //ipAddr is BIG ENDIAN!
  ////udpHost = (uint8_t *)&ipAddr;
  udpHost = (IPAddress)ipAddr;
}

//data for UDP_test:
static uint8_t d[10] = {1,2,3,4,5,6,7,8,9,10};

FLASHMEM void UDP_test(void) {
  bool r;
  r = UDP_send(8080, d, sizeof(d));
  if (r)
    print_log(UART_OUT, "OK\r\n");
  else
    print_log(UART_OUT, "FAIL\r\n");
}

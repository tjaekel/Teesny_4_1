
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include <SD.h>
#include <QNEthernet.h>
#include <QNEthernetUDP.h>
#include "VCP_UART.h"
#include "tftp_server.h"

#include "SYS_config.h"

using namespace qindesign::network;

File file;

// file interface
static uint32_t nbytes, us;

void* tftp_fs_open(const char *fname, const char *mode, uint8_t write)
{
  nbytes = 0;
  ////Serial.printf("*I: opening %s %d %d\r\n", fname, write, O_READ);
  us = micros();
  if (write == 0) {
    ////Serial.println("*I: opening for read");
    file = SD.open(fname);
  }
  else
    file = SD.open(fname, /*FILE_WRITE*/ FILE_WRITE_BEGIN);
  if (file) {
    ////Serial.println("*I: opened for write");
    file.truncate();            //like: O_TRUNC
  }
  else return NULL;

  return (void *)fname;
}

void tftp_fs_close(void *handle)
{
  us = micros() - us;
  file.close();
  ////Serial.printf("*I: closed %d bytes %d us\r\n", nbytes, us);
}

int tftp_fs_read(void *handle, void *buf, int bytes)
{
  int ret = 0;

  //Serial.printf("read avail %d\n", file.available());
  if (file.available()) {
    ret = file.read((uint8_t*)buf, bytes);
    nbytes += ret;
    // Serial.printf("*I: read  %d %d\n", bytes, ret);
  }
  return ret;
}

int tftp_fs_write(void *handle, void *buf, int bytes)
{
  int ret;

  nbytes += bytes;
  ret = file.write((char *)buf, bytes);
  return ret;
}

#if 0
static void teensyMAC(uint8_t *mac)
{
  uint32_t m1 = HW_OCOTP_MAC1;
  uint32_t m2 = HW_OCOTP_MAC0;
  mac[0] = m1 >> 8;
  mac[1] = m1 >> 0;
  mac[2] = m2 >> 24;
  mac[3] = m2 >> 16;
  mac[4] = m2 >> 8;
  mac[5] = m2 >> 0;
}
#endif

extern void tftp_thread(void *pvParameters);
/*const*/ tftp_context tftp_ctx = { tftp_fs_open, tftp_fs_close, tftp_fs_read, tftp_fs_write };

FLASHMEM void TFTP_setup(EResultOut out) {
#if 0
  if (!SD.begin(BUILTIN_SDCARD)) {
    UART_printString("*E: SD card failed\r\n", out);
    return;
  }
#endif

#if 0
  //if not yet done by HTTP_server
  uint8_t mac[6];
  teensyMAC(mac);
  //not on QNEthernet, just begin()
  Ethernet.begin(mac);
#endif
  
  {
    uint32_t rawAddr;
    rawAddr = Ethernet.localIP();
    if (gCFGparams.DebugFlags & DBG_NETWORK)
      print_log(out, "\r\n*I: IP address: %ld.%ld.%ld.%ld\r\n", (rawAddr >> 0) & 0xFF, (rawAddr >> 8) & 0xFF, (rawAddr >> 16) & 0xFF, (rawAddr >> 24) & 0xFF);
  }
  
  ::xTaskCreate(tftp_thread, "tftp_thread", THREAD_STACK_SIZE_TFTP, nullptr, THREAD_PRIO_TFTP, nullptr);
}

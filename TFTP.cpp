
#include <SD.h>
#include <QNEthernet.h>
#include <QNEthernetUDP.h>
#include <TeensyThreads.h>
#include "tftp_server.h"

using namespace qindesign::network;

int TFTPid  = 0;
File file;

// file interface
static uint32_t nbytes, us;

void* tftp_fs_open(const char *fname, const char *mode, uint8_t write)
{
  ////char  *f = (char *)"xx";      /WHO is doing such crab?

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

  //return f;
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

extern void tftp_thread(void);
/*const*/ tftp_context tftp_ctx = { tftp_fs_open, tftp_fs_close, tftp_fs_read, tftp_fs_write };

void TFTP_setup() {
#if 0
  Serial.begin(9600);
  while (!Serial);
  delay(100);
#endif
  ////Serial.print("*I: Initializing SD card...");

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("*E: SD card failed");
    return;
  }
  ////Serial.println("initialization done");

#if 0
  //if not yet done by HTTP_server
  uint8_t mac[6];
  teensyMAC(mac);
  //not on QNEthernet, just begin()
  Ethernet.begin(mac);
#endif
  Serial.print("*I: IP address: ");
  Serial.println(Ethernet.localIP());

  ////tftp_init(&tftp_ctx);     //not as thread
  TFTPid = threads.addThread(tftp_thread, 0, 2048);
}

void TFTP_kill(void) {
  if (TFTPid) {
    threads.kill(TFTPid);

    //close UDP socket? - it should stop on next Udp.begin()
    tftp_kill();
    TFTPid = 0;
  }

}
// tftp server
// 4 byte header, 512 byte data,  network byte order

#include <QNEthernet.h>
#include <QNEthernetUDP.h>

#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include "tftp_server.h"

using namespace qindesign::network;

#define swap2 __builtin_bswap16

#define TFTP_PORT 69

#define TFTP_MAX_PAYLOAD_SIZE 512
#define TFTP_HEADER_LENGTH    4

#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5

enum tftp_error {
  TFTP_ERROR_FILE_NOT_FOUND    = 1,
  TFTP_ERROR_ACCESS_VIOLATION  = 2,
  TFTP_ERROR_DISK_FULL         = 3,
  TFTP_ERROR_ILLEGAL_OPERATION = 4,
  TFTP_ERROR_UNKNOWN_TRFR_ID   = 5,
  TFTP_ERROR_FILE_EXISTS       = 6,
  TFTP_ERROR_NO_SUCH_USER      = 7
};

#define TFTP_TIMEOUT_MSECS    5000
#define TFTP_MAX_RETRIES      5

#include <string.h>

struct tftp_state {
  const struct tftp_context *ctx;
  void *handle;
  IPAddress addr;
  uint16_t port;
  int timer;
  int last_pkt;
  uint16_t blknum;
  uint8_t retries;
  uint8_t mode_write;
};

static struct tftp_state tftp_state;

// tftp packet  4-byte header 512-byte payload
uint16_t pktin[256], pktout[258];
uint8_t *pin = (uint8_t *) pktin;
uint8_t *pout = (uint8_t *) pktout;
uint8_t *pbufin = (uint8_t *) &pktin[2];
uint8_t *pbufout = (uint8_t *) &pktout[2];
int send_lth;

EthernetUDP Udp;

void close_connection() {
  if (tftp_state.handle) {
    tftp_state.ctx->close(tftp_state.handle);
    tftp_state.handle = NULL;
  }
}

void send_error(IPAddress host, int port, int code, char *msg) {
  int n = strlen(msg) + 1 + TFTP_HEADER_LENGTH;
  pktout[0] = swap2(TFTP_ERROR);
  pktout[1] = swap2(code);
  strcpy((char *)pbufout, msg);
  Udp.beginPacket(host, port);
  Udp.write(pout, n);
  Udp.endPacket();
  Serial.printf("error %d %s\n", code, msg);
}


void resend() {
  Udp.beginPacket(tftp_state.addr, tftp_state.port);
  Udp.write(pout, send_lth);
  Udp.endPacket();
}


void send_ack(int blknum) {
  pktout[0] = swap2(TFTP_ACK);
  pktout[1] = swap2(blknum);
  send_lth = TFTP_HEADER_LENGTH;
  Udp.beginPacket(tftp_state.addr, tftp_state.port);
  Udp.write(pout, send_lth);
  Udp.endPacket();
  tftp_state.last_pkt = millis();

}

void send_data() {
  int ret;

  pktout[0] = swap2(TFTP_DATA);
  pktout[1] = swap2(tftp_state.blknum);
  ret = tftp_state.ctx->read(tftp_state.handle, pbufout, TFTP_MAX_PAYLOAD_SIZE);
  if (ret < 0) {
    send_error(tftp_state.addr, tftp_state.port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: Error occured while reading file");
    close_connection();
    return;
  }
  send_lth = ret + TFTP_HEADER_LENGTH;
  // Serial.printf("send blk %d send_lth %d\n", tftp_state.blknum, send_lth);
  tftp_state.last_pkt = millis();

  resend();
}

void recv() {
  int rlth, port;
  uint16_t op, blk;
  IPAddress remote;
  char *mode = (char *)"binary";  // not used

  rlth = Udp.read(pin, sizeof(pktin));
  //needed for QNEthernet
  if ( ! rlth) {
    ////threads.delay(10);
    vTaskDelay(10);
    return;
  }

  remote = Udp.remoteIP();
  port = Udp.remotePort();
  op = swap2(pktin[0]);
  blk = swap2(pktin[1]);

  //  Serial.printf("rlth %d op %d blk %d\n", rlth, op, blk);
  switch (op) {
    case TFTP_RRQ:
    case TFTP_WRQ:
      {
        if (tftp_state.handle != NULL) {
          send_error(remote, port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: Only one connection is supported");
          break;
        }
        char *filename = (char *)&pktin[1];
        //        Serial.println(filename);
        tftp_state.handle = tftp_state.ctx->open(filename, mode, op == TFTP_WRQ);
        tftp_state.blknum = 1;

        if (!tftp_state.handle) {
          send_error(remote, port, TFTP_ERROR_FILE_NOT_FOUND, (char *)"*E: Unable to open file");
          break;
        }

        send_lth = 0;
        tftp_state.addr = remote;
        tftp_state.port = port;
        tftp_state.last_pkt = millis();
        tftp_state.retries = 0;

        if (op == TFTP_WRQ) {
          tftp_state.mode_write = 1;
          send_ack(0);
        } else {
          tftp_state.mode_write = 0;
          send_data();
        }
        break;
      }

    case TFTP_ACK:
      {
        if (tftp_state.handle == NULL) {
          send_error(remote, port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: No connection");
          break;
        }

        if (tftp_state.mode_write != 0) {
          send_error(remote, port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: Not a read connection");
          break;
        }

        if (blk != tftp_state.blknum) {
          send_error(remote, port, TFTP_ERROR_UNKNOWN_TRFR_ID, (char *)"*E: wrong block number");
          break;
        }

        int lastpkt = 0;

        if (send_lth != 0) {
          lastpkt = send_lth != (TFTP_MAX_PAYLOAD_SIZE + TFTP_HEADER_LENGTH);
        }
        if (!lastpkt) {
          tftp_state.blknum++;
          send_data();
        } else {
          close_connection();
        }
        break;
      }

    case TFTP_DATA:
      {
        if (tftp_state.handle == NULL) {
          send_error(remote, port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: No connection");
          break;
        }

        if (tftp_state.mode_write != 1) {
          send_error(remote, port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: Not a write connection");
          break;
        }

        int ret = tftp_state.ctx->write(tftp_state.handle, pbufin, rlth - TFTP_HEADER_LENGTH);
        if (ret < 0) {
          send_error(remote, port, TFTP_ERROR_ACCESS_VIOLATION, (char *)"*E: Error writing file");
          close_connection();
        } else {
          send_ack(blk);
        }

        if ((rlth - TFTP_HEADER_LENGTH) < TFTP_MAX_PAYLOAD_SIZE) {
          close_connection();
        }
        break;
      }

    default:
      send_error(remote, port, TFTP_ERROR_ILLEGAL_OPERATION, (char *)"*E: Unknown operation");
      Serial.print(op);
      break;
  }
}

#if 0
//if not as thread
void tftp_init(const struct tftp_context *ctx)
{
  tftp_state.handle    = NULL;
  tftp_state.port      = 0;
  tftp_state.ctx       = ctx;
  tftp_state.timer     = 0;

  Udp.begin(TFTP_PORT);

  while (1) {
    if (Udp.parsePacket()) recv();
    uint32_t ms = millis();
    if (tftp_state.handle != NULL) {
      // timeout and retry test
      if (ms - tftp_state.last_pkt > TFTP_TIMEOUT_MSECS) {
        tftp_state.retries++;
        tftp_state.last_pkt = ms;
        resend();
        if (tftp_state.retries > TFTP_MAX_RETRIES) {
          tftp_state.ctx->close(tftp_state.handle);
          tftp_state.handle = NULL;
        }
      }
    }
  }
}
#endif

extern /*const*/ tftp_context tftp_ctx;

void tftp_thread(void *pvParameters) {
  tftp_state.handle    = NULL;
  tftp_state.port      = 0;
  tftp_state.ctx       = &tftp_ctx;
  tftp_state.timer     = 0;

  Udp.begin(TFTP_PORT);

  while (1) {
    if (Udp.parsePacket())
      recv();
    else
      vTaskDelay(10);

    uint32_t ms = millis();
    if (tftp_state.handle != NULL) {
      // timeout and retry test
      if (ms - tftp_state.last_pkt > TFTP_TIMEOUT_MSECS) {
        tftp_state.retries++;
        tftp_state.last_pkt = ms;
        resend();
        if (tftp_state.retries > TFTP_MAX_RETRIES) {
          tftp_state.ctx->close(tftp_state.handle);
          tftp_state.handle = NULL;
        }
      }
    }
    taskYIELD();
  }
}

void tftp_kill(void) {
  Udp.stop();
}

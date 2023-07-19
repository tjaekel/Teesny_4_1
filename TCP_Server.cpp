
#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

#include <QNEthernet.h>

#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include <climits>

#include "define_sys.h"
#include "cmd_dec.h"
#include "VCP_UART.h"
#include "HTTP_data.cpp"
#include "SYS_config.h"
#include "SYS_error.h"

using namespace qindesign::network;

// --------------------------------------------------------------------------
//  Configuration
// --------------------------------------------------------------------------

// The DHCP timeout, in milliseconds. Set to zero to not wait and
// instead rely on the listener to inform us of an address assignment.
constexpr uint32_t kDHCPTimeout = 5000;  // 5 seconds

// The link timeout, in milliseconds. Set to zero to not wait and
// instead rely on the listener to inform us of a link.
constexpr uint32_t kLinkTimeout = 5000;  // 5 seconds

constexpr uint16_t kServerPort = 8080;    //our HTTP port is 8080

// Timeout for waiting for input from the client.
constexpr uint32_t kClientTimeout = 5000;  // 5 seconds

// Timeout for waiting for a close from the client after a
// half close.
constexpr uint32_t kShutdownTimeout = 30000;  // 30 seconds

// Set the static IP to something other than INADDR_NONE (zero)
// to not use DHCP. The values here are just examples.
////IPAddress staticIP{0, 0, 0, 0};
IPAddress staticIP{192, 168, 0, 84};        //will be overwritten by syscfg!
IPAddress subnetMask{255, 255, 255, 0};
IPAddress gateway{192, 168, 0, 1};

static int ETHLinkStatus = 0;

// --------------------------------------------------------------------------
//  Types
// --------------------------------------------------------------------------

/* used by SYSCFG - set before we start the server */
void TCP_Server_setIPaddress(int32_t ip) {
  staticIP = ip;
}

// Keeps track of state for a single client.
struct ClientState {
  ClientState(EthernetClient client)
      : client(std::move(client)) {}

  EthernetClient client;
  bool closed = false;

  // For timeouts.
  uint32_t lastRead = millis();  // Mark creation time

  // For half closed connections, after "Connection: close" was sent
  // and closeOutput() was called
  uint32_t closedTime = 0;    // When the output was shut down
  bool outputClosed = false;  // Whether the output was shut down

  // Parsing state
  bool emptyLine = false;
};

// --------------------------------------------------------------------------
//  Program State
// --------------------------------------------------------------------------

// Keeps track of what and where belong to whom.
std::vector<ClientState> clients;

// The server.
EthernetServer TCPserver{kServerPort};

// --------------------------------------------------------------------------
//  Thead Implementation
// --------------------------------------------------------------------------

static void TCP_Server_thread(void *pvParameters);

// Forward declarations
void tellServer(bool hasIP, bool linkState);

FLASHMEM void TCP_Server_setup(void) {
  if (gCFGparams.DebugFlags & DBG_NETWORK)
    print_log(UART_OUT, "\r\nNetwork starting...\r\n");

  // Unlike the Arduino API (which you can still use), QNEthernet uses
  // the Teensy's internal MAC address by default, so we can retrieve
  // it here
  uint8_t mac[6];
  Ethernet.macAddress(mac);  // This is informative; it retrieves, not sets
  ////print_log(UART_OUT, "MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
  ////       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Add listeners
  // It's important to add these before doing anything with Ethernet
  // so no events are missed.

  // Listen for link changes
  Ethernet.onLinkState([](bool state) {
    if (gCFGparams.DebugFlags & DBG_NETWORK)
      print_log(UART_OUT, "[Ethernet] Link %s\r\n", state ? "ON" : "OFF");
      ETHLinkStatus = (int)state;

      if ( ! state)
        SYSERR_Set(UART_OUT, SYSERR_ETH);
  });

  // Listen for address changes
  Ethernet.onAddressChanged([]() {
    IPAddress ip = Ethernet.localIP();
    bool hasIP = (ip != INADDR_NONE);
    if (hasIP) {
      if (gCFGparams.DebugFlags & DBG_NETWORK) {
        print_log(UART_OUT, "[Ethernet] Address changed:\r\n");
        print_log(UART_OUT, "    Local IP = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
        ip = Ethernet.subnetMask();
        print_log(UART_OUT, "    Subnet   = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
        ip = Ethernet.gatewayIP();
        print_log(UART_OUT, "    Gateway  = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
        ip = Ethernet.dnsServerIP();
        if (ip != INADDR_NONE) {  // May happen with static IP
          print_log(UART_OUT, "    DNS      = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
        }
      }
    } else {
      if (gCFGparams.DebugFlags & DBG_NETWORK) {
        print_log(UART_OUT, "[Ethernet] Address changed: No IP address\r\n");
        SYSERR_Set(UART_OUT, SYSERR_ETH);
      }
    }

    // Tell interested parties the state of the IP address and link,
    // for example, servers, SNTP clients, and other sub-programs that
    // need to know whether to stop/start/restart/etc
    // Note: When setting a static IP, the address will be set but a
    //       link or active network interface might not yet exist
    tellServer(hasIP, Ethernet.linkState());
  });

  if (staticIP == INADDR_NONE) {
    if (gCFGparams.DebugFlags & DBG_NETWORK)
      print_log(UART_OUT, "Starting Ethernet with DHCP...\r\n");
    if (!Ethernet.begin()) {
      if (gCFGparams.DebugFlags & DBG_NETWORK) {
        print_log(UART_OUT, "Failed to start Ethernet\r\n");
        SYSERR_Set(UART_OUT, SYSERR_ETH);
      }
      return;
    }

    // We can choose not to wait and rely on the listener to tell us
    // when an address has been assigned
    if (kDHCPTimeout > 0) {
      if (!Ethernet.waitForLocalIP(kDHCPTimeout)) {
        if (gCFGparams.DebugFlags & DBG_NETWORK) {
          print_log(UART_OUT, "Failed to get IP address from DHCP\r\n");
          // We may still get an address later, after the timeout,
          // so continue instead of returning
        }
        SYSERR_Set(UART_OUT, SYSERR_ETH);
      }
    }
  } else {
    if (gCFGparams.DebugFlags & DBG_NETWORK)
      print_log(UART_OUT, "Starting Ethernet with static IP...\r\n");
    
    Ethernet.begin(staticIP, subnetMask, gateway);

    // When setting a static IP, the address is changed immediately,
    // but the link may not be up; optionally wait for the link here
    if (kLinkTimeout > 0) {
      if (!Ethernet.waitForLink(kLinkTimeout)) {
        if (gCFGparams.DebugFlags & DBG_NETWORK) {
          print_log(UART_OUT, "Failed to get link\r\n");
        }
        SYSERR_Set(UART_OUT, SYSERR_ETH);
        // We may still see a link later, after the timeout, so
        // continue instead of returning
      }
      ////else
      ////  print_log(UART_OUT, "OK, got link\r\n");
    }
  }

  ::xTaskCreate(TCP_Server_thread, "TCP_Server_thread", THREAD_STACK_SIZE_HTTPD, nullptr, THREAD_PRIO_HTTPD, nullptr);
}

// Tell the server there's been an IP address or link state change.
void tellServer(bool hasIP, bool linkState) {
  // If there's no IP address or link, could optionally stop the
  // server, depending on your needs
  if (hasIP && linkState) {
    if (TCPserver) {
      // Optional
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "Address changed: Server already started\r\n");
    } else {
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "Starting server on port %u...", kServerPort);
      TCPserver.begin();
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "%s\r\n", TCPserver ? "done." : "FAILED!");
    }
  } else {
    if (!TCPserver) {
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "Address changed: restart server\r\n");
      TCPserver.begin();
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "%s\r\n", TCPserver ? "done." : "FAILED!");
    } else {
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "Stopping server...");
      SYSERR_Set(UART_OUT, SYSERR_ETH);
      TCPserver.end();
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "done\r\n");
    }
  }
}

//HTTP request buffer
static uint8_t rxBuf[HTTPD_BUF_REQ_SIZE] DMAMEM;

void processClientBinary(ClientState &state, uint8_t *buf, int avail) {
  int len;
  len  = *(buf + 1);
  len |= *(buf + 2) << 8;
  len |= *(buf + 3) << 16;       //get the length, LITTLE ENDIAN

  if (gCFGparams.DebugFlags & DBG_NETWORK)
    print_log(UART_OUT, "*D: TCP bin length: %d | cmd: %02x\r\n", len, *buf);

  switch (*buf) {
    case 0x01 :     //binary command
          //put a NUL into buffer:
          *(buf + 4 + len) = '\0';

          CMD_DEC_execute((char *)(buf + 4), HTTPD_OUT);
          {
            int l;
            char *b;
            ////HTTP_PutEOT();                    //place EOT into buffer - we do not need here
            l = HTTP_GetOut(&b);

            /* hijack the buffer to update command and length */
            *(buf + 1) = (uint8_t)l;
            *(buf + 2) = (uint8_t)(l >> 8);
            *(buf + 2) = (uint8_t)(l >> 16);

            strncpy((char *)(buf + 4), b, l);
            state.client.writeFully(buf, l + 4);
            HTTP_ClearOut();
          }
          break;
    default:
          break;
  }
}

void convertURL(char *s) {
  /* convert + to space, convert %20 to a single space
   * it modifies the input string
   * '+' is used on "GET /?CMD="
   * %20 is used when entered as URL
   */
  while (*s) {
    if (*s == '+')
      *s = ' ';
    s++;

    //for %20 we need two pointers and move the tail forward, or create a buffer for output
  }
}

void processClientData(ClientState &state) {
  // Loop over available data until an empty line or no more data
  // Note that if emptyLine starts as false then this will ignore any
  // initial blank line.
  int dontClose = 0;
  int avail = state.client.available();
  if (avail <= 0) {
    return;
  }

  state.lastRead = millis();
  state.client.read(rxBuf, sizeof(rxBuf) - 1);        //actually avail is number of bytes

  /* check if we have a binary command */
  if (*rxBuf < ' ') {
    processClientBinary(state, rxBuf, avail);
    return;
  }

  //append a NUL for printf
  *(rxBuf + avail) = '\0';
  if (gCFGparams.DebugFlags & DBG_NETWORK)
    ////print_log(UART_OUT, "|%s|\r\n", rxBuf);
    hex_dump(rxBuf, 16, 1, UART_OUT);

  //DEBUG:
  //IPAddress ip = state.client.remoteIP();
  //print_log(UART_OUT, "Sending to client: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);

  {
    char *s, *s2;
    int notHTTP = 0;

    //place NUL after command string (strip of "HTML")
    s = strstr((char *)rxBuf, " HTTP");
    if (s)
      *s = '\000';
    
    //check if we are called with "GET /?CMD=" and take the string afterwards
    s = strstr((char *)rxBuf, "GET /?CMD=");
    if (s)
    {
      s += 10;
      convertURL(s);
      CMD_DEC_execute(s, HTTPD_OUT);
    }
    else {
      s2 = NULL;
      //we might have "GET /anyCommand", "GET /Ccmd" or "GET /cLENcmd"
      s = strstr((char *)rxBuf, "GET /");
      if (s) {
        s2 = strstr((char *)rxBuf, "GET /C");
        if (s2) {
          /* old ASCII command */
          s2 += 6;
          convertURL(s2);
          CMD_DEC_execute(s2, HTTPD_OUT);
          HTTP_PutEOT();
          notHTTP = 1;
        }
        if ( ! s2) {
          s2 = strstr((char *)rxBuf, "GET /c");
          if (s2) {
            int len;
            /* binary length, BIG ENDIAN, two bytes */
            s2 += 6;
            len  = (int)*s2++ << 8;
            len |= (int)*s2++ << 0;
            if (gCFGparams.DebugFlags & DBG_NETWORK)
              print_log(UART_OUT, "*D: TCP bin length: %d\r\n", len);

            convertURL(s2);
            CMD_DEC_execute(s2, HTTPD_OUT);
            ////HTTP_PutEOT();
            dontClose = 1;
            notHTTP = 1;
          }
        }

        if ( !s2) {
          /* for web browser: URL/cmd */
          s += 5;
          convertURL(s);
          CMD_DEC_execute(s, HTTPD_OUT);
        }
      }
      else {
        //without "GET /"
        CMD_DEC_execute((char *)rxBuf, HTTPD_OUT);
        HTTP_PutEOT();
        notHTTP = 1;
      }
    }

    {
      int l;
      char *b;
      l = HTTP_GetOut(&b);
      if ( ! notHTTP)
        state.client.writeFully(data_html_a);
      if (l) {
        state.client.writeFully(b);
      }
      else {
        /* in case nothing after command, send EOT */
        state.client.writeFully("\003");
      }
      HTTP_ClearOut();
      if ( ! notHTTP)
        state.client.writeFully(data_html_b);
    }
  }

  state.client.flush();

  if (dontClose)
    return;

  // Half close the connection, per
  // [Tear-down](https://datatracker.ietf.org/doc/html/rfc7230#section-6.6)
  state.client.closeOutput();
  state.outputClosed = true;

  vTaskDelay(1);
  state.client.close();
  state.closed = true;

  state.closedTime = millis();
}

static void TCP_Server_thread(void *pvParameters) {
  while (1) {
    vTaskDelay(1);
    EthernetClient client = TCPserver.accept();
    if (client) {
      // We got a connection!
      IPAddress ip = client.remoteIP();
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "Client connected: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
      clients.emplace_back(std::move(client));
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "Client count: %zu\r\n", clients.size());
    }

    // Process data from each client
    for (ClientState &state : clients) {  // Use a reference so we don't copy
      vTaskDelay(1);
      if ( ! state.client.connected()) {
        state.closed = true;
        continue;
      }

#if 0
      /* do this when we got an ASCII request, for BINARY - keep it open */
      // Check if we need to force close the client
      if (state.outputClosed) {
        if (millis() - state.closedTime >= kShutdownTimeout) {
          IPAddress ip = state.client.remoteIP();
          print_log(UART_OUT, "Client shutdown timeout: %u.%u.%u.%u\r\n",
               ip[0], ip[1], ip[2], ip[3]);
          state.client.close();
          state.closed = true;
          continue;
        }
      }
      else {
        if (millis() - state.lastRead >= kClientTimeout) {
          IPAddress ip = state.client.remoteIP();
          print_log(UART_OUT, "Client timeout: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
          state.client.close();
          state.closed = true;
          continue;
        }
      }
#endif

      processClientData(state);
    } /* end for */

    // Clean up all the closed clients
    size_t size = clients.size();
    clients.erase(std::remove_if(clients.begin(), clients.end(),
                               [](const auto &state) { return state.closed; }),
                clients.end());
    if (clients.size() != size) {
      if (gCFGparams.DebugFlags & DBG_NETWORK)
        print_log(UART_OUT, "New client count: %zu\r\n", clients.size());
    }
  } /* end while */
}

uint32_t HTTPD_GetIPAddress(void) {
  return Ethernet.localIP();
}

unsigned int HTTPD_GetClientNumber(void) {
  return clients.size();
}

int HTTPD_GetETHLinkState(void) {
  return ETHLinkStatus;
}

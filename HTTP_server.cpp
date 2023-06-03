
#include <core_id.h>
#if !( defined(CORE_TEENSY) && defined(__IMXRT1062__) && defined(ARDUINO_TEENSY41) )
  #error Only Teensy 4.1 supported
#endif

#include "VCP_UART.h"
#include "cmd_dec.h"
#include "HTTP_data.cpp"

// Debug Level from 0 to 4
#define _TEENSY41_ASYNC_TCP_LOGLEVEL_       1
#define _AWS_TEENSY41_LOGLEVEL_             1

#define SHIELD_TYPE     "Teensy4.1 QNEthernet"

#if (_AWS_TEENSY41_LOGLEVEL_ > 3)
  #warning Using QNEthernet lib for Teensy 4.1. Must also use Teensy Packages Patch or error
#endif

#define USING_DHCP            true
//#define USING_DHCP            false

#if !USING_DHCP
  // Set the static IP address to use if the DHCP fails to assign
  IPAddress myIP(192, 168, 2, 222);
  IPAddress myNetmask(255, 255, 255, 0);
  IPAddress myGW(192, 168, 2, 1);
  //IPAddress mydnsServer(192, 168, 2, 1);
  IPAddress mydnsServer(8, 8, 8, 8);
#endif

#include "QNEthernet.h"
using namespace qindesign::network;

#include <AsyncWebServer_Teensy41.h>

AsyncWebServer    server(80);
AsyncWebServer    server2(8080);

int reqCount = 0;                // number of requests received

void handleRoot(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print(data_html_a);

  //process "CMD=command_line" and insert TEXTAREA
  {
    int params = request->params();

    for (int i=0;i<params;i++)
    {
      AsyncWebParameter* p = request->getParam(i);
  
      if (p->isFile())
      {
        response->printf("FILE[%s]: %s, size: %u\r\n", p->name().c_str(), p->value().c_str(), p->size());
      } 
      else if (p->isPost())
      {
        response->printf("POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
      } 
      else 
      {
        //this is "/GET CMD=command_line" - here it works with space separated arguments - WebBrowser does for use
        ////response->printf("GET[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        CMD_DEC_execute((char *)(p->value().c_str()), HTTPD_OUT);
      }
    }
  }

  {
    int l;
    char *b;
    HTTP_PutEOT();                    //place EOT into buffer
    l = HTTP_GetOut(&b);
    if (l)
      response->print(b);
    HTTP_ClearOut();
  }
  response->print(data_html_b);
  request->send(response);            //it is needed, but closes the connection!
}

void handleNotFound(AsyncWebServerRequest *request)
{
  String message;

  String str = request->url().c_str();
  const char *cstr = str.c_str();

  if (*cstr == '/') {
    CMD_DEC_execute((char *)++cstr, HTTPD_OUT);
    {
      int l;
      char *b;
      ////HTTP_PutEOT();                    //place EOT into buffer - it seems to kill the WebBrowser session!
      l = HTTP_GetOut(&b);
      if (l)
        message += b;
      HTTP_ClearOut();
    }
  }
  else {
    //not possible to enter URL without '/' in WebBrowser
    message += "BIN command";
  }

  request->send(200, "text/plain", message);
}

void handleNotFound2(AsyncWebServerRequest *request)
{
  /* send a request without '/', just "GET " and following command:
   * but it fails with space separated command line arguments!
   * the URL (URI) must be encoded with %20 (etc.), because the URL is parsed and a space cuts all remaining parts!
   * ATT: this generates also a header, with "200, text/plain, Connection: close", but we do not need and do not want this
   * header - HOW? also this conection should not be closed, but it looks like - the MCU does not close, good
   */
  String str = request->url().c_str();
  const char *cstr = str.c_str();

  //for debug - on UART
  size_t l = strlen(cstr);
  Serial.printf("L: %d\r\n", l);
  for (size_t i = 0; i < l; i++)
  {
    Serial.printf("%02X ", cstr[i]);
  }
  Serial.println();
  //end debug

  CMD_DEC_execute((char *)cstr, HTTPD_OUT);

  {
    int l;
    char *b;
    HTTP_PutEOT();                    //place EOT into buffer - needed by Python script!
    l = HTTP_GetOut(&b);
    if (l)
      request->send(200, "text/plain", b);
      Serial.println(b);
    HTTP_ClearOut();
  }
}

void HTTPD_setup(void)
{
#if USING_DHCP
  // Start the Ethernet connection, using DHCP
  Ethernet.begin();
#else
  // Start the Ethernet connection, using static IP
  Ethernet.begin(myIP, myNetmask, myGW);
  Ethernet.setDNSServerIP(mydnsServer);
#endif

  if (!Ethernet.waitForLocalIP(5000))
  {
    Serial.println(F("*E: Failed to configure Ethernet"));

    if (!Ethernet.linkStatus())
    {
      Serial.println(F("*E: Ethernet cable is not connected."));
    }

    // Stay here forever in case of error
    while (true)
    {
      delay(1);
    }
  }

#if USING_DHCP
  delay(1000);
#else
  delay(2000);
#endif

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleRoot(request);
  });

  server.onNotFound(handleNotFound);

  server.begin();

  //it looks like you can still activate the callbacks on the other port
  server2.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleRoot(request);      //this reuses the callback on port 80: if we request with "GET /command",
                              //this is handled here
  });

  server2.onNotFound(handleNotFound2);

  server2.begin();
}

uint32_t HTTPD_GetIPAddress(void) {
  return Ethernet.localIP();
}


#include <core_id.h>
#if !( defined(CORE_TEENSY) && defined(__IMXRT1062__) && defined(ARDUINO_TEENSY41) )
  #error Only Teensy 4.1 supported
#endif

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

const int led = 13;

void handleRoot(AsyncWebServerRequest *request)
{
  digitalWrite(led, 1);

#define BUFFER_SIZE     400

  char temp[BUFFER_SIZE];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  //int day = hr / 24;

  snprintf(temp, BUFFER_SIZE - 1,
           "<html>\
<head>\
<meta http-equiv='refresh' content='1'/>\
<title>AsyncWebServer-%s</title>\
<style>\
body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
</style>\
</head>\
<body>\
<h2>AsyncWebServer_Teensy41!</h2>\
<h3>running on %s</h3>\
<p>Port: <b>%d</b> Uptime: %02d:%02d:%02d</p>\
<img src=\"/test.svg\" />\
</body>\
</html>", BOARD_NAME, BOARD_NAME, /*day*/ request->client()->getLocalPort(), hr % 24, min % 60, sec % 60);

  request->send(200, "text/html", temp);

  digitalWrite(led, 0);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";

  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";

  for (uint8_t i = 0; i < request->args(); i++)
  {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }

  request->send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void drawGraph(AsyncWebServerRequest *request)
{
  String out;

  out.reserve(4000);
  char temp[70];

  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"310\" height=\"150\">\n";
  out += "<rect width=\"310\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"2\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"blue\">\n";
  int y = rand() % 130;

  for (int x = 10; x < 300; x += 10)
  {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"2\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }

  out += "</g>\n</svg>\n";

  request->send(200, "image/svg+xml", out);
}


void HTTPD_setup(void)
{
#if 0
  Serial.print("\nStart Async_AdvancedWebServer on ");
  Serial.print(BOARD_NAME);
  Serial.print(" with ");
  Serial.println(SHIELD_TYPE);
  Serial.println(ASYNC_WEBSERVER_TEENSY41_VERSION);

  delay(500);
#endif

#if USING_DHCP
  // Start the Ethernet connection, using DHCP
#if 0
  Serial.print("Initialize Ethernet using DHCP => ");
#endif
  Ethernet.begin();
#else
  // Start the Ethernet connection, using static IP
#if 0
  Serial.print("Initialize Ethernet using static IP => ");
#endif
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

    // Stay here forever
    while (true)
    {
      delay(1);
    }
  }
  else
  {
#if 0
    Serial.print(F("*I: Connected! IP address:"));
    Serial.println(Ethernet.localIP());
#endif
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

  server.on("/test.svg", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    drawGraph(request);
  });

  server.on("/inline", [](AsyncWebServerRequest * request)
  {
    request->send(200, "text/plain", "This works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();

  //
  server2.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleRoot(request);
  });

  server2.on("/test.svg", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    drawGraph(request);
  });

  server2.on("/inline", [](AsyncWebServerRequest * request)
  {
    request->send(200, "text/plain", "This works as well");
  });

  server2.onNotFound(handleNotFound);

  server2.begin();

#if 0
  Serial.print(F("\r\n*I: HTTP MCU IP address : "));
  Serial.println(Ethernet.localIP());
#endif
}

uint32_t HTTPD_GetIPAddress(void) {
  return Ethernet.localIP();
}

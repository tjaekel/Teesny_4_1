
#if 0
////#include <SPI.h>
#include <NativeEthernet.h>
#include <TeensyThreads.h>

void thread_HTTPD(void);
void thread_NETCMD(void);

Threads::Mutex serverLock;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 196);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
EthernetServer serverNETCMD(8080);

void HTTPD_setup(void) {
  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit Featherwing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit Featherwing Ethernet

#if 0
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif
  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at: ");
  Serial.println(Ethernet.localIP());

  // run as a RTOS thread
  threads.addThread(thread_HTTPD);
  threads.addThread(thread_NETCMD);
}

uint32_t HTTPD_GetIPAddress(void) {
  return Ethernet.localIP();
}

void thread_HTTPD(void) {
  static uint32_t cnt = 0;

  while(1) {
    // listen for incoming clients

    serverLock.lock();

    EthernetClient client = server.available();
    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = false;
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
#if 0
          //this prints all received characters on Serial
          Serial.write(c);
#endif
          // if you've gotten to the end of the line (received a newline
          // character) and the line is blank, the http request has ended,
          // so you can send a reply
          if (c == '\n' && currentLineIsBlank) {
            // send a standard http response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println("Refresh: 1");         // refresh the page automatically every 5 sec
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            // avoid to request all the time a favicon - otherwise, we get always two requests!
            client.println("<head><link rel=\"icon\" href=\"data:,\"></head><body>");

#if 0
            // output the value of each analog input pin
            for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
              int sensorReading = analogRead(analogChannel);
              client.print("analog input ");
              client.print(analogChannel);
              client.print(" is ");
              client.print(sensorReading);
              client.println("<br />");
            }
#else
            client.print("<h2>Welcome on Teensy: ");
            client.print(cnt);
            Serial.print("XXXX: "); Serial.println(cnt);
            cnt++;
            client.println("</h2>");
#endif
            client.println("</body></html>");
            break;
          }

          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }
      // give the web browser time to receive the data
      //delay(1);
      threads.delay(1);
      // close the connection:
      client.stop();
      Serial.println("client disconnected");

    }
    ////threads.delay(1);
    threads.yield();

    serverLock.unlock();
  }
}

void thread_NETCMD(void) {
  static uint32_t cnt = 0;

  while(1) {
    // listen for incoming clients

    serverLock.lock();

    EthernetClient client = serverNETCMD.available();
    if (client) {
      Serial.println("new client");
      // an http request ends with a blank line
      boolean currentLineIsBlank = false;
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
#if 0
          //this prints all received characters on Serial
          Serial.write(c);
#endif
          // if you've gotten to the end of the line (received a newline
          // character) and the line is blank, the http request has ended,
          // so you can send a reply
          if (c == '\n' && currentLineIsBlank) {
            // send a standard http response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println("Refresh: 1");         // refresh the page automatically every 5 sec
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            // avoid to request all the time a favicon - otherwise, we get always two requests!
            client.println("<head><link rel=\"icon\" href=\"data:,\"></head><body>");

#if 0
            // output the value of each analog input pin
            for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
              int sensorReading = analogRead(analogChannel);
              client.print("analog input ");
              client.print(analogChannel);
              client.print(" is ");
              client.print(sensorReading);
              client.println("<br />");
            }
#else
            client.print("<h2>Teensy CMD: ");
            client.print(cnt);
            Serial.print("XXXX: "); Serial.println(cnt);
            cnt++;
            client.println("</h2>");
#endif
            client.println("</body></html>");
            break;
          }

          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }
      }
      // give the web browser time to receive the data
      //delay(1);
      threads.delay(1);
      // close the connection:
      client.stop();
      Serial.println("client disconnected");
    }
    ////threads.delay(1);
    threads.yield();

    serverLock.unlock();
  }
}

#else
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

#include "QNEthernet.h"       // https://github.com/ssilverman/QNEthernet
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
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  delay(200);
#endif

  Serial.print("\nStart Async_AdvancedWebServer on ");
  Serial.print(BOARD_NAME);
  Serial.print(" with ");
  Serial.println(SHIELD_TYPE);
  Serial.println(ASYNC_WEBSERVER_TEENSY41_VERSION);

  delay(500);

#if USING_DHCP
  // Start the Ethernet connection, using DHCP
  Serial.print("Initialize Ethernet using DHCP => ");
  Ethernet.begin();
#else
  // Start the Ethernet connection, using static IP
  Serial.print("Initialize Ethernet using static IP => ");
  Ethernet.begin(myIP, myNetmask, myGW);
  Ethernet.setDNSServerIP(mydnsServer);
#endif

  if (!Ethernet.waitForLocalIP(5000))
  {
    Serial.println(F("Failed to configure Ethernet"));

    if (!Ethernet.linkStatus())
    {
      Serial.println(F("Ethernet cable is not connected."));
    }

    // Stay here forever
    while (true)
    {
      delay(1);
    }
  }
  else
  {
    Serial.print(F("Connected! IP address:"));
    Serial.println(Ethernet.localIP());
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

  Serial.print(F("HTTP MCU IP address : "));
  Serial.println(Ethernet.localIP());
}

uint32_t HTTPD_GetIPAddress(void) {
  return Ethernet.localIP();
}

#endif

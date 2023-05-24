
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

  //run as a RTOS thread
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
            client.println("Refresh: 5");         // refresh the page automatically every 5 sec
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
    threads.delay(1);
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
            client.println("Refresh: 5");         // refresh the page automatically every 5 sec
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
    threads.delay(1);
    threads.yield();

    serverLock.unlock();
  }
}

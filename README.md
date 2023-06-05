# Teesny_4_1
 Arduino Teensy 4.1 project

## Features
- USB VCP UART shell (for interactive commands)
- SPI master, with "spiclk" and "rawspi" commands (change speed, send any SPI transaction via command line)
- use SPI in DMA mode (fastest speed on SPI possible)
- ETH HTTPD server - up and running (port 80 and 8080, 2x)
- code to run SPI also as slave (as compile option, speed limited to 20 Mbps)
- initialize and use SD Card (from command line): "sdinit 1"
- start TFTP from command line, via "tftp 1"
- Pico-C: C-code command line interpreter (write C-code without a compiler)

## Work Items
- add TFTP to/from SD Card via ETH - DONE<br>
  activate from UART command line via "tftp", run TFTP as a thread (in background, not blocking UART command line)<br>
  change tftp_server to QNEthernet - DONE
- add Pico-C (C-code interpreter, using EXTMEN for scripts) - DONE<br>
  activate from UART command line via "picoc", works
- add HTTPD server web pages, with forms to enter commands in Web Browser - DONE 
  (no need for Telnet)<br>
  use QNEthernet and AsyncWebServer: two instances (port 80 and 8080) are working in parallel - DONE
- a second SPI, a shared SPI (same bus with two PCS/SS signals) - TODO

## Pico-C needs EXTMEM
ATTENTION: the Pico-C "picoc" uses the external QSPI memory (not flash, both as RAM, 16 MB)<br>
You have to have 2x QSPI RAM chip soldered, otherwise disable Pico-C and do not start (via "picoc" on UART command line)

## Stress Test
- get the MCU IP address: on UART command line: "ipaddr"<br>
  it will display the <MCUIPAddr> (DHCP is used)
- two HTTPD servers are running: open two Web Browsers and enter as URLs:<br>
  <MCUIPAddr> (on default port 80 for HTTP) and <MCUIPAddr>:8080 (the second server on port 8080)
- start TFTP: "sdinit 1" and "tftp 1" on UART command shell<br>
  tansfer a file as test: open CMD window on host PC and enter: "tftp -i <MCUIPAddr> put <filename>"
- start Pico-C: "picoc" on UART command shell (a different prompt as ':')
- enter these picoc command lines:<br>
  int i;<br>
  i = 0;<br>
  while (1) { printf("i: %d\r\n", i++); mssleep(1000); }<br>
- it should run both WebBrowsers, do a TFTP file tansfer and display the incrementing 'i' every second (picoc)

## Python Script
A Python script to use server on port 8080 is added: fire MCU commands from
an interactrive Python command loop (like TELNET).<br>
Modify the IP address in Python script for your MCU (DHCP is used - different IP address).<br>
The port 8080 is based on QNEthernet TCP server (as native TCP server), running as a TeensyThread.


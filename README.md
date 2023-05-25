# Teesny_4_1
 Arduino Teensy 4.1 project

## Features
- USB VCP UART shell (for interactive commands)
- SPI master, with "spiclk" and "rawspi" commands (change speed, send any SPI transaction via command line)
- use SPI in DMA mode (fastest speed on SPI possible)
- ETH HTTPD server - up and running running
- code to run SPI also as slave (compile option)
- initialize and use SD Card (from command line)

## Work Items
- add TFTP to/from SD Card via ETH - DONE<br>
  activate from UART command line via "tftp", run TFTP as a thread (in background, not blocking UART command line)<br>
  change tftp_server to QNEthernet - DONE
- add Pico-C (C-code interpreter, using EXTMEN for scripts) - DONE<br>
  activate from UART command line via "picoc", works, just add additional commands, e.g. to do a SPI transaction, delay, etc.
- add HTTPD server web pages, with forms to enter commands in Web Browser
  (no need for Telnet)<br>
  use QNEthernet and AsyncWebServer: two instances (port 80 and 8080) are working in parallel - DONE
- a second SPI, a shared SPI (same bus with two PCS/SS signals)


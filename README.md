# Teesny_4_1
 Arduino Teensy 4.1 project

## Features
- USB VCP UART shell (for interactive commands)
- SPI master, with "spiclk" and "rawspi" commands (change speed, send any SPI transaction via command line)
- use SPI in DMA mode (fastest speed on SPI possible)
- ETH HTTPD server - up and running running
- code to run SPI also as slave (compile option)
- initialize and use SD Card (from command line)

##Intentions
- add TFTP to/from SD Card via ETH
- add Pico-C (C-code interpreter, using EXTMEN for scripts)
- add HTTPD server web pages, with forms to enter commands in Web Browser
  (no need for Telnet)
- a second SPI, a shared SPI (same bus with two PCS/SS signals)


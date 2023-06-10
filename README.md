# Teesny_4_1
 Arduino Teensy 4.1 project<br>
 version: 1.1.0

## Features
- USB VCP UART shell (for interactive commands)
- SPI master, with "spiclk" and "rawspi" commands (change speed, send any SPI transaction via command line)
- use SPI in DMA mode (fastest speed on SPI possible)
- ETH HTTPD server - up and running (port 80)
- code to run SPI also as slave (as compile option, speed limited to 20 Mbps)
- initialize and use SD Card (from command line): "sdinit 1"
- start TFTP on init, running in background, needs "sdinit 1" first
- Pico-C: C-code command line interpreter (write C-code without a compiler)
- GPIO interrupt: ISR triggers a handler thread to do all stuff outside INT context<br>
  call an (optional) set INT handler in Pico-C
- send UDP packet, with a 16bit counter, send UDP data in INT handler (e.g. a FIFO drained)

## Pico-C needs EXTMEM
ATTENTION: the Pico-C "picoc" uses the external QSPI memory (not flash, both as RAM, 16 MB)<br>
You have to have 2x QSPI RAM chip soldered, otherwise disable Pico-C and do not start (via "picoc" on UART command line)

## Python Script
A Python script example to use server on port 80 is added: fire MCU commands from
an interactive Python command loop (like TELNET), use binary commands...<br>
Modify the IP address in Python script for your MCU (DHCP is used - different IP address).<br>
The port 80 is based on QNEthernet TCP server (as native TCP server), running as a FreeRTOS thread.<br>
Python can send HTTP (TEXT) requests or BINARY (with command and length field).

## Prerequisites
This project uses now FreeRTOS, not TeensyThreads anymore! (needed for GPIO interrupts without polling)
You have to install the ZIP library:<br>
see and get from here:<br>
https://github.com/tsandmann/freertos-teensy

## How to use?
When compiled and flashed, open UART terminal (any baudrate is fine)<br>
enter command "help" and see implemented commands (or: help(); inside Pico-C)

## Contact
You can send me an email: tj@tjaekel.com


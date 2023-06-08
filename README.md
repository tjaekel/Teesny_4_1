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

## Work Items
- add TFTP to/from SD Card via ETH - DONE
- add Pico-C (C-code interpreter, using EXTMEN for scripts) - DONE<br>
  launch from UART command line via "picoc"
- Pico-C: break endless loops, via CTRL-C on UART - DONE
- Pico-C INT handler - triggered by GPIO interrupt - DONE
- add HTTPD server web page, with forms to enter commands in Web Browser - DONE 
  (no need for Telnet: use Pythong for a network command session)<br>
  use QNEthernet: one instance (port 80), usable from Python (in BINARY mode) and WebBrowser (TEXT mode) - DONE
- a second SPI, a shared SPI (same bus with two PCS/SS signals) - TODO

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


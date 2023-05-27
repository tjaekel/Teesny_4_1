# Teesny_4_1
 Arduino Teensy 4.1 project

## Features
- USB VCP UART shell (for interactive commands)
- SPI master, with "spiclk" and "rawspi" commands (change speed, send any SPI transaction via command line)
- use SPI in DMA mode (fastest speed on SPI possible)
- ETH HTTPD server - up and running (port 80 and 8080, 2x)
- code to run SPI also as slave (compile option)
- initialize and use SD Card (from command line)
- start TFTP from command line, via "tftp 1"
- Pico-C: C-code command line interpreter (write C-code without a compiler)

## Work Items
- add TFTP to/from SD Card via ETH - DONE<br>
  activate from UART command line via "tftp", run TFTP as a thread (in background, not blocking UART command line)<br>
  change tftp_server to QNEthernet - DONE
- add Pico-C (C-code interpreter, using EXTMEN for scripts) - DONE<br>
  activate from UART command line via "picoc", works, just add additional commands, e.g. to do a SPI transaction, delay, etc.<br>
  ATT: when "tftp 1" is running - Pico-C does not work! (crash)
- add HTTPD server web pages, with forms to enter commands in Web Browser
  (no need for Telnet)<br>
  use QNEthernet and AsyncWebServer: two instances (port 80 and 8080) are working in parallel - DONE
- a second SPI, a shared SPI (same bus with two PCS/SS signals)

## Issues
- the use of threads, e.g. via TeensyThreads - does not work (it crashes)
- we need all as non-blocking "threads" done in loop() (but TFTP is still a
  real thread, but it seems to create conflicst)

## Pico-C needs EXTMEM
ATTENTION: the Pico-C "picoc" uses the external QSPI memory (not flash, as RAM)<br>
You have to have 2x QSPI RAM chip soldered, otherwise disable Pico-C and do not start (via "picoc" on UART command line)


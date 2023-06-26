#pragma once
#ifndef __DEBUG_SYS__
#define __DEBUG_SYS__

/* ------------------------- configuration ---------------------------------*/
//#define STATIC_IP
//#define DEBUG                                       //UART as debug log for this code
//#define WAIT_FOR_UART                               //on startup: wait for UART connected or not
#define HTTPD_CMD_LEN           (64*1024+16)          //len of ASCII command string via HTTPD
#define HTTPD_RXBUF_LEN         (HTTPD_CMD_LEN+8)     //total length HTTPD receiver buffer, also for BINARY mode
#define HTTPD_RXCHUNK_LEN       SOCKET_BUFFER_SIZE    //it never gets more as 256 bytes per client.read() call, no need to set larger
#define HTTPD_BUF_SIZE          (4096*4)              //total length HTTPD response buffer (for ASCII commands)
#define DONT_CLOSE_CONNECTION                         //close connection in BINARY mode or not
#define HTTPD_BUF_REQ_SIZE      (4*1024)              //size of string for HTTPD request

//#define WITH_ITCM									//use ITCM memory and initialize code
//#define CLOCK_CFG									//configure 480 MHz
//#define WITH_SDCARD								//with SDCar init and commands
//#define WITH_SDRAM                //with SDRAM init
//#define WITH_I2C									//with I2C functions

#endif

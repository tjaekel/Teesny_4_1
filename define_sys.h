#pragma once
#ifndef __DEBUG_SYS__
#define __DEBUG_SYS__

/* ------------------------- configuration ---------------------------------*/
#define WITH_SDCARD
//#define STATIC_IP
//#define DEBUG                                       //UART as debug log for this code
#define DONT_CLOSE_CONNECTION                         //close connection in BINARY mode or not
#define HTTPD_BUF_REQ_SIZE      (4*1024 + 1)          //size of string for HTTPD request, >= LINE_BUFFER_LEN

#endif

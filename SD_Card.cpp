
#include <SD.h>
#include "VCP_UART.h"
#include "SD_Card.h"
#include "cmd_dec.h"
#include "MEM_Pool.h"
#include "SYS_error.h"

// change this to match your SD shield or module;
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20
// Wiz820+SD board: pin 4
// Teensy audio board: pin 10
// Teensy 3.5 & 3.6 & 4.1 on-board: BUILTIN_SDCARD
const int chipSelect = BUILTIN_SDCARD;

int SDCARD_init = 0;

static void printSpaces(int num, EResultOut out);
#if 0
static void printTime(const DateTimeFields tm);
#endif

void SDCARD_setup(EResultOut out) {
  if ( ! SDCARD_init) {
    if (!SD.begin(chipSelect)) {
      UART_printString("*E: SD initialization failed", out);
      return;
    }
    SDCARD_init = 1;
  }

  SDCARD_printDirectory("/", 0, out);
}

void SDCARD_ShowDir(EResultOut out) {
  SDCARD_printDirectory("/", 0, out);
}

void SDCARD_deinit(void) {
  ////SD.end();//does not exist!
  SDCARD_init = 0;
}

int SDCARD_GetStatus(void) {
  return  SDCARD_init;
}

void SDCARD_printDirectory(const char *str, int numSpaces, EResultOut out) {
  File dir = SD.open(str);

  if ( ! SDCARD_init)
    return;

   while(true) {
     File entry = dir.openNextFile();
     if (! entry) {
       break;
     }
     printSpaces(numSpaces, out);
     UART_printString(entry.name(), out);
     if (entry.isDirectory()) {
       UART_printString("/\r\n", out);
       SDCARD_printDirectory(entry.name(), numSpaces+2, out);
     } else {
#if 0
       // files have sizes, directories do not
       int n = log10f(entry.size());
       if (n < 0) n = 10;
       if (n > 10) n = 10;
       printSpaces(50 - numSpaces - strlen(entry.name()) - n);
       Serial.print("  ");
       Serial.print(entry.size(), DEC);
       DateTimeFields datetime;
       if (entry.getModifyTime(datetime)) {
         printSpaces(4);
         printTime(datetime);
       }
#endif
       UART_printString("\r\n", out);
     }
     entry.close();
   }
}

static void printSpaces(int num, EResultOut out) {
  for (int i=0; i < num; i++) {
    UART_printChar(' ', out);
  }
}

#if 0
static void printTime(const DateTimeFields tm) {
  const char *months[12] = {
    "January","February","March","April","May","June",
    "July","August","September","October","November","December"
  };
  if (tm.hour < 10) Serial.print('0');
  Serial.print(tm.hour);
  Serial.print(':');
  if (tm.min < 10) Serial.print('0');
  Serial.print(tm.min);
  Serial.print("  ");
  Serial.print(tm.mon < 12 ? months[tm.mon] : "???");
  Serial.print(" ");
  Serial.print(tm.mday);
  Serial.print(", ");
  Serial.print(tm.year + 1900);
}
#endif

int SDCARD_PrintFile(char *file, EResultOut out) {
  if ( ! SDCARD_init)
    return 0;           //not initialized

  // open the file.
  File dataFile = SD.open(file);

  // if the file is available, read from it and print:
  if (dataFile) {
    while (dataFile.available()) {
      UART_printChar((char)dataFile.read(), out);
    }
    dataFile.close();
    //append a newline?
    return 1;
  }  
  // if the file isn't open, pop up an error:
  else {
    print_log(out, "*E: error opening file '%s'\r\n", file);
    return 0;
  }
}

int SDCARD_ReadFile(const char *file, unsigned char *b) {
  int i = 0;
  if ( ! SDCARD_init)
    return 0;           //not initialized

  // open the file.
  File dataFile = SD.open(file);

  // if the file is available, read from it and store:
  if (dataFile) {
    while (dataFile.available()) {
      *b++ = (unsigned char)dataFile.read();
      i++;
    }
    dataFile.close();
    return i;
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.print("*E: error opening file");
    Serial.println(file);
    return 0;
  }
}

int SDCARD_fgets(char *p, int len, void *file) {
  int i = 0;
  int state = 0;
  static char pbChar = '\0';
  File *f = (File *)file;

  if (pbChar != '\0') {
    *p++ = pbChar;
    i++;
    pbChar = '\0';
  }

  while (f->available()) {
    *p = (char)f->read();
    i++;

    if ((*p == '\r') || (*p == '\n'))
      state = 1;
    else {
      if (state) {
        pbChar = *p;
        break;
      }
    }

    p++;
    i++;

    if (i >= len)
      break;
  }

  *p = '\0';

  return i;
}

#define FILE_LINE_SIZE  MEM_POOL_SEG_BYTES

ECMD_DEC_Status SDCARD_Exec(TCMD_DEC_Results *res, EResultOut out)
{
	char *fileLine;		/* buffer for single line */
	int silent = 0;
  ECMD_DEC_Status err = CMD_DEC_OK;

	if(SDCARD_GetStatus() && res->str)
	{
		fileLine = (char *)MEM_PoolAlloc(FILE_LINE_SIZE);
		if ( ! fileLine)
		{
			return CMD_DEC_OOMEM;
		}

    if (res->opt)
		  if (strncmp(res->opt, "-s", 2) == 0)
			  silent = 1;

    File MyFile = SD.open(res->str);
		if( ! MyFile)
		{
			MEM_PoolFree(fileLine);
			return CMD_DEC_ERROR;			//error
		}
		else
		{
		    while (SDCARD_fgets(fileLine, FILE_LINE_SIZE -1, &MyFile) != 0)
		    {
		    	if ( ! silent)
		    	{
#if 0
		    		/* print to see as log what came from file, '$' is 'file prompt' */
		    		UART_Send((const char *)"$ ", 2, out);
		    		/* log entire line - even with comments */
		    		UART_Send((const char *)fileLine, (int)strlen(fileLine), out);
		    		if (out != HTTPD_OUT)
		    			/* just in case there is not '\r' in file, just Linux '\n' */
		    			UART_Send((const char *)"\r", 1, out);
#else
		    		/* strip of the comments */
		    		{
		    			char *strComment;
		    			int strLenLine;
		    			int appendNL = 0;
		    			strComment = strchr(fileLine, '#');
		    			if (strComment)
		    			{
		    				strLenLine = strComment - fileLine;
		    				if (strLenLine)				/* avoid empty line if just commented line */
		    					appendNL = 2;
		    			}
		    			else
		    			{
		    				strLenLine = (int)strlen(fileLine);
		    				/* is there a \r from file? we assume there is always a \n from Unix file */
		    				if (strchr(fileLine, '\r') == NULL)
		    					appendNL = 1;
		    			}
		    			if (strLenLine)
		    			{
		    				UART_Send((const char *)"$ ", 2, out);
		    				UART_Send((const char *)fileLine, strLenLine, out);
		    			}
		    			else
		    				continue;
		    			if (appendNL == 2)
		    				UART_Send((const char *)"\r\n", 2, out);
		    			if (appendNL == 1)
		    				UART_Send((const char *)"\r", 1, out);		//it becomes \n\r
		    		}
#endif
		    	}

		    	/* now execute command */
		    	CMD_DEC_execute(fileLine, out);

		    	/* Q: should we stop on first CMD error, e.g. wrong command? */
		    } /* end while */

		    MyFile.close();

		    MEM_PoolFree(fileLine);
		    return err;
		}
	}
	else
		return CMD_DEC_ERROR;
}

int SDCARD_format(void) {
  bool res;
  res = SD.format();

  if (res)
    return 1;
  else
    return 0;
}

int SDCARD_delete(const char *fname) {
  bool res;
  res = SD.remove(fname);

  if (res)
    return 1;
  else
    return 0;
}

int SDCARD_writeFile(const char *fname, const char *buf) {
  File MyFile = SD.open(fname, FILE_WRITE_BEGIN);

  if (MyFile) {
    MyFile.write(buf, strlen(buf));

    MyFile.close();

    return 1;
  }
  return 0;
}
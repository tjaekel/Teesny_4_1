
#include <SD.h>
#include "SD_Card.h"

// change this to match your SD shield or module;
// Teensy 2.0: pin 0
// Teensy++ 2.0: pin 20
// Wiz820+SD board: pin 4
// Teensy audio board: pin 10
// Teensy 3.5 & 3.6 & 4.1 on-board: BUILTIN_SDCARD
const int chipSelect = BUILTIN_SDCARD;

int SDCARD_init = 0;

static void printSpaces(int num);
static void printTime(const DateTimeFields tm);

void SDCARD_setup(void) {
  if ( ! SDCARD_init) {
    //Serial.print("Initializing SD card...");

    if (!SD.begin(chipSelect)) {
      Serial.println("initialization failed!");
      return;
    }
    //Serial.println("initialization done.");
    SDCARD_init = 1;
  }

  SDCARD_printDirectory("/", 0);
  
  //Serial.println("done!");
}
void SDCARD_deinit(void) {
  ////SD.end();//does not exist!
  SDCARD_init = 0;
}

void SDCARD_printDirectory(const char *str, int numSpaces) {
  File dir = SD.open(str);

  if ( ! SDCARD_init)
    return;

   while(true) {
     File entry = dir.openNextFile();
     if (! entry) {
       //Serial.println("** no more files **");
       break;
     }
     printSpaces(numSpaces);
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       SDCARD_printDirectory(entry.name(), numSpaces+2);
     } else {
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
       Serial.println();
     }
     entry.close();
   }
}

static void printSpaces(int num) {
  for (int i=0; i < num; i++) {
    Serial.print(" ");
  }
}

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

int SDCARD_PrintFile(char *file) {
  if ( ! SDCARD_init)
    return 0;           //not initialized

  // open the file.
  File dataFile = SD.open(file);

  // if the file is available, read from it and print:
  if (dataFile) {
    while (dataFile.available()) {
      Serial.write(dataFile.read());
    }
    dataFile.close();
    return 1;
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening ");
    Serial.println(file);
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
    Serial.print("error opening ");
    Serial.println(file);
    return 0;
  }
}

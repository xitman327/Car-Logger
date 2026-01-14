#include <Arduino.h>
#include <SPI.h>
// #include <SD.h>

#ifndef DISABLE_FS_H_WARNING
#define DISABLE_FS_H_WARNING  // Disable warning for type File not defined. 
#endif  // DISABLE_FS_H_WARNING 
#include "SdFat.h"


#define SD_CS 15
#define SD_MOSI 7
#define SD_MISO 6
#define SD_SCK 5
#define SD_DETECT 16

SdFat32 sd;
File32 sdfile;


int sd_ready = 0;

void setup_sd(){
    // TEST BOARD ONLY, SHARED BUS BETWEEN LCD AND TOUCH IC
    // DESELECT THOSE
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);


    // Initialize SPI bus on your chosen pins
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  // Start slow for bring-up
  SdSpiConfig cfg(SD_CS, SHARED_SPI, 10000000 /*10MHz*/, &SPI);

    if(!sd.begin(cfg)){
        Serial.println("SD.begin() failed");
        if(sd.card()->errorCode()){
            Serial.println(sd.card()->errorCode(), HEX);
            printSdErrorText(&Serial, sd.card()->errorCode());
            Serial.println("");
        }
        sd_ready = 0;
        return;
    }else{
        Serial.println("SD init success");
        sd_ready = 1;
    }
    
    
}
#include "Wire.h"
#include "SHT2x.h"

SHT2x sht;

float temperaturas, humidituras;

uint32_t lastTemp = 0;
uint32_t intervalTemp = 2000;
uint32_t lastHum = 0;
uint32_t intervalHum = 5000;

void sensors_setup(){
    // Wire.begin(SDA, SCL);
    if(!sht.begin()){log_e("SHT Sensor not ok"); return;}
    delay(20);
    sht.requestTemperature();
}

void sensors_loop(){

    if(!sht.isConnected()) return;

    uint32_t now = millis();

  //  schedule and handle temperature
  if ((now - lastTemp > intervalTemp) && (sht.getRequestType() == 0x00))
  {
    sht.requestTemperature();
  }
  if (sht.reqTempReady())
  {
    lastTemp = now;
    sht.readTemperature();
    temperaturas = sht.getTemperature();
  }

  //  schedule and handle humidity
  if ((now - lastHum > intervalHum) && (sht.getRequestType() == 0x00))
  {
    sht.requestHumidity();
  }
  if (sht.reqHumReady())
  {
    lastTemp = now;
    sht.readHumidity();
    humidituras = sht.getHumidity();
  }
}
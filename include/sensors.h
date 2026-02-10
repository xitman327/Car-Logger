#include "Wire.h"
#include "SHT2x.h"

SHT2x sht;

float temperaturas, humidituras;
float vin, aux1, aux2, aux3;

#define vin_devider_r1 27000
#define vin_divider_r2 1000
#define aux1_devider_r1 27000
#define aux1_divider_r2 1000
#define aux2_devider_r1 27000
#define aux2_divider_r2 1000
#define aux3_devider_r1 27000
#define aux3_divider_r2 1000

#define VIN_FROM_ADC(adc, R1, R2, VREF) ((float)(adc) * ((R1) + (R2)) / (R2))

uint32_t lastTemp = 0;
uint32_t intervalTemp = 2000;
uint32_t lastHum = 0;
uint32_t intervalHum = 5000;

#define analog_refresh 200
uint32_t tm_analog = 0;

void sensors_setup()
{
  if (!sht.begin())
  {
    log_e("SHT Sensor not ok");
    return;
  }
  delay(20);
  sht.requestTemperature();
  analogSetAttenuation(ADC_0db);
  pinMode(vin_pin, ANALOG);
  pinMode(aux1_pin, ANALOG);
  pinMode(aux2_pin, ANALOG);
  pinMode(aux3_pin, ANALOG);
}

void sensors_loop()
{

  if (sht.isConnected())
  {

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

  if (millis() - tm_analog > analog_refresh)
  {
    tm_analog - millis();

    vin = VIN_FROM_ADC((float)analogReadMilliVolts(vin_pin)/1000.0, vin_devider_r1, vin_divider_r2, 1.1000);
    aux1 = VIN_FROM_ADC((float)analogReadMilliVolts(aux1_pin)/1000.0, aux1_devider_r1, aux1_divider_r2, 1.100);
    aux2 = VIN_FROM_ADC((float)analogReadMilliVolts(aux2_pin)/1000.0, aux2_devider_r1, aux2_divider_r2, 1.100);
    aux3 = VIN_FROM_ADC((float)analogReadMilliVolts(aux3_pin)/1000.0, aux3_devider_r1, aux3_divider_r2, 1.100);
  }
}
#define DEMO_MODE 1
#define DEBUG_SERIAL Serial
#include "user.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include "AutoConnect.h"
#include "ESPTelnet.h"

ESPTelnet telnet;

WebServer Server;
AutoConnect       Portal(Server);
AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4

#include <FirebaseClient.h>


/*

follow guide 
https://randomnerdtutorials.com/esp32-firebase-realtime-database/

create a user.h file inside include folder and populate it with the following

#define Web_API_KEY "REPLACE_WITH_YOUR_FIREBASE_PROJECT_API_KEY"
#define DATABASE_URL "REPLACE_WITH_YOUR_FIREBASE_DATABASE_URL"
#define USER_EMAIL "REPLACE_WITH_FIREBASE_PROJECT_EMAIL_USER"
#define USER_PASS "REPLACE_WITH_FIREBASE_PROJECT_USER_PASS"

*/

void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds


int intValue = 0;
float floatValue = 0.01;
String stringValue = "";


#include "ELMduino.h"


#define ELM_PORT Serial1


ELM327 myELM327;

float consum_l_s /* L/s consumption*/, kms/*Km/s*/, lpkm/*L/km*/, gfps, lbsps, rpmn/*rpm*/, kmph, maf, fuel_level;
bool engine_on;



typedef enum { ENG_RPM,
               SPEED ,
               MAF,
               FUEL_LEVEL
                          } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

void init_elm(){
  DEBUG_SERIAL.println("Attempting to connect to ELM327...");

  if (!myELM327.begin(ELM_PORT, false, 2000))
  {
    DEBUG_SERIAL.println("Couldn't connect to OBD scanner");
    // while (1);
  }else{
    DEBUG_SERIAL.println("Connected to ELM327");
  }

  
}

void telnet_onconnect(String ip){

  DEBUG_SERIAL.println("hello!");
  DEBUG_SERIAL.println(esp_rom_get_reset_reason(0));
  DEBUG_SERIAL.println(esp_rom_get_reset_reason(1));
  DEBUG_SERIAL.println(ESP.getFreeHeap());

}


unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup()
{

  Serial.begin(115200);
  ELM_PORT.begin(38400, SERIAL_8N1, 17, 18, false, 2000);
  telnet.onConnect(telnet_onconnect);
  DEBUG_SERIAL.println("Starting");

  Config.autoReset = false;
  Config.autoReconnect = true;
  Config.reconnectInterval = 6;
  Config.retainPortal = true;
  Config.ota = AC_OTA_BUILTIN;
  Portal.config(Config);
  Portal.begin();

  // WiFi.begin("Wokwi-GUEST", "", 6);

  configTime(0, 0, ntpServer);

  if(!DEMO_MODE){
    init_elm();
  }

  ssl_client.setInsecure();
  // ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  while(!WiFi.isConnected() && millis() < 10000){}
  telnet.begin(23);

}

#define afr_gassoline 14.7
#define dens_gassoline 710.0

#define convertion_time 1000
#define obd_pull_time 200
uint32_t tm_convertion, tm_obdpull, tm_t_consum;
float lts_trip;

void loop()
{
  app.loop();

  if(app.ready()){
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval){
      lastSendTime = currentTime;
      
      // send a string
      stringValue = "value_" + String(currentTime);
      Database.set<String>(aClient, "/test/string", stringValue, processData, "RTDB_Send_String");
      // send an int
      Database.set<int>(aClient, "/test/int", intValue, processData, "RTDB_Send_Int");
      intValue++; //increment intValue in every loop

      // send a string
      floatValue = 0.01 + random (0,100);
      Database.set<float>(aClient, "/test/float", floatValue, processData, "RTDB_Send_Float");

    }
  }






  telnet.loop();

  if(millis() - tm_obdpull > obd_pull_time){
    tm_obdpull = millis();

    if((myELM327.nb_rx_state == ELM_UNABLE_TO_CONNECT || myELM327.nb_rx_state == ELM_NO_DATA) && !DEMO_MODE ){
      init_elm();
    }

    if(DEMO_MODE){
      rpmn = 2400;
      kmph = 123;
      maf = 1.7;
      fuel_level = 20;
    }else{
      switch (obd_state)
      {
        case ENG_RPM:
        {
          float resp = myELM327.rpm();
          
          if (myELM327.nb_rx_state == ELM_SUCCESS)
          {
            rpmn = resp;
            obd_state = SPEED;
          }
          else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
          {
            myELM327.printError();
            obd_state = SPEED;
          }
          
          break;
        }
        
        case SPEED:
        {
          float resp = myELM327.kph();
          
          if (myELM327.nb_rx_state == ELM_SUCCESS)
          {
            kmph = resp;
            obd_state = MAF;
          }
          else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
          {
            myELM327.printError();
            obd_state = MAF;
          }
          
          break;
        }

        case MAF:
        {
          float resp = myELM327.mafRate();
          
          if (myELM327.nb_rx_state == ELM_SUCCESS)
          {
            maf = resp;
            obd_state = FUEL_LEVEL;
          }
          else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
          {
            myELM327.printError();
            obd_state = FUEL_LEVEL;
          }
          
          break;
        }

        case FUEL_LEVEL:
        {
          float resp = myELM327.fuelLevel();
          
          if (myELM327.nb_rx_state == ELM_SUCCESS)
          {
            fuel_level = resp;
            obd_state = ENG_RPM;
          }
          else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
          {
            myELM327.printError();
            obd_state = ENG_RPM;
          }
          
          break;
        }
      }
    }
  }

  



  if(millis() - tm_convertion > convertion_time){
    tm_convertion = millis();

    if(rpmn > 500){
      engine_on = 1;
    }else{
      engine_on = 0;
    }

    // consum_l_s = maf / ((1/afr_gassoline)* (1/dens_gassoline));
    if(kmph < 1.0){
      kms = kmph * (1.0/3600.0);
      // lpkm = consum_l_s / (kms+0.1);
      lpkm = (float)(3600.0*maf)/(9069.90*kms);
    }else{
      lpkm = 0;
    }

    if(engine_on){
      consum_l_s = maf / 14.7;
      // liters per delta time
      float lt_dts = (consum_l_s / 1000.0) * (millis() - tm_t_consum);
      tm_t_consum = millis();
      lts_trip += lt_dts;
    }
    
    


    DEBUG_SERIAL.printf("%f RPM %f kmh %f kms %f maf %f l/km %f l/s %f lt trip \r", rpmn, kmph, kms, maf, lpkm, consum_l_s, lts_trip);
  }
  


}











/*

Natural gas: 17.2
Gasoline: 14.7
Propane: 15.5
Ethanol: 9
Methanol: 6.4
Hydrogen: 34
Diesel: 14.6


*/













void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}
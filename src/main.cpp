#define DEMO_MODE 1
#define DEBUG_SERIAL Serial

#include "WiFi.h"
#include "WebServer.h"
#include "AutoConnect.h"
#include "ESPTelnet.h"

ESPTelnet telnet;

WebServer Server;
WebServer websoc(3232);
AutoConnect  Portal(Server);
AutoConnectConfig Config;

AutoConnectAux Data("/data", "Data");



const char PAGE_MAIN[] PROGMEM = R"=====(
  <html>
    <style>
     .fanrpmslider {
      width: 30%;
      height: 55px;
      outline: none;
    }
     .bodytext {
      font-family: "Verdana", "Arial", sans-serif;
      font-size: 24px;
      text-align: left;
      font-weight: light;
      border-radius: 5px;
      display:inline;
    }
    </style>
    <body>
      <header>
      
      </header>
      <main>
          <div class="bodytext">Fan Speed Control (RPM: <span id="fanrpm"></span>)</div>
    <br>
          <input type="" class="fanrpmslider" min="0" max="255" value = "0" width = "0%" oninput="UpdateSlider(this.value)"/>
      </main>
      <footer>
      
      </footer>
    </body>
    <script>
    
    </script>
  </html>
  
  )=====";


#include <ESP_Google_Sheet_Client.h>

// Google Project ID
#define PROJECT_ID "lexical-pattern-446023-p9"

// Service Account's client email
#define CLIENT_EMAIL "esp32-car-logger@lexical-pattern-446023-p9.iam.gserviceaccount.com"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC3lsWNFAY9/mw4\n3oqEljiZ+JGMgEqmTPr0crfJBkCy8cOOxdHnyqx/ke5u22Ex3UhTJr60tti91C2E\nuPGah/YsNMNF29c+cUwuOS8c0Zzoyb7C16QrnrGg60GbLNz3EUIF38Osv+RFRwCi\nfSl1G/rXJnyrQRjwz2fpXtp2T8qQGC3+CJZ7/FBFBRgFel8GcV6f72x604Y+dzhl\nnhhJDNRyMaJCvRXgD8Rhcmdc8gphlKmccMQIMrkmYDxEXOq6o2AFlCT2UpXD70+T\nyJfzG4UBCdM73DxZggI9ILCaQBrP8cHb1dsyLauX02GMU2/LU3QHF8C/hhzFDiui\ns5givLmDAgMBAAECggEADXxmU52HyIWy0R9L1pSAYOhuV1Sp5yXFfi/FH1+1tfoQ\nzvaUxkL66+i+NpIn8B/yAFBazESYOaLnRcDI1/yqe9x+XFiGAGDAjgQGWXFuZiKX\nwDv9+R77AQ1MWvKZ4rj3psh6rmsZgMXynn0KOWo47BVi2A5RSjM6I12j4kgB4xa7\nwM4gsas/NHhcMIRC1+b1uF4EEV75xH7/jtWYYJto8KWeTA2d9aAm9yVZSU6M4/yq\nw5lwtegb8NxJnZeTSDvV09hU2OmPCHFI8pEYHxdC/azjGpcqz/qsuTBhdTilu0C6\n8wZFBwKfVdyeHaqZR6hU96JZXUlNSHdNHSeeZyQrhQKBgQDfZL2XQ8gUizd7kcKb\nIdLAlRgBcv7xTjEaYcBOPeWtbEf/lQnrvBaBCRsTL4Mf4qZ74ofVsAU2CoUswVe9\n97I7xILf3Vihu6+rmLp06W7ueL8aZzg1YjcRvpApRWIDkCQGzzl4JOfNs8H+j21n\n+BFmyQx+lacKa0bEdAHEPeB5dwKBgQDSYrRyHjHvigDEZyTtEckiXP7MdKvZpNsL\nMNGgjiNAFXj1Keggq8xHpTesvAaL/ZNkpvEPNOws4omBy1R3sDVxhMv/WAEXNVWJ\nj1XaWoIs7WwRBBhYy/IfxD8m712uZATPhr7TIf6wg7lBwVjpgooHN65ysE0iXn+O\nuKzMqngDVQKBgG5MKSfUeadbDWvfjfxWeN5JPu8IPkQaXTgvZ2m6OfqkafARQDK6\nTUosmsegP9ewao7kTDj/jbMfAp4UYCQVdlT4M74gZbCFILlS0U0ELJdl2sVIjYIe\nGh6Svk8CSOCFfKQ4EKH3ZtQzmub7HgOlgcIEJj0h2rXUPb6loHGqM3kFAoGBAMc7\nwCRd4e41Y1qbTwXOlQocxRL73g5jJyhm1+3TDSDP9Nc/E1t1NiQXGy1SdmS2A5IK\n4jgDZzFuasfmtRwPW90f5EvwnD/NOy7k9Wmt0p8XTAKlPSVCJg3dO+z0O2Q9xax9\nr1KkW9vvMk5J+phQDcSd/28O9Ez67pNK4iGDwdltAoGBALSmyAehM7oR+RdzXd/C\nHx4R9OqxVxuSo0G6vn2sL3n3u6UtIcXpwj28SRtrDpBZj1NLBJTMakGmNx4nbpwo\nWeokcT2o3beQ1QwgI4V5WyFI6VInZNKZTIGhx35v5go7hBAdcD8ZsMBHtekG3sET\nAJGf3FjRmWDQrjyQw71yugwA\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "1Igi_Eu8ytkM2I7W8L3MctP7x26rzCqba0UuEbO5jsLQ";


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

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}

void init_Gsheets(){
  // Set the callback for Google API access token generation status (for debug only)
    GSheet.setTokenCallback(tokenStatusCallback);

    // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
    GSheet.setPrerefreshSeconds(10 * 60);

    // Begin the access token generation for Google API authentication
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void append_gsheets(){

  init_Gsheets();

  FirebaseJson response;
  FirebaseJson valueRange;

  valueRange.add("majorDimension", "COLUMNS");
  valueRange.set("values/[0]/[0]", fuel_level);
  valueRange.set("values/[1]/[0]", rpmn);
  valueRange.set("values/[2]/[0]", kmph);
  valueRange.set("values/[3]/[0]", lpkm);

  // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
  // Append values to the spreadsheet
  bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
  if (success){
      response.toString(Serial, true);
      valueRange.clear();
  }
  else{
      DEBUG_SERIAL.println(GSheet.errorReason());
  }
}

void telnet_onconnect(String ip){

  DEBUG_SERIAL.println("hello!");
  DEBUG_SERIAL.println(esp_rom_get_reset_reason(0));
  DEBUG_SERIAL.println(esp_rom_get_reset_reason(1));
  DEBUG_SERIAL.println(ESP.getFreeHeap());

}

void setup()
{

  Serial.begin(115200);
  ELM_PORT.begin(38400, SERIAL_8N1, 17, 18, false, 2000);
  telnet.onConnect(telnet_onconnect);

  Server.on("/", [](){Server.send(200, "text/html", PAGE_MAIN);} );
  Server.on("/data", [](){DEBUG_SERIAL.println("starting gshets!"); append_gsheets(); Server.send(200, "text", "ok");} );

  Config.autoReset = false;
  Config.autoReconnect = true;
  Config.reconnectInterval = 6;
  Config.retainPortal = true;
  Config.ota = AC_OTA_BUILTIN;
  Portal.config(Config);
  Portal.join(Data);
  Portal.begin();

  init_elm();

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
  Portal.handleClient();
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
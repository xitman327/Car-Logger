#define DEMO_MODE 1
#define DEBUG_SERIAL Serial
#include "user.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include "AutoConnect.h"
#include "ESPTelnet.h"
#include "time.h"
#include "ArduinoJson.h"

ESPTelnet telnet;

WebServer Server;
AutoConnect       Portal(Server);
AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4

#define ENABLE_USER_CONFIG
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_FS
// #include "FirebaseFS.h"
#include <FirebaseClient.h>
#include <ExampleFunctions.h>

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
AsyncResult databaseResult;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds


// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String presPath = "/pressure";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;

const char* ntpServer = "pool.ntp.org";


#include "ELMduino.h"


#define ELM_PORT Serial1


ELM327 myELM327;
float engine_temp;
float consum_l_s /* L/s consumption*/, kms/*Km/s*/, lpkm/*L/km*/, gfps, lbsps, rpmn/*rpm*/, kmph, maf, fuel_level;
float lpkm_max/*L/km*/, rpmn_max/*rpm*/, kmph_max;
bool engine_on;



typedef enum { ENG_RPM,
               SPEED ,
               MAF,
               FUEL_LEVEL,
               ENG_TEMP
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


bool FS_STARTED = 0;
void setup()
{

  Serial.begin(115200);
  ELM_PORT.begin(38400, SERIAL_8N1, 17, 18, false, 2000);
  telnet.onConnect(telnet_onconnect);
  DEBUG_SERIAL.println("Starting");

  if(MY_FS.begin()){
    Serial.println("FS STARTED");
    FS_STARTED = 1;
  }else{
    if(MY_FS.format()){
      Serial.println("FS FORMAT");
      if(MY_FS.begin()){
        Serial.println("FS STARTED");
        FS_STARTED = 1;
      }else{
        FS_STARTED = 0;
        Serial.println("FS FAILLED");
      }
    }
    else{
      Serial.println("FS FORMAT FAILLED");
      FS_STARTED = 0;
    }
  }

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

  set_ssl_client_insecure_and_buffer(ssl_client);

  // ssl_client.setInsecure();
  // // ssl_client.setConnectionTimeout(1000);
  // ssl_client.setHandshakeTimeout(5);
  
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


float temperature;
float humidity;
float pressure;

long trip_distance_km = 0 ;
long trip_time_s = 0;

String time_and_distance;

JsonDocument trips[10];
JsonDocument single_trip_data;

uint32_t trip_locations_count = 0;

float current_consumption_l = 0;

bool log_started = 0;

FileConfig upload_data("/trip.bin", file_operation_callback);     // Can be set later with upload_data.setFile("/upload.bin", fileCallback);
char time_string[32];
void trip_start(){
  log_started = 1;
  single_trip_data.clear();
  trip_distance_km = 0;
  trip_locations_count = 0;
  single_trip_data["start_timestamp"] = getTime();
  time_t t = single_trip_data["start_timestamp"];
  struct tm *lt = localtime(&t);
  strftime(time_string, sizeof(time_string), "%Y.%m.%d:%H.%M.%S", lt);
  single_trip_data["start_timestamp_string"] = time_string;
}

void populate_current_json(){


  kmph_max = max(kmph_max, kmph);
  single_trip_data["top_speed"] = kmph_max;

  lpkm_max = max(lpkm_max, lpkm);
  single_trip_data["max_consumption"] = lpkm_max;

  single_trip_data["trip_distance"] = trip_distance_km;

  single_trip_data["trip_locations_count"] = ++trip_locations_count;
  single_trip_data["trip_locations"][trip_locations_count]["time"] = getTime();
  time_t t = single_trip_data["trip_locations"][trip_locations_count]["time"];
  struct tm *lt = localtime(&t);
  strftime(time_string, sizeof(time_string), "%Y.%m.%d:%H.%M.%S", lt);
  single_trip_data["trip_locations"][trip_locations_count]["time_string"] = time_string;
  single_trip_data["trip_locations"][trip_locations_count]["log"] = (float) 0;
  single_trip_data["trip_locations"][trip_locations_count]["lat"] = (float) 0;
  single_trip_data["trip_locations"][trip_locations_count]["dist_traveled"] = trip_distance_km;
  single_trip_data["trip_locations"][trip_locations_count]["speed"] = kmph;
  single_trip_data["trip_locations"][trip_locations_count]["rpm"] = rpmn;
  single_trip_data["trip_locations"][trip_locations_count]["engine_temp"] = engine_temp;
  single_trip_data["trip_locations"][trip_locations_count]["consumption"]["lpkm"] = lpkm;
  single_trip_data["trip_locations"][trip_locations_count]["consumption"]["lps"] = current_consumption_l;
  

  Serial.printf("single_trip_data size %d b \n\n", sizeof(single_trip_data));
}

void trip_end(){
  log_started = 0;
  single_trip_data["end_timestamp"] = getTime();
  time_t t = single_trip_data["end_timestamp"];
  struct tm *lt = localtime(&t);
  strftime(time_string, sizeof(time_string), "%Y.%m.%d:%H.%M.%S", lt);
  single_trip_data["end_timestamp_string"] = time_string;
  single_trip_data["trip duration"] =(long) single_trip_data["end_timestamp"].as<long>() - single_trip_data["start_timestamp"].as<long>();
  if(FS_STARTED){
    File file = MY_FS.open("/trip.bin", "w", true);
    if(file){
      serializeJsonPretty(single_trip_data, Serial);
      Serial.println();
      Serial.printf("%d data serialized \n", (uint32_t)serializeJson(single_trip_data, file));
      file.close();
    }else{
      Serial.println("file error");
      file.close();
    }
  }else{
    Serial.println("FS not started");
  }

  if(WiFi.isConnected() && app.ready()){

    uid = app.getUid().c_str();
    // Update database path
    timestamp = getTime();
    databasePath = "/UsersData/" + uid + "/readings";
    trip_distance_km = single_trip_data["trip_distance"];
    trip_time_s = single_trip_data["trip duration"];
    time_t t = timestamp;
    struct tm *lt = localtime(&t);
    strftime(time_string, sizeof(time_string), "%Y_%m_%d__%H_%M_%S", lt);
    parentPath= databasePath + "/" + String(trip_time_s) + "_s__" + String(trip_distance_km) + "_km__" + String(time_string);
    // String trip_data_string;
    // serializeJson(single_trip_data, trip_data_string);
    Database.set<object_t>(aClient, parentPath, object_t(single_trip_data.as<String>()), processData, "RTDB_Send_Data");
    // bool status = Database.set(aClient, parentPath, getFile(upload_data));
    // Database.set(aClient, "/examples/File/data1", getFile(upload_data), processData, "⬆️  uploadTask");

  }else{
    if(!app.ready()){
      Serial.println("app not ready");
    }
    if(!WiFi.isConnected()){
      Serial.println("WiFi not connected");
    }
  }
}

#define log_time 1000
uint32_t tm_log;

void loop()
{
  app.loop();
  processData(databaseResult);
  // if(app.ready()){
  //   unsigned long currentTime = millis();
  //   if (currentTime - lastSendTime >= sendInterval){
  //     lastSendTime = currentTime;
      
  //     uid = app.getUid().c_str();

  //     // Update database path
  //     databasePath = "/UsersData/" + uid + "/readings";

  //     //Get current timestamp
  //     timestamp = getTime();
  //     Serial.print ("time: ");
  //     Serial.println (timestamp);

  //     parentPath= databasePath + "/" + String(trip_time_s) + "_s__" + String(trip_distance_km) + "_km__" + String(timestamp);

  //     // Get sensor readings
  //     temperature = 12;
  //     humidity = 14;
  //     pressure = 15;

  //     // Create a JSON object with the data
  //     writer.create(obj1, tempPath, temperature);
  //     writer.create(obj2, humPath, humidity);
  //     writer.create(obj3, presPath, pressure);
  //     writer.create(obj4, timePath, timestamp);
  //     writer.join(jsonData, 4, obj1, obj2, obj3, obj4);

  //     Database.set<object_t>(aClient, parentPath, jsonData, processData, "RTDB_Send_Data");
    
  //   }
  // }






  telnet.loop();

  if(millis() - tm_obdpull > obd_pull_time){
    tm_obdpull = millis();

    if((myELM327.nb_rx_state == ELM_UNABLE_TO_CONNECT || myELM327.nb_rx_state == ELM_NO_DATA) && !DEMO_MODE ){
      init_elm();
    }

    if(DEMO_MODE){
      while(Serial.available()){
        char kjl = Serial.read();
        if(kjl == 'K'){
          rpmn = 2400;
          kmph = 140;
          maf = 5.7;
        }else if (kjl == 'L'){
          rpmn = 0;
          kmph = 0;
          maf = 0;
        }
      }
      // rpmn = 2400;
      // kmph = 123;
      // maf = 1.7;
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
            obd_state = ENG_TEMP;
          }
          else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
          {
            myELM327.printError();
            obd_state = ENG_TEMP;
          }
          
          break;
        }
        case ENG_TEMP:
        {
          float resp = myELM327.engineCoolantTemp();
          
          if (myELM327.nb_rx_state == ELM_SUCCESS)
          {
            engine_temp = resp;
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
    if(kmph < (float)0.5){
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
      current_consumption_l = lt_dts;
      lts_trip += lt_dts;
      float moving_speed_attime = kmph * 0.0002777778;
      trip_distance_km += moving_speed_attime;

    }
    
    


    // DEBUG_SERIAL.printf("%f RPM %f kmh %f kms %f maf %f l/km %f l/s %f lt trip \r\n", rpmn, kmph, kms, maf, lpkm, consum_l_s, lts_trip);
  }

  if(millis() - tm_log > log_time){
    tm_log = millis();

    if(engine_on && !log_started){
      trip_start();
    }else if(!engine_on && log_started){
      trip_end();
    }
    if(log_started){
      populate_current_json();
    }

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













// void processData(AsyncResult &aResult) {
//   if (!aResult.isResult())
//     return;

//   if (aResult.isEvent())
//     Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

//   if (aResult.isDebug())
//     Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

//   if (aResult.isError())
//     Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

//   if (aResult.available())
//     Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
// }

void processData(AsyncResult &aResult)
{
    // Exits when no result available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.downloadProgress())
    {
        Firebase.printf("Download task: %s, downloaded %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.downloadInfo().progress, "%", aResult.downloadInfo().downloaded, aResult.downloadInfo().total);
        if (aResult.downloadInfo().total == aResult.downloadInfo().downloaded)
        {
            Firebase.printf("Download task: %s, complete!✅️\n", aResult.uid().c_str());
            print_file_content("/download.bin");
        }
    }

    if (aResult.uploadProgress())
    {
        Firebase.printf("Upload task: %s, uploaded %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.uploadInfo().progress, "%", aResult.uploadInfo().uploaded, aResult.uploadInfo().total);
        if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded)
            Firebase.printf("Upload task: %s, complete!✅️\n", aResult.uid().c_str());

        if (aResult.name().length())
            Firebase.printf("Name: %s\n", aResult.name().c_str());
    }
}
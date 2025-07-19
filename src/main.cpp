#define DEMO_MODE 1
#define DEBUG_SERIAL Serial
#include "SPIFFS.h"
#include "user.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <WebServer.h>
// #include "AutoConnect.h"
// #include "time.h"
#include "ArduinoJson.h"

#include <LovyanGFX.hpp>
#include "LGFX_user.h"
LGFX_ILI9488 lcd;


bool FS_STARTED = 0;
// WebServer Server;
// AutoConnect       Portal(Server);
// AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4

#define ENABLE_USER_CONFIG
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_FS
// #include "FirebaseFS.h"
#include <FirebaseClient.h>
#include <ExampleFunctions.h>

bool upload_request =0;

float fix_lat, fix_lng;
bool fix_valid;

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

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;

const char* ntpServer = "pool.ntp.org";

void upload_trip();

#include "ELMduino.h"


#define ELM_PORT Serial1

// #include <TinyGPS++.h>
// #define GPS_SERIAL Serial2  // Using Serial1 for GPS
// TinyGPSPlus gps;

#include <NMEAGPS.h>
// #include <GPSport.h>
NMEAGPS  gps; // This parses the GPS characters
gps_fix  fix; // This holds on to the latest values

#include <ESP32Time.h>
ESP32Time rtc;


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


unsigned long getTime() {
  if(WiFi.isConnected()){
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      //Serial.println("Failed to obtain time");
      return(0);
    }
    time(&now);
    return now;
  }
  return(0);
}

long trip_distance_km = 0 ;
long trip_time_s = 0;

String time_and_distance;

JsonDocument single_trip_data;

uint32_t trip_locations_count = 0;

float current_consumption_l = 0;

bool log_started = 0;

// FileConfig upload_data("/trip.bin", file_operation_callback);     // Can be set later with upload_data.setFile("/upload.bin", fileCallback);
char time_string[32];
void trip_start(){
  log_started = 1;
  single_trip_data.clear();
  current_consumption_l = 0;
  lpkm = 0;
  trip_distance_km = 0;
  trip_locations_count = 0;
  single_trip_data["start_timestamp"] =  rtc.getEpoch();
  // time_t t = single_trip_data["start_timestamp"];
  // struct tm *lt = localtime(&t);
  // strftime(time_string, sizeof(time_string), "%Y.%m.%d:%H.%M.%S", lt);
  // sprintf(time_string, "%d.%d.%d.%d.%d.%d", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHour(), rtc.getMinute(), rtc.getSecond());
  single_trip_data["start_timestamp_string"] = rtc.getTime("%Y_%m_%d__%H_%M_%S");
  Serial.printf("Trip started %s", time_string);
}

void populate_current_json(){


  kmph_max = max(kmph_max, kmph);
  single_trip_data["top_speed"] = kmph_max;

  lpkm_max = max(lpkm_max, lpkm);
  single_trip_data["max_consumption"] = lpkm_max;

  single_trip_data["trip_distance"] = trip_distance_km;

  single_trip_data["trip_locations_count"] = ++trip_locations_count;
  single_trip_data["trip_locations"][trip_locations_count]["time"] = rtc.getEpoch();
  // time_t t = single_trip_data["trip_locations"][trip_locations_count]["time"];
  // struct tm *lt = localtime(&t);
  // strftime(time_string, sizeof(time_string), "%Y.%m.%d:%H.%M.%S", lt);
  // sprintf(time_string, "%d.%d.%d.%d.%d.%d", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHour(), rtc.getMinute(), rtc.getSecond());
  single_trip_data["trip_locations"][trip_locations_count]["time_string"] = rtc.getTime("%Y_%m_%d__%H_%M_%S");
  single_trip_data["trip_locations"][trip_locations_count]["lng"] = fix_lng;
  single_trip_data["trip_locations"][trip_locations_count]["lat"] = fix_lat;
  single_trip_data["trip_locations"][trip_locations_count]["dist_traveled"] = trip_distance_km;
  single_trip_data["trip_locations"][trip_locations_count]["speed"] = kmph;
  single_trip_data["trip_locations"][trip_locations_count]["rpm"] = rpmn;
  single_trip_data["trip_locations"][trip_locations_count]["engine_temp"] = engine_temp;
  single_trip_data["trip_locations"][trip_locations_count]["consumption"]["lpkm"] = lpkm;
  single_trip_data["trip_locations"][trip_locations_count]["consumption"]["lps"] = current_consumption_l;
  

  // Serial.printf("single_trip_data size %d b\n", sizeof(single_trip_data));
  
}

void trip_end(){
  //end trip and complete json
  log_started = 0;
  single_trip_data["end_timestamp"] = rtc.getEpoch();
  // time_t t = single_trip_data["end_timestamp"];
  // struct tm *lt = localtime(&t);
  // strftime(time_string, sizeof(time_string), "%Y.%m.%d:%H.%M.%S", lt);
  // sprintf(time_string, "%d.%d.%d.%d.%d.%d", rtc.getYear(), rtc.getMonth(), rtc.getDay(), rtc.getHour(), rtc.getMinute(), rtc.getSecond());
  single_trip_data["end_timestamp_string"] = rtc.getTime("%Y_%m_%d__%H_%M_%S");
  single_trip_data["trip duration"] =(long) single_trip_data["end_timestamp"].as<long>() - single_trip_data["start_timestamp"].as<long>();
  Serial.printf("Trip ended %s", single_trip_data["end_timestamp_string"]);
  //save json to file with aproperate filename
  if(FS_STARTED){
    // time_t t = single_trip_data["start_timestamp"].as<long>();
    // struct tm *lt = localtime(&t);
    // strftime(time_string, sizeof(time_string), "%Y_%m_%d__%H_%M_%S", lt);
    String filename = "/" + single_trip_data["start_timestamp_string"].as<String>() + ".json";
    File file = SPIFFS.open(filename, "w", true);
    if(file){
      // serializeJsonPretty(single_trip_data, Serial);
      Serial.println();
      Serial.printf("%s %d data serialized \n", file.name(), (uint32_t)serializeJson(single_trip_data, file));
      // file.close();
    }else{
      Serial.println("file error");
      // file.close();
    }
    file.close();
  }else{
    Serial.println("FS not started");
  }
}

int num_of_files = 0;
String dir_filenames_array[11];
#define max_filenames 10

int get_filenames(){

  num_of_files = 0;
  Serial.println("listing files in /");

  File root = SPIFFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
      if(num_of_files > 10)break;
      Serial.print("FILE: ");
      Serial.println(file.name());
      dir_filenames_array[num_of_files] = file.name();
      num_of_files++;
      file = root.openNextFile();
  }

  return num_of_files;
}


void delete_all_trips(){
  for(int i = get_filenames();i>0;i--){
    MY_FS.remove("/"+dir_filenames_array[max(i - 1, 0)]);
  }
}

bool result_error =0, result_done=0, upload_done = 0;
void upload_trip(){
  Serial.println("uploading data");
  if(WiFi.isConnected() && app.ready()){
    for(int i = get_filenames();i>0;i){
      String temp_string_filename = dir_filenames_array[max(i - 1, 0)];
      temp_string_filename.remove(temp_string_filename.length() - 5);
      File upload_trip = SPIFFS.open("/"+temp_string_filename+".json", "r");
      if(upload_trip){

        Serial.printf("uploading file %s \n", upload_trip.name());

        uid = app.getUid().c_str();
        databasePath = "/UsersData/" + uid + "/trips/";
        parentPath = databasePath + temp_string_filename;
        String trip_data;
        while(upload_trip.available()){
          trip_data += char(upload_trip.read());
        }
        // Serial.println(trip_data);

        Database.set<object_t>(aClient, parentPath, object_t(trip_data), processData, "TRIP_DB_Send_Data");

        // result_error = Database.set<object_t>(aClient, parentPath, object_t(trip_data));
        // Firebase.printf("Error, msg: %s, code: %d\n", aClient.lastError().message().c_str(), aClient.lastError().code());
        // if(!result_error){
        //   Serial.println("Upload success");
        // }else{
        //   Serial.println("Upload failled");
        //   return;
        // }
        // result_error = 0;

      }else{
        Serial.printf("file %s not found", temp_string_filename);
      }
      upload_trip.close();
      if(upload_done){
        upload_done = 0;
        if(i>0){
          i--;
        }
      }
    }
    Serial.println("done uploading data");
    // delete_all_trips();
    Serial.println("deleting old data");
  }else{
    Serial.println("error uploading data");
    if(!app.ready()){
      Serial.println("app not ready");
    }
    if(!WiFi.isConnected()){
      Serial.println("WiFi not connected");
    }
  }
}

#define wifi_timeout 120000
uint32_t time_waiting_for_connect = 0;

int upload_stage=0;

#define wifi_retries 50
int wifi_retry_count=0;

void task_upload_data(){

  switch (upload_stage){

    case 0:
      if(WiFi.isConnected()){
        Serial.println("wifi conneced");
        
        wifi_retry_count = 0;
        upload_stage = 2;
      }else{
        WiFi.mode(WIFI_MODE_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        WiFi.setAutoReconnect(true);
        Serial.printf("Connect to %s %s \n", WIFI_SSID, WIFI_PASS);
        time_waiting_for_connect = millis();
        wifi_retry_count = 0;
        upload_stage = 1;
      }
    break;

    case 1:// connect to wifi
      if(WiFi.isConnected()){
        upload_stage = 2;
      }
      if(!WiFi.isConnected() && (wifi_retry_count > wifi_retries || millis() - time_waiting_for_connect > wifi_timeout)){
        Serial.println("Connection Timed Out");
        WiFi.setAutoReconnect(false);
        upload_stage = 0;
        upload_request = 0;
      }
      
    break;

    case 2: // app auth
      // initializeApp(aClient, app, getAuth(user_auth), 3000, auth_debug_print);
      // app.autoAuthenticate(false);
      // app.getApp<RealtimeDatabase>(Database);
      // Database.url(DATABASE_URL);
      if(app.isAuthenticated() && !app.isExpired() && app.isInitialized()){
        upload_stage = 3;
      }else {
        // initializeApp(aClient, app, getAuth(user_auth), 3000, auth_debug_print);
        if(!app.isAuthenticated()){
          app.authenticate();
        }
      }
    break;

    case 3:
      // wait app to authenticate
      if(app.isAuthenticated()){
        app.getApp<RealtimeDatabase>(Database);
        Database.url(DATABASE_URL);
        upload_stage = 4;
      }
    break;

    case 4:
      //upload data
      upload_trip();
      upload_stage = 5;
    break;

    case 5:

      if(result_done ==0){

      }

      upload_stage = 0;
      Serial.println("Exiting");
      // vTaskDelete(NULL);
      upload_request = 0;
    break;
  
  default:
    break;
  }

  // if(connect_wifi()){
  //   return;
  // }

  // if(!app.isAuthenticated()){
  //   app.authenticate();
  // }
  

  // while(!app.isAuthenticated()){
  //   app.loop();
  //   yield();
  // }

  // if(!app.isAuthenticated() || !app.isInitialized()){
  //   Serial.printf("%s %s \nExiting\n", app.isAuthenticated()?"isAuthenticated":"isNotAuthenticated", app.isInitialized()?"isInitialized":"isNotInitialized");
  //   // vTaskDelete(NULL);
  //   return;
  // }


  // if(app.ready()){
  //   upload_trip();
  // }


  // Serial.println("Exiting");
  // vTaskDelete(NULL);
}

#define NEOPIXEL_PIN 12
#define NUMPIXELS 15 
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void runRgbWave(Adafruit_NeoPixel &strip, int numPixels, uint32_t baseColor, int waveWidth, float speed) {
  static float wavePosition = 0;
  
  // Clear the strip
  for(int i = 0; i < numPixels; i++) {
    strip.setPixelColor(i, 0);
  }
  
  // Draw the wave
  for(int i = 0; i < numPixels; i++) {
    float distance = fabs(i - wavePosition);
    
    if(distance < waveWidth) {
      // Calculate brightness (1.0 at center, 0.0 at edges)
      float brightness = 1.0 - (distance / waveWidth);
      
      // Extract color components
      uint8_t r = (baseColor >> 16) & 0xFF;
      uint8_t g = (baseColor >> 8) & 0xFF;
      uint8_t b = baseColor & 0xFF;
      
      // Apply brightness
      r *= brightness;
      g *= brightness;
      b *= brightness;
      
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
  }
  
  strip.show();
  
  // Move wave position
  wavePosition += speed;
  if(wavePosition > numPixels + waveWidth) {
    wavePosition = -waveWidth;
  }
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi connected! IP address: ");
      Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
      // Your callback code here
      break;
      
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      wifi_retry_count++;
      break;

    default:
      break;
  }
}

// #include <SPI.h>
// #include <TFT_eSPI.h>
// TFT_eSPI tft = TFT_eSPI();



#define LCD_BL 4
#define light_sensor 13

HardwareSerial ELMSerial(1);

HardwareSerial gpsSerial(2);

void init_elm(){
  DEBUG_SERIAL.println("Attempting to connect to ELM327...");

  if (!myELM327.begin(ELMSerial, false, 2000))
  {
    DEBUG_SERIAL.println("Couldn't connect to OBD scanner");
    lcd.setCursor(50, 300);
    lcd.print("ELM Connect Fault");
    // while (1);
  }else{
    DEBUG_SERIAL.println("Connected to ELM327");
    lcd.setCursor(50, 300);
    lcd.print("ELM Connect OK");
  }

  
}

void setup()
{
  pixels.begin();
  pixels.clear();
  // for(int i=0;i<NUMPIXELS; i++){
  //   pixels.setPixelColor(i, 255,255,255);
  // }
  pixels.setBrightness(10);
  pixels.show();

  analogReadResolution(12);
  pinMode(light_sensor, ANALOG);
  pinMode(LCD_BL, OUTPUT);
  
  lcd.init();
  lcd.clearDisplay();
  lcd.setRotation(3);  // Adjust rotation (0-3)
  lcd.fillScreen(TFT_BLACK);
  digitalWrite(LCD_BL, HIGH);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);
  lcd.println("Hello, Xitos!");

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  esp_log_level_set("*",ESP_LOG_INFO);
  Serial.begin(115200);
  ELMSerial.begin(38400, SERIAL_8N1, 48, 47, false, 2000);
  gpsSerial.begin(9600, SERIAL_8N1, 38, 37);

  DEBUG_SERIAL.println("Starting");

  if(SPIFFS.begin(true)){
    Serial.println("FS STARTED");
    FS_STARTED = 1;
  }else{
    if(SPIFFS.format()){
      Serial.println("FS FORMAT");
      if(SPIFFS.begin()){
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
  

  if(!DEMO_MODE){
    init_elm();
  }

  // configTime(0, 0, ntpServer);
  set_ssl_client_insecure_and_buffer(ssl_client);
  initializeApp(aClient, app, getAuth(user_auth), 3000, auth_debug_print);
  app.autoAuthenticate(false);
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);


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

static uint8_t hue = 0;

#define log_time 1000
#define led_time 20
uint32_t tm_log, tm_led;
char tmp0[100];
bool fix_valid_time_prev =0, fix_valid_date_prev=0;
void loop()
{
  // Portal.handleClient();

  if(WiFi.isConnected()){
    app.loop();
  }
  

  if(millis() - tm_led > led_time){
    tm_led = millis();
    uint32_t color = pixels.ColorHSV(hue * 256, 255, 255);
    runRgbWave(pixels, NUMPIXELS, color, 10, 0.5);
    hue++; // Cycle through colors
  }
  

  while (gps.available( gpsSerial )) {
    fix = gps.read();

    // if(fix.valid.time != fix_valid_time_prev && fix.valid.date != fix_valid_date_prev){
    //   fix_valid_time_prev = fix.valid.time;
    //   fix_valid_date_prev = fix.valid.date;
    //   if(fix.valid.time && fix.valid.date){
    //     int year = fix.dateTime.year;
    //     if (year < 100) year += 2000; // Convert 2-digit to 4-digit (e.g., 23 → 2023)

    //     // Set RTC (UTC time)
    //     rtc.setTime(
    //       fix.dateTime.seconds,  // sec
    //       fix.dateTime.minutes,  // min
    //       fix.dateTime.hours,    // hour
    //       fix.dateTime.date,     // day
    //       fix.dateTime.month,    // month
    //       year                   // year (now 4-digit)
    //     );
    //     // rtc.setTime(fix.dateTime.seconds, fix.dateTime.minutes, fix.dateTime.hours, fix.dateTime.day, fix.dateTime.month, fix.dateTime.year);
    //     int timezone_offset = 2; // Hours ahead of UTC
    //     rtc.setTime(rtc.getEpoch() + (timezone_offset * 3600)); // Add seconds
    //   }
    // }

    // if(fix.valid.time){
    //   lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // }else{
    //   lcd.setTextColor(TFT_RED, TFT_BLACK);
    // }
    // lcd.setCursor(5,15);
    // lcd.setTextSize(1);
    
    // sprintf(tmp0, "%2d : %2d : %2d", fix.dateTime.hours, fix.dateTime.minutes, fix.dateTime.seconds);
    // lcd.print(tmp0);
    // lcd.print("   ");
    // lcd.print(rtc.getTime("%Y-%m-%d %H:%M:%S"));

    // if(fix.valid.satellites){
    //   lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // }else{
    //   lcd.setTextColor(TFT_RED, TFT_BLACK);
    // }
    // lcd.setCursor(5,30);
    // sprintf(tmp0, "sats: %2d", fix.satellites);
    // lcd.print(tmp0);

    // fix_valid = fix.valid.location;
    // if(fix.valid.location){
    //   lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // }else{
    //   lcd.setTextColor(TFT_RED, TFT_BLACK);
    // }
    // fix_lat = fix.location.latF();
    // fix_lng = fix.location.lonF();
    // lcd.setCursor(5,45);
    // sprintf(tmp0, "location speed: %f lat: %f lng: %f",fix.speed_kph(), fix.location.latF(), fix.location.lonF());
    // lcd.print(tmp0);

  }




  if(upload_request){
    task_upload_data();
  }

  if(millis() - tm_obdpull > obd_pull_time){
    tm_obdpull = millis();

    if((myELM327.nb_rx_state == ELM_UNABLE_TO_CONNECT || myELM327.nb_rx_state == ELM_NO_DATA) && !DEMO_MODE ){
      init_elm();
    }

    if(DEMO_MODE){
      while(Serial.available()){
        char kjl = Serial.read();
        // Serial.println(kjl);
        if(kjl == 'K'){
          rpmn = 2400;
          kmph = 140;
          maf = 5.7;
        }else if (kjl == 'L'){
          rpmn = 0;
          kmph = 0;
          maf = 0;
        }else if(kjl == 'J'){
          upload_trip();
        }else if(kjl == 'D'){
          delete_all_trips();
        }else if(kjl == 'F'){
          get_filenames();
        }else if(kjl == 'H'){
          upload_request = 1;
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
    
    
    // lcd.setCursor(5, 100);
    // lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // sprintf(tmp0, "%4d RPM %3d KMh Engine %s %s", (uint16_t)rpmn, (uint16_t)kmph, engine_on?" on":"off", log_started?"Logging    ":"Not logging");
    // lcd.print(tmp0);

    // DEBUG_SERIAL.printf("%f RPM %f kmh %f kms %f maf %f l/km %f l/s %f lt trip \r\n", rpmn, kmph, kms, maf, lpkm, consum_l_s, lts_trip);
  }

  if(millis() - tm_log > log_time){
    tm_log = millis();

    // lcd.setCursor(5, 200);
    // lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    // lcd.print(millis());


    if(engine_on && !log_started){
      trip_start();
      // delay(2000);
      // task_upload_data(NULL);
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






void processData(AsyncResult &aResult) {

  if(aResult.uploadProgress()){
    int get_getprogress = aResult.uploadInfo().progress;
    Serial.println(get_getprogress);
    if(get_getprogress > 98){
      upload_done = 1;
      Serial.println("Upload done");
    }
  }

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    result_error = 1;
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available())
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
}
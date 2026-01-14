#define DEFAULT_DEMO_MODE 0
#define DEBUG_SERIAL Serial
#include "constants.h"
#include "SPIFFS.h"
#include "user.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include "ArduinoJson.h"
#include <time.h>

bool FS_STARTED = 0;


bool demo_mode = DEFAULT_DEMO_MODE;
bool upload_request = 0;
bool firebase_initialized = false;

float fix_lat, fix_lng;
bool gps_location_valid = false;
uint8_t last_sat_count = 0;
time_t last_gps_fix_time = 0;

const char* ntpServer = "pool.ntp.org";


#include <ESP32Time.h>
ESP32Time rtc;
bool gps_time_synced = false;
bool wifi_time_synced = false;
bool ntp_requested = false;
uint32_t last_ntp_attempt_ms = 0;
const uint32_t ntp_retry_interval_ms = 60000;
const long timezone_offset_seconds = 0;

const uint32_t wifi_connect_timeout = 30000;
const uint32_t wifi_retry_interval = 5000;
const uint8_t wifi_max_attempts = 5;
uint32_t wifi_connect_started_at = 0;
uint32_t last_wifi_attempt_ms = 0;
uint8_t wifi_attempt_count = 0;
bool time_corrected_this_trip = false;

enum UploadStage : uint8_t { UploadIdle, UploadConnectWiFi, UploadAuth, UploadSend, UploadComplete };
UploadStage upload_stage = UploadIdle;
const uint8_t db_max_retries = 3;
uint8_t db_retry_count = 0;

int current_upload_file_index = -1;
int active_upload_file_index = -1;
bool upload_in_progress = false;
bool upload_error = false;
bool upload_done_flag = false;
uint32_t upload_started_at = 0;
const uint32_t upload_timeout_ms = 30000;

bool lpg_eligible = false;
bool lpg_likely = false;
uint32_t engine_start_ms = 0;
const float lpg_min_coolant_c = 40.0f;
const uint32_t lpg_min_run_ms = 20000;
const float lpg_rpm_threshold = 3000.0f;

float trip_distance_km = 0;

JsonDocument single_trip_data;

uint32_t trip_locations_count = 0;
uint32_t last_distance_update_ms = 0;
float gps_speed_kmph = 0;
bool gps_speed_valid = false;

float current_consumption_l = 0;


float lts_trip = 0;

bool log_started = 0;
bool debug_dashboard_enabled = false;

int num_of_files = 0;
String dir_filenames_array[11];
#define max_filenames 10

extern int num_of_files;


 enum{
    ADAPT_VOLTAGE,
    OBD_VOLTAGE,
    ENG_RPM,
    VEHECLE_SPEED,
    GPS_SPEED,
    GPS_POS_ALTERED,
    DEBUG_FORCED
  }trip_start_condition_enum;

  byte trip_start_condition = ENG_RPM;
  byte temp_trip_start_condition = trip_start_condition;
  #define CHARGING_VOLTAGE (float)13.5

  float charging_voltage = CHARGING_VOLTAGE;
bool log_need_restart = 0;
bool debug_log_start = 0;

#include "sdcard.h"
#include "sensors.h"
#include "gps.h"
#include "obd.h"
#include "settings.h"
#include "ble.h"
#include "trip.h"
#include "files.h"
#include "lcd.h"
#include "debug.h"
#include "extra_functions.h"




void setup()
{

  load_settings();
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(10);
  pixels.show();

  analogReadResolution(12);
  pinMode(light_sensor, ANALOG);

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setAutoReconnect(false);
  esp_log_level_set("*",ESP_LOG_INFO);
  Serial.begin(115200);
  
  setup_gps();

  log_i("\e[0;32m Starting");

  setup_sd();
  setup_lcd();
  sensors_setup();

  if(SPIFFS.begin(true)){
    log_i("FS STARTED");
    FS_STARTED = 1;
  }else{
    if(SPIFFS.format()){
      log_i("FS FORMAT");
      if(SPIFFS.begin()){
        log_i("FS STARTED");
        FS_STARTED = 1;
      }else{
        FS_STARTED = 0;
        log_e("FS FAILLED");
      }
    }
    else{
      log_e("FS FORMAT FAILLED");
      FS_STARTED = 0;
    }
  }
  log_d("Starting BLE");
  ble_setup();
  log_d("BLE OK");
  log_d("Starting ELM");
  setup_elm();
  log_d("ELM OK");


}

#define afr_gassoline 14.7
#define dens_gassoline 710.0


#define obd_pull_time 200

static uint8_t hue = 0;

#define log_time 2000 //1000 or 2000

#define led_time 20
#define debug_report_time 2000
uint32_t tm_log, tm_led;
uint32_t tm_debug_report = 0;
void loop()
{
  if(WiFi.isConnected()){
    // ensureFirebaseReady();
    // app.loop();
    if(!gps_time_synced || !wifi_time_synced){
      sync_time_from_wifi();
    }
  }

  if(millis() - tm_led > led_time){
    tm_led = millis();
    uint32_t color = pixels.ColorHSV(hue * 256, 255, 255);
    runRgbWave(pixels, NUMPIXELS, color, 10, 0.5);
    hue++;
  }

  while (Serial.available()) {
    char key = Serial.read();
    if (key == 'T') {
      demo_mode = !demo_mode;
      Serial.printf("DBG: Demo mode %s\n", demo_mode ? "ENABLED" : "DISABLED");
      continue;
    }
    if (demo_mode) {
      handleDebugCommand(key);
    }
  }

  

  if(upload_request){
    task_upload_data();
  }


  loop_lcd();
  loop_elm();
  ble_loop();
  sensors_loop();
  loop_gps();


 

  if(millis() - tm_log > log_time){
    tm_log = millis();

    // restart trip if pids change
    if(log_need_restart && log_started){
      log_i("Trip Restarting!");
      trip_end();
      trip_start();
      log_need_restart = 0;
    }else if(log_need_restart && !log_started){
      log_i("No Trip Started!");
      log_need_restart = 0;
    }

    switch(trip_start_condition){
      case ADAPT_VOLTAGE:
        if(battery_voltage > charging_voltage && !log_started){trip_start();}else
        if(battery_voltage < charging_voltage && log_started){trip_end();}
      break;
      case OBD_VOLTAGE:
        if(battery_voltage > charging_voltage && !log_started){trip_start();}else
        if(battery_voltage < charging_voltage && log_started){trip_end();}
      break;
      case ENG_RPM:
        if(rpmn > 350 && !log_started){trip_start();}else
        if(rpmn < 350 && log_started){trip_end();}
      break;
      case VEHECLE_SPEED:
        if(kmph > 5.0 && !log_started){trip_start();}else
        if(kmph < 5.0 && log_started){trip_end();}
      break;
      case GPS_SPEED:
        if(fix.speed_kph() > 5.0 && !log_started){trip_start();}else
        if(fix.speed_kph() < 5.0 && log_started){trip_end();}
      break;
      case GPS_POS_ALTERED:
         if (detector.updateLeaveBase() && !log_started) {
            trip_start();
          }
          if (detector.updateStationaryAndMaybeSetBase() && log_started) {
            trip_end();
          }
      break;
      case DEBUG_FORCED:
          if(debug_log_start && !log_started){trip_start();}else
          if(!debug_log_start && log_started){trip_end();}
      break;
      default:
      break;
      
    }

    if(log_started){
      populate_current_json();
    }


  }
  
  if(debug_dashboard_enabled && millis() - tm_debug_report > debug_report_time){
    tm_debug_report = millis();
    printDebugDashboard();
  }
}


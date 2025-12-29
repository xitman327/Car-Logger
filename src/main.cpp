#define DEFAULT_DEMO_MODE 0
#define DEBUG_SERIAL Serial
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


#include <NMEAGPS.h>
NMEAGPS  gps;
gps_fix  fix;

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

uint32_t tm_convertion = 0, tm_obdpull = 0, tm_t_consum = 0;
float lts_trip = 0;

bool log_started = 0;
bool debug_dashboard_enabled = false;

int num_of_files = 0;
String dir_filenames_array[11];
#define max_filenames 10

HardwareSerial gpsSerial(2);


extern int num_of_files;



#include "obd.h"
#include "ble.h"
#include "trip.h"
#include "files.h"
#include "lcd.h"
#include "debug.h"
#include "extra_functions.h"



void setup()
{
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(10);
  pixels.show();

  analogReadResolution(12);
  pinMode(light_sensor, ANALOG);

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  esp_log_level_set("*",ESP_LOG_INFO);
  Serial.begin(115200);
  
  gpsSerial.begin(9600, SERIAL_8N1, 38, 37);

  DEBUG_SERIAL.println("Starting");

  setup_lcd();

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
  
  ble_setup();
  setup_elm();
  // ensureFirebaseReady();


}

#define afr_gassoline 14.7
#define dens_gassoline 710.0

#define convertion_time 1000
#define obd_pull_time 200

float temperature;
float humidity;
float pressure;

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

  while (gps.available( gpsSerial )) {
    fix = gps.read();
    sync_rtc_from_gps_fix(fix);
    if(fix.valid.location){
      fix_lat = fix.latitude();
      fix_lng = fix.longitude();
      gps_location_valid = true;
    } else {
      gps_location_valid = false;
    }

  }

  if(upload_request){
    task_upload_data();
  }

  loop_lcd();
  loop_elm();
  ble_loop();

  if(millis() - tm_convertion > convertion_time){
    tm_convertion = millis();
    update_distance_from_speed();

    if(rpmn > 500){
      engine_on = 1;
    }else{
      engine_on = 0;
    }
    if(engine_on != engine_on_prev){
      engine_on_prev = engine_on;
      if(!engine_on){
        lpg_eligible = false;
        lpg_likely = false;
        engine_start_ms = 0;
      }else{
        engine_start_ms = millis();
      }
    }
    update_lpg_state();

    if(kmph > (float)1.0){
      kms = kmph * (1.0/3600.0);
      lpkm = (float)(3600.0*maf)/(9069.90*kms);
    }else{
      lpkm = 0;
    }

    if(engine_on){
      consum_l_s = maf / 14.7;
      float lt_dts = (consum_l_s / 1000.0) * (millis() - tm_t_consum);
      tm_t_consum = millis();
      current_consumption_l = lt_dts;
      lts_trip += lt_dts;
    }

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
  
  if(debug_dashboard_enabled && millis() - tm_debug_report > debug_report_time){
    tm_debug_report = millis();
    printDebugDashboard();
  }
}


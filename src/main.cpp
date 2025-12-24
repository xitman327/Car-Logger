#define DEFAULT_DEMO_MODE 0
#define DEBUG_SERIAL Serial
#include "SPIFFS.h"
#include "user.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include "ArduinoJson.h"
#include <time.h>

#include "ble.h"

bool FS_STARTED = 0;

#define ENABLE_USER_CONFIG
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_FS
#include <FirebaseClient.h>
#include <ExampleFunctions.h>

bool demo_mode = DEFAULT_DEMO_MODE;
bool upload_request = 0;
bool firebase_initialized = false;

float fix_lat, fix_lng;
bool gps_location_valid = false;
uint8_t last_sat_count = 0;
time_t last_gps_fix_time = 0;

void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;
AsyncResult databaseResult;

String uid;
String databasePath;
String parentPath;

const char* ntpServer = "pool.ntp.org";

void upload_trip();
void request_trip_upload();



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

#include "obd.h"

float lpkm_max, rpmn_max, kmph_max;
bool engine_on;
bool engine_on_prev = false;

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

int num_of_files = 0;
String dir_filenames_array[11];
#define max_filenames 10

void update_lpg_state(){
  if(!engine_on){
    engine_start_ms = 0;
    lpg_eligible = false;
    lpg_likely = false;
    return;
  }
  if(engine_start_ms == 0){
    engine_start_ms = millis();
  }
  if(!lpg_eligible){
    if(engine_temp > lpg_min_coolant_c && millis() - engine_start_ms >= lpg_min_run_ms){
      lpg_eligible = true;
    }
  }
  if(lpg_eligible && rpmn > lpg_rpm_threshold){
    lpg_likely = true;
  }else if(rpmn < (lpg_rpm_threshold * 0.8f)){
    lpg_likely = false;
  }
}

void set_location(JsonVariant obj){
  if(gps_location_valid){
    obj["lng"] = fix_lng;
    obj["lat"] = fix_lat;
  }else{
    obj["lng"] = nullptr;
    obj["lat"] = nullptr;
  }
}

float get_effective_speed_kmph() {
  if (gps_speed_valid) return gps_speed_kmph;
  return kmph;
}

void update_distance_from_speed() {
  uint32_t now = millis();
  if (last_distance_update_ms == 0) {
    last_distance_update_ms = now;
    return;
  }
  uint32_t dt_ms = now - last_distance_update_ms;
  last_distance_update_ms = now;

  float speed_kmph = get_effective_speed_kmph();
  if (speed_kmph <= 0.1f) {
    return;
  }
  float hours = dt_ms / 3600000.0f;
  trip_distance_km += speed_kmph * hours;
}


String formatEpoch(time_t ts) {
  if (ts <= 0) return "-";
  struct tm tm_buf;
  if (!localtime_r(&ts, &tm_buf)) return "-";
  char buf[20]; // "YYYY-MM-DD HH:MM:SS" + null
  if (strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf) == 0) return "-";
  return String(buf);
}

String formatEpochForFilename(time_t ts) {
  if (ts <= 0) return "";
  struct tm tm_buf;
  if (!localtime_r(&ts, &tm_buf)) return "";
  char buf[20]; // "YYYY-MM-DD_HH-MM-SS" + null
  if (strftime(buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", &tm_buf) == 0) return "";
  return String(buf);
}

String formatKilobytes(size_t bytes) {
  float kb = bytes / 1024.0f;
  char buf[16];
  snprintf(buf, sizeof(buf), "%.2f", kb);
  for (char *p = buf; *p; ++p) {
    if (*p == '.') *p = ','; // use comma as decimal separator per request
  }
  return String(buf);
}

int wifi_signal_percent(){
  if(!WiFi.isConnected()) return 0;
  int32_t rssi = WiFi.RSSI();
  if(rssi <= -100) return 0;
  if(rssi >= -50) return 100;
  return (rssi + 100) * 2;
}

const char* uploadStageName(UploadStage stage) {
  switch (stage) {
    case UploadIdle: return "Idle";
    case UploadConnectWiFi: return "ConnectWiFi";
    case UploadAuth: return "Auth";
    case UploadSend: return "Send";
    case UploadComplete: return "Complete";
    default: return "Unknown";
  }
}

void printDebugDashboard() {
  String ip = WiFi.isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
  String now_str = formatEpoch(rtc.getEpoch());
  String last_fix_str = formatEpoch(last_gps_fix_time);
  size_t total_heap = ESP.getHeapSize();
  size_t free_heap = ESP.getFreeHeap();
  uint8_t used_pct = total_heap ? (uint8_t)(((total_heap - free_heap) * 100) / total_heap) : 0;
  String free_heap_kb = formatKilobytes(free_heap);
  String total_heap_kb = formatKilobytes(total_heap);

  Serial.println();
  Serial.println("===== DEBUG DASHBOARD =====");
  Serial.printf("Time: %s\n", now_str.c_str());
  Serial.printf("GPS : %s | sats:%u | last_fix:%s | speed:%.1f km/h\n",
    gps_location_valid ? "FIX" : "NO-FIX",
    last_sat_count,
    last_fix_str.c_str(),
    gps_speed_kmph);
  Serial.printf("WiFi: %s | IP:%s | signal:%d%%\n",
    WiFi.isConnected() ? "CONNECTED" : "OFFLINE",
    ip.c_str(),
    wifi_signal_percent());
  Serial.printf("OBD : protocol: %s\n", ELMprotocol.c_str());
  Serial.printf("Upload: stage:%s | in_progress:%d | current_idx:%d | files:%d\n",
    uploadStageName(upload_stage),
    upload_in_progress,
    current_upload_file_index,
    num_of_files);
  Serial.printf("Log  : %s | trip_dist:%.2f km | points:%lu | start_ts:%ld\n",
    log_started ? "ON " : "OFF",
    trip_distance_km,
    (unsigned long)trip_locations_count,
    (long)(single_trip_data["start_timestamp"] | 0L));
  Serial.printf("Car  : eng_on:%d rpm:%.0f kmph:%.1f gps_kmph:%.1f temp:%.1fC fuel:%.1f%% batt:%.2fV lpg:%d\n",
    engine_on,
    rpmn,
    kmph,
    gps_speed_kmph,
    engine_temp,
    fuel_level,
    battery_voltage,
    lpg_likely);
  Serial.printf("RAM : %sKB free / %sKB total (%u%% used)\n",
    free_heap_kb.c_str(),
    total_heap_kb.c_str(),
    used_pct);
  Serial.println("===========================");
  Serial.println();
}


#define CHUNK_SIZE 200

// int trip_locations_count = 0;
int chunkIndex = 0;
String tripBaseName;     // formatted datetime used as filename root
// bool log_started = false;

void trip_start() {
  log_started = true;
  trip_locations_count = 0;
  chunkIndex = 0;
  single_trip_data.clear();

  time_t now = rtc.getEpoch();
  tripBaseName = formatEpochForFilename(now);
  if (tripBaseName.isEmpty()) {
    tripBaseName = String(now);
  }

  single_trip_data["start_timestamp"] = now;
  single_trip_data["trip_locations_count"] = trip_locations_count;

  // FIXED: Use createNestedObject() instead of direct indexing
  JsonObject location = single_trip_data["trip_locations"].createNestedObject();
  location["time"] = now;
  set_location(location);
  float speed_to_log = get_effective_speed_kmph();
  location["speed"] = speed_to_log;
  location["lpg"] = lpg_likely;
  location["rpm"] = rpmn;
  location["engine_temp"] = engine_temp;
  location["lpkm"] = lpkm;

  Serial.printf("Trip started %s\n", tripBaseName.c_str());
}


// void split_chunk() {
//   if (single_trip_data.size() == 0) return;

//   char fname[40];
//   snprintf(fname, sizeof(fname), "/%s_%03d.json",
//            tripBaseName.c_str(), chunkIndex);

//   File f = SPIFFS.open(fname, FILE_WRITE);
//   if (!f) {
//     Serial.println("❌ Failed to save chunk");
//     return;
//   }

//   uint32_t wrote = serializeJson(single_trip_data, f);
//   f.close();

//   Serial.printf("💾 Chunk saved %s (%u bytes) — total count=%d\n",
//                 fname, wrote, trip_locations_count);

//   single_trip_data["trip_locations"].clear();  // free RAM
//   chunkIndex++;
// }

void split_chunk() {
  if (single_trip_data.size() == 0) return;

  char fname[40];
  snprintf(fname, sizeof(fname), "/%s_%03d.json",
           tripBaseName.c_str(), chunkIndex);

  File f = SPIFFS.open(fname, FILE_WRITE);
  if (!f) {
    Serial.println("❌ Failed to save chunk");
    return;
  }

  // Add end timestamp for this chunk
  single_trip_data["end_timestamp"] = rtc.getEpoch();
  
  uint32_t wrote = serializeJson(single_trip_data, f);
  f.close();

  Serial.printf("💾 Chunk saved %s (%u bytes) — total count=%d\n",
                fname, wrote, trip_locations_count);

  // Clear and prepare for next chunk
  single_trip_data.clear();
  
  // Start new chunk with current stats
  single_trip_data["start_timestamp"] = rtc.getEpoch();
  single_trip_data["trip_locations_count"] = 0;  // Reset for this chunk
  
  chunkIndex++;
}


void populate_current_json() {
  if (!log_started) return;

  kmph_max = max(kmph_max, kmph);
  single_trip_data["top_speed"] = kmph_max;

  lpkm_max = max(lpkm_max, lpkm);
  single_trip_data["max_consumption"] = lpkm_max;

  single_trip_data["trip_distance"] = trip_distance_km;

  // FIXED: Use createNestedObject() here too
  JsonObject location = single_trip_data["trip_locations"].createNestedObject();
  location["time"] = rtc.getEpoch();
  set_location(location);
  float speed_to_log = get_effective_speed_kmph();
  location["speed"] = speed_to_log;
  location["lpg"] = lpg_likely;
  location["rpm"] = rpmn;
  location["engine_temp"] = engine_temp;
  location["lpkm"] = lpkm;

  trip_locations_count++;
  single_trip_data["trip_locations_count"] = trip_locations_count;

  // split every CHUNK_SIZE locations
  if (trip_locations_count % CHUNK_SIZE == 0) {
    split_chunk();
  }
}


void trip_end() {
  if (!log_started) return;
  log_started = false;

  // Save leftover unsplit locations
  // if (single_trip_data["trip_locations"].size() > 0) {
  //   split_chunk();
  // }
  char fname[40];
  snprintf(fname, sizeof(fname), "/%s_%03d.json",
           tripBaseName.c_str(), chunkIndex);
  // Create final file WITHOUT suffix (last <1000 entries)
  // String fname = "/" + tripBaseName + ".json";
  File f = SPIFFS.open(fname, FILE_WRITE);
  if (!f) {
    Serial.println("❌ Failed to save final trip file");
    return;
  }

  uint32_t wrote = serializeJson(single_trip_data, f);
  f.close();

  Serial.printf("🏁 Final file saved %s (%u bytes), total locations=%d\n",
                fname, wrote, trip_locations_count);

  single_trip_data.clear();  // free RAM
  Serial.println("🧹 JSON cleared from RAM");
}


int get_filenames(){

  num_of_files = 0;
  Serial.println("listing files in /");
  if(!FS_STARTED){
    Serial.println("FS not started");
    return 0;
  }

  File root = SPIFFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
      if(num_of_files >= max_filenames)break;
      if(file.isDirectory()){
        file = root.openNextFile();
        continue;
      }
      String name = file.name();
      if(!name.endsWith(".json")){
        file = root.openNextFile();
        continue;
      }
      Serial.print("FILE: ");
      Serial.print(name);
      Serial.print("  ");
      Serial.println(file.size());
      dir_filenames_array[num_of_files] = name.startsWith("/") ? name : "/" + name;
      num_of_files++;
      file = root.openNextFile();
  }

  return num_of_files;
}


void delete_all_trips(){
  for(int i = get_filenames();i>0;i--){
    SPIFFS.remove(dir_filenames_array[max(i - 1, 0)]);
  }
}

void request_trip_upload(){
  upload_request = 1;
  upload_stage = WiFi.isConnected() ? UploadAuth : UploadConnectWiFi;
  wifi_connect_started_at = millis();
  last_wifi_attempt_ms = 0;
  wifi_attempt_count = 0;
  current_upload_file_index = -1;
  active_upload_file_index = -1;
  upload_in_progress = false;
  upload_error = false;
  upload_done_flag = false;
  db_retry_count = 0;
}

bool start_file_upload(int index){
  if(index < 0 || index >= num_of_files){
    return false;
  }

  if(!app.ready()){
    Serial.println("DB client not ready, cannot start upload");
    return false;
  }

  String filename = dir_filenames_array[index];
  if(!filename.endsWith(".json")){
    return false;
  }

  File upload_file = SPIFFS.open(filename, "r");
  if(!upload_file){
    Serial.printf("file %s not found\n", filename.c_str());
    return false;
  }

  Serial.printf("uploading file %s \n", upload_file.name());
  String trip_data;
  while(upload_file.available()){
    trip_data += char(upload_file.read());
  }
  upload_file.close();

  if(trip_data.isEmpty()){
    Serial.printf("file %s empty\n", filename.c_str());
    return false;
  }

  

  // Serial.println(trip_data);

  uid = app.getUid().c_str();
  databasePath = "/UsersData/" + uid + "/trips/";

  String sanitized_name = filename;

  // remove leading slash
  if (sanitized_name.startsWith("/")) {
    sanitized_name.remove(0, 1);
  }

  // remove .json extension
  if (sanitized_name.endsWith(".json")) {
    sanitized_name.remove(sanitized_name.length() - 5);
  }

  // remove chunk suffix `_000`, `_001`, `_002` ...
  int underscore_index = sanitized_name.lastIndexOf('_');
  if (underscore_index != -1) {
    // check if suffix is exactly 3 digits
    if (underscore_index + 4 == sanitized_name.length()) {
      if (isDigit(sanitized_name[underscore_index + 1]) &&
          isDigit(sanitized_name[underscore_index + 2]) &&
          isDigit(sanitized_name[underscore_index + 3])) {
        sanitized_name.remove(underscore_index);   // remove suffix
      }
    }
  }


  // String sanitized_name = filename;
  // if(sanitized_name.startsWith("/")){
  //   sanitized_name.remove(0,1);
  // }
  // if(sanitized_name.length() > 5){
  //   sanitized_name.remove(sanitized_name.length() - 5);
  // }else{
  //   sanitized_name = "trip";
  // }


  parentPath = databasePath + sanitized_name;

  upload_done_flag = 0;
  upload_error = false;
  upload_in_progress = true;
  upload_started_at = millis();
  active_upload_file_index = index;
  db_retry_count = 0;

  Database.update<object_t>(aClient, parentPath, object_t(trip_data), processData, "TRIP_DB_Send_Data");

  return true;
}

void correct_trip_time_if_needed() {
  if(!log_started) return;
  time_t now = rtc.getEpoch();
  if(now < 100000) return;

  long start_ts = single_trip_data["start_timestamp"] | 0L;
  if(start_ts >= 100000 && time_corrected_this_trip) return;

  single_trip_data["start_timestamp"] = now;
  single_trip_data["trip_locations"][0]["time"] = now;
  time_corrected_this_trip = true;
  Serial.println("Trip time corrected using synced clock");
}

void sync_rtc_from_gps_fix(const gps_fix &latest_fix) {
  if (latest_fix.valid.time && latest_fix.valid.date) {
    int year = latest_fix.dateTime.year;
    if (year < 100) {
      year += 2000;
    }
    rtc.setTime(
      latest_fix.dateTime.seconds,
      latest_fix.dateTime.minutes,
      latest_fix.dateTime.hours,
      latest_fix.dateTime.date,
      latest_fix.dateTime.month,
      year
    );
    rtc.setTime(rtc.getEpoch() + timezone_offset_seconds);
    gps_time_synced = true;
    correct_trip_time_if_needed();
  }

  gps_speed_valid = latest_fix.valid.speed;
  gps_speed_kmph = gps_speed_valid ? latest_fix.speed_kph() : 0;

  gps_location_valid = latest_fix.valid.location;
  if(gps_location_valid){
    fix_lat = latest_fix.latitude();
    fix_lng = latest_fix.longitude();
    last_gps_fix_time = rtc.getEpoch();
  }
  if(latest_fix.valid.satellites){
    last_sat_count = latest_fix.satellites;
  }
}

void request_ntp_time() {
  if (!WiFi.isConnected()) {
    return;
  }
  if (!ntp_requested || (millis() - last_ntp_attempt_ms) > ntp_retry_interval_ms) {
    last_ntp_attempt_ms = millis();
    ntp_requested = true;
    configTime(0, 0, ntpServer);
  }
}

void sync_time_from_wifi() {
  if (!WiFi.isConnected()) {
    return;
  }
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    rtc.setTimeStruct(timeinfo);
    rtc.setTime(rtc.getEpoch() + timezone_offset_seconds);
    wifi_time_synced = true;
    correct_trip_time_if_needed();
  } else {
    request_ntp_time();
  }
}

void ensureFirebaseReady() {
  if (!WiFi.isConnected()) {
    return;
  }

  if (!firebase_initialized) {
    set_ssl_client_insecure_and_buffer(ssl_client);
    initializeApp(aClient, app, getAuth(user_auth), 3000, auth_debug_print);
    app.autoAuthenticate(false);
    firebase_initialized = true;
  }

  if (!app.isAuthenticated()) {
    app.authenticate();
    return;
  }

  if (!app.ready()) {
    return;
  }

  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void upload_trip(){
  request_trip_upload();
}

void task_upload_data(){
  switch (upload_stage){

    case UploadIdle:
      if(upload_request){
        upload_stage = WiFi.isConnected() ? UploadAuth : UploadConnectWiFi;
        wifi_connect_started_at = millis();
      }
    break;

    case UploadConnectWiFi:
      if(WiFi.isConnected()){
        upload_stage = UploadAuth;
        wifi_attempt_count = 0;
        break;
      }
      if(last_wifi_attempt_ms == 0 || (millis() - last_wifi_attempt_ms) > wifi_retry_interval){
        last_wifi_attempt_ms = millis();
        if(wifi_attempt_count >= wifi_max_attempts){
          Serial.println("WiFi retry limit reached, aborting upload request");
          upload_stage = UploadIdle;
          upload_request = 0;
          break;
        }
        wifi_attempt_count++;
        WiFi.mode(WIFI_MODE_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        WiFi.setAutoReconnect(true);
        Serial.printf("Connect to %s (attempt %d/%d)\n", WIFI_SSID, wifi_attempt_count, wifi_max_attempts);
      }
      if(millis() - wifi_connect_started_at > wifi_connect_timeout){
        Serial.println("Connection Timed Out");
        upload_stage = UploadIdle;
        upload_request = 0;
      }
    break;

    case UploadAuth:
      if(!WiFi.isConnected()){
        upload_stage = UploadConnectWiFi;
        wifi_connect_started_at = millis();
        break;
      }
      ensureFirebaseReady();
      if(!app.isAuthenticated()){
        app.authenticate();
        break;
      }
      if(!app.ready()){
        break;
      }
      if(current_upload_file_index < 0){
        get_filenames();
        current_upload_file_index = 0;
      }
      upload_stage = UploadSend;
    break;

    case UploadSend:
      if(!WiFi.isConnected()){
        upload_stage = UploadConnectWiFi;
        wifi_connect_started_at = millis();
        break;
      }
      ensureFirebaseReady();
      if(!app.ready()){
        upload_stage = UploadAuth;
        break;
      }

      if(upload_in_progress){
        if(millis() - upload_started_at > upload_timeout_ms){
          Serial.println("Upload timed out, skipping file");
          upload_in_progress = false;
          upload_error = true;
        }
        break;
      }

      if(upload_done_flag || upload_error){
        if(upload_error){
          db_retry_count++;
          if(db_retry_count >= db_max_retries){
            Serial.println("Upload failed, retry limit reached, skipping file");
            db_retry_count = 0;
            current_upload_file_index++;
          }else{
            Serial.printf("Upload failed, retrying file (attempt %d/%d)\n", db_retry_count, db_max_retries);
          }
        }
        if(upload_done_flag){
          if(FS_STARTED && active_upload_file_index >= 0 && active_upload_file_index < num_of_files){
            String fname = dir_filenames_array[active_upload_file_index];
            if(SPIFFS.remove(fname)){
              Serial.printf("Upload success, deleted file %s\n", fname.c_str());
            }else{
              Serial.printf("Upload success, failed to delete file %s\n", fname.c_str());
            }
          }
          db_retry_count = 0;
          current_upload_file_index++;
        }
        upload_done_flag = false;
        upload_error = false;
        active_upload_file_index = -1;
      }
      
      if(current_upload_file_index >= num_of_files){
        upload_stage = UploadComplete;
        break;
      }

      if(!start_file_upload(current_upload_file_index)){
        upload_error = true;
      }
    break;

    case UploadComplete:
      upload_stage = UploadIdle;
      upload_request = 0;
    break;
  
    default:
    break;
  }
}

#define NEOPIXEL_PIN 12
#define NUMPIXELS 15 
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void runRgbWave(Adafruit_NeoPixel &strip, int numPixels, uint32_t baseColor, int waveWidth, float speed) {
  static float wavePosition = 0;
  
  for(int i = 0; i < numPixels; i++) {
    strip.setPixelColor(i, 0);
  }
  
  for(int i = 0; i < numPixels; i++) {
    float distance = fabs(i - wavePosition);
    
    if(distance < waveWidth) {
      float brightness = 1.0 - (distance / waveWidth);
      uint8_t r = (baseColor >> 16) & 0xFF;
      uint8_t g = (baseColor >> 8) & 0xFF;
      uint8_t b = baseColor & 0xFF;
      r *= brightness;
      g *= brightness;
      b *= brightness;
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
  }
  
  strip.show();
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
      wifi_time_synced = false;
      ntp_requested = false;
      break;
      
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      wifi_time_synced = false;
      ntp_requested = false;
      firebase_initialized = false;
      deinitializeApp(app);
      time_corrected_this_trip = false;
      break;

    default:
      break;
  }
}

#define light_sensor 13


HardwareSerial gpsSerial(2);


extern int num_of_files;

void logDebugStatus() {
  String ip = WiFi.isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
  time_t now_epoch = rtc.getEpoch();
  String last_fix_str = formatEpoch(last_gps_fix_time);
  String now_str = formatEpoch(now_epoch);
  size_t total_heap = ESP.getHeapSize();
  size_t free_heap = ESP.getFreeHeap();
  uint8_t used_pct = total_heap ? (uint8_t)(((total_heap - free_heap) * 100) / total_heap) : 0;
  String free_heap_kb = formatKilobytes(free_heap);
  Serial.printf(
    "DBG Status -> WiFi:%s IP:%s Firebase init:%d auth:%d ready:%d upload_stage:%s in_progress:%d current_idx:%d files:%d sats:%u last_fix:%s now:%s log:%d rpm:%.0f kmph:%.1f temp:%.1f fuel:%.1f batt:%.2fV lpg:%d RAM:%sKB %u%% used\r",
    WiFi.isConnected() ? "yes" : "no",
    ip.c_str(),
    firebase_initialized,
    app.isAuthenticated(),
    app.ready(),
    uploadStageName(upload_stage),
    upload_in_progress,
    current_upload_file_index,
    num_of_files,
    last_sat_count,
    last_fix_str.c_str(),
    now_str.c_str(),
    log_started,
    rpmn,
    kmph,
    engine_temp,
    fuel_level,
    battery_voltage,
    lpg_likely,
    free_heap_kb.c_str(),
    used_pct
  );
}

void handleDebugCommand(char key) {
  switch (key) {
    case 'K':
      rpmn = 2400;
      kmph = 140;
      maf = 5.7;
      Serial.println("DBG: Mock high load (K)");
      break;
    case 'L':
      rpmn = 0;
      kmph = 0;
      maf = 0;
      Serial.println("DBG: Mock idle (L)");
      break;
    case 'J':
      request_trip_upload();
      Serial.println("DBG: Upload requested (J/H)");
      break;
    case 'D':
      delete_all_trips();
      Serial.println("DBG: Deleted all trips (D)");
      break;
    case 'F':
      get_filenames();
      Serial.printf("DBG: Listed files, count=%d (F)\n", num_of_files);
      break;
    case 'W':
      if (WiFi.isConnected()) {
        Serial.println("DBG: WiFi already connected (W)");
      } else {
        WiFi.mode(WIFI_MODE_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        WiFi.setAutoReconnect(true);
        Serial.println("DBG: WiFi connect triggered (W)");
      }
      break;
    case 'A':
      ensureFirebaseReady();
      Serial.println("DBG: ensureFirebaseReady invoked (A)");
      break;
    case 'R':
      firebase_initialized = false;
      ensureFirebaseReady();
      Serial.println("DBG: Firebase reinit requested (R)");
      break;
    case 'S':
      printDebugDashboard();
      break;
    case 'U':
      upload_stage = WiFi.isConnected() ? UploadAuth : UploadConnectWiFi;
      wifi_connect_started_at = millis();
      last_wifi_attempt_ms = 0;
      current_upload_file_index = -1;
      active_upload_file_index = -1;
      upload_in_progress = false;
      upload_error = false;
      upload_done_flag = false;
      upload_request = 1;
      Serial.println("DBG: Upload state reset (U)");
      break;
    default:
      Serial.printf("DBG: Unhandled key '%c'\n", key);
      break;
  }
}
#include "lcd.h"
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
  ensureFirebaseReady();


}

#define afr_gassoline 14.7
#define dens_gassoline 710.0

#define convertion_time 1000
#define obd_pull_time 200

float temperature;
float humidity;
float pressure;

static uint8_t hue = 0;

#define log_time 50 //1000 or 2000

#define led_time 20
#define debug_report_time 2000
uint32_t tm_log, tm_led;
uint32_t tm_debug_report = 0;
void loop()
{
  if(WiFi.isConnected()){
    ensureFirebaseReady();
    app.loop();
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
  
  if(millis() - tm_debug_report > debug_report_time){
    tm_debug_report = millis();
    printDebugDashboard();
  }
}

void processData(AsyncResult &aResult) {

  if(aResult.uploadProgress()){
    int get_getprogress = aResult.uploadInfo().progress;
    Serial.println(get_getprogress);
    if(get_getprogress > 98){
      upload_done_flag = true;
      upload_in_progress = false;
      Serial.println("Upload done");
    }
  }

  if (aResult.isEvent()){
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
  }

  if (aResult.isDebug()){
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError()){
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    upload_error = true;
    upload_in_progress = false;
  }

  if (aResult.available()){
    upload_done_flag = true;
    upload_in_progress = false;
  }
}

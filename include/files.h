#include <HTTPClient.h>

#ifndef NODERED_USER
#define NODERED_USER ""
#define NODERED_PASS ""
#endif
// If your Node-RED endpoint is protected with basic auth, set these.
// Otherwise leave as empty strings.
const char* HTTP_USER = NODERED_USER;   // e.g. "admin"
const char* HTTP_PASS = NODERED_PASS;   // e.g. "secret"
const char* UPLOAD_URL = NODERED_URL;

static bool uploadFileSPIFFS(const char* url, String fileName) {
  if (!SPIFFS.exists(fileName)) {
    Serial.printf("File not found in SPIFFS: %s\n", fileName);
    return false;
  }

  File f = SPIFFS.open(fileName, FILE_READ);
  if (!f) {
    Serial.printf("Failed to open: %s\n", fileName);
    return false;
  }

  const size_t fileSize = f.size();
  if (fileSize == 0) {
    Serial.printf("File is empty: %s\n", fileName);
    f.close();
    return false;
  }

  HTTPClient http;
  WiFiClient client;

  Serial.printf("Uploading %s (%u bytes) -> %s\n", fileName, (unsigned)fileSize, url);

  if (!http.begin(client, url)) {
    Serial.println("http.begin() failed");
    f.close();
    return false;
  }

  // Optional Basic Auth
  if (HTTP_USER[0] != '\0') {
    http.setAuthorization(HTTP_USER, HTTP_PASS);
  }

  // Metadata headers (Node-RED can use these to name file)
  http.addHeader("Content-Type", "application/octet-stream");
  http.addHeader("X-Filename", fileName);

  // This is the important call: streams file directly, no big RAM buffer
  int code = http.sendRequest("POST", &f, fileSize);

  Serial.printf("HTTP response code: %d\n", code);
  String resp = http.getString();
  if (resp.length()) {
    Serial.println("Server response:");
    Serial.println(resp);
  }

  http.end();
  f.close();

  // Treat any 2xx as success
  return (code >= 200 && code < 300);
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

  file.close();
  root.close();

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
  upload_file.close();

  upload_done_flag = 0;
  upload_error = false;
  upload_in_progress = true;
  upload_started_at = millis();
  active_upload_file_index = index;
  db_retry_count = 0;

  if(uploadFileSPIFFS(UPLOAD_URL, filename)){
    upload_done_flag = true;
    upload_in_progress = false;
    Serial.println("Upload done");
  }

  return true;
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
    //   ensureFirebaseReady();
    //   if(!app.isAuthenticated()){
    //     app.authenticate();
    //     break;
    //   }
    //   if(!app.ready()){
    //     break;
    //   }
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
    //   ensureFirebaseReady();
    //   if(!app.ready()){
    //     upload_stage = UploadAuth;
    //     break;
    //   }

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
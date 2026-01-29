

int wifi_signal_percent(){
  if(!WiFi.isConnected()) return 0;
  int32_t rssi = WiFi.RSSI();
  if(rssi <= -100) return 0;
  if(rssi >= -50) return 100;
  return (rssi + 100) * 2;
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
  Serial.printf("GPS : %s | sats:%u | last_fix:%s | speed:%.1f km/h | last_gps_msg %d\n",
    gps_location_valid ? "FIX" : "NO-FIX",
    last_sat_count,
    last_fix_str.c_str(),
    gps_speed_kmph,
    last_gps);
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

void printAllSettings() {
  Serial.println();
  Serial.println("===== SETTINGS DUMP =====");
  for (int i = 0; i < 4; i++) {
    Serial.printf("WiFi[%d]: ssid:%s pass:%s enabled:%d\n",
      i,
      Wifi_credentials[i].WiFi_Name.c_str(),
      Wifi_credentials[i].WiFi_Pass.c_str(),
      Wifi_credentials[i].enabled ? 1 : 0);
  }
  Serial.printf("Node-RED: url:%s user:%s pass:%s\n",
    nodeRed_credentials.Node_URL.c_str(),
    nodeRed_credentials.Node_User.c_str(),
    nodeRed_credentials.Node_Pass.c_str());
  Serial.printf("Trip Start Condition: %u\n", (unsigned)trip_start_condition);
  Serial.print("PID Request List: ");
  for (size_t i = 0; i < sizeof(pid_request_list); i++) {
    Serial.printf("%02X", pid_request_list[i]);
    if (i + 1 < sizeof(pid_request_list)) Serial.print(" ");
  }
  Serial.println();
  Serial.println("==========================");
  Serial.println();
}


void handleDebugCommand(char key) {
  switch (key) {
    case 'H':
      Serial.println("DBG: Help (H)");
      Serial.println("Start Log (K)");
      Serial.println("Stop Log (L)");
      Serial.println("Deleted all trips (D)");
      Serial.println("Listed files (F)");
      Serial.println("DBG: Upload requested (J)");
      Serial.println("Upload state reset (U)");
      Serial.println("WiFi Force Connect (W)");
      Serial.println("Debug Dashboard (B)");
      Serial.println("Print Settings (S)");
    break;
    case 'K':
      temp_trip_start_condition = trip_start_condition;
      debug_log_start = 1;
      rpmn = 2400;
      kmph = 140;
      maf = 5.7;
      Serial.println("DBG: Start Log (K)");
      break;
    case 'L':
      debug_log_start = 0;
      trip_start_condition = temp_trip_start_condition;
      rpmn = 0;
      kmph = 0;
      maf = 0;
      Serial.println("DBG: Stop Log (L)");
      break;
    case 'J':
      request_trip_upload();
      Serial.println("DBG: Upload requested (J)");
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
    case 'B':
      debug_dashboard_enabled = !debug_dashboard_enabled;
      Serial.printf("DBG: Dashboard %s (B)\n", debug_dashboard_enabled ? "ENABLED" : "DISABLED");
      break;
    case 'S':
      printAllSettings();
      Serial.println("DBG: Settings printed (S)");
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
    case 'P':
      // Serial.println(pin)
    break;
    default:
      Serial.printf("DBG: Unhandled key '%c'\n", key);
      break;
  }
}

#include <ezButton.h>

ezButton button(USER_BUTTON);  // create ezButton object that attach to pin 7;
uint32_t tm_button_pressed;
#define button_presses_timeout 500

#define button_timeout (millis() - tm_button_pressed > button_presses_timeout)

void setup_button(){
  button.setDebounceTime(50); // set debounce time to 50 milliseconds
  button.setCountMode(COUNT_FALLING);
}

void loop_button(){
  button.loop();
  
  if(button.isPressed()){
    tm_button_pressed = millis();
    log_d("button pressed : %d", button.getCount());
  }

  if (button.getCount() >= 10 && button_timeout){
    default_settings();
    button.resetCount();
    log_d("default settings");
  }

  if(button.getCount() && button_timeout){
    button.resetCount();
    log_d("count reset");
  }


}

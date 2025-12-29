

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
    0, //firebase_initialized,
    0, //app.isAuthenticated(),
    0, //app.ready(),
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
    // case 'A':
    //   ensureFirebaseReady();
    //   Serial.println("DBG: ensureFirebaseReady invoked (A)");
    //   break;
    // case 'R':
    //   firebase_initialized = false;
    //   ensureFirebaseReady();
    //   Serial.println("DBG: Firebase reinit requested (R)");
    //   break;
    case 'B':
      debug_dashboard_enabled = !debug_dashboard_enabled;
      Serial.printf("DBG: Dashboard %s (B)\n", debug_dashboard_enabled ? "ENABLED" : "DISABLED");
      if (debug_dashboard_enabled) {
        printDebugDashboard();
      }
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
#define CHUNK_SIZE 500
int chunkIndex = 0;
String tripBaseName;     // formatted datetime used as filename root



void set_location(JsonVariant obj){
  if(gps_location_valid){
    obj.add(fix_lng);
    obj.add(fix_lat);
  }else{
    obj.add(nullptr);
    obj.add(nullptr);
  }
}

float get_effective_speed_kmph() {
  if (elm_connected || can_connected) return kmph;
  if (gps_speed_valid) return gps_speed_kmph;
  return 0.0;
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

void set_tripBaseName(){
  time_t now = rtc.getEpoch();
  tripBaseName = formatEpochForFilename(now);
  if (tripBaseName.isEmpty()) {
    tripBaseName = String(now);
  }
  tripBaseName += ".csv";
}
JsonDocument fileHeader;
void trip_start(){
  if (log_started) return;
  log_started = true;
  trip_locations_count = 0;
  trip_distance_km = 0.0;
  sync_time();
  time_t now = rtc.getEpoch();
  set_tripBaseName();

  // JsonDocument fileHeader;
  fileHeader["start_timestamp"] = now;
  fileHeader["trip_locations_count"] = trip_locations_count;
  JsonArray log_objs = fileHeader["log_objs"].add<JsonArray>();
  log_objs.add("time");
  log_objs.add("lng");
  log_objs.add("lat");
  for(int i =0; i < pid_request_list_size_max;i++){
    if(pid_request_list[i] != 0){
      String pname = pid_name(pid_request_list[i]);
      log_objs.add(pname);
      log_i("\e[0;36m List of pids to log: %d - PID %d - %s \e[0m]",i, pid_request_list[i], pname);
    }
  }

  if(sd_ready){
    if(sdfile.open(tripBaseName.c_str(), O_CREAT | O_RDWR)){
      log_i("File Openned %s", tripBaseName.c_str());
      uint32_t wrote = serializeJson(fileHeader, sdfile);
      sdfile.println();
      if(wrote > 0 && sdfile.size() > 0){
        log_i("File wrote success. %s (%u bytes)", tripBaseName.c_str(), wrote);
      }else{
        log_e("File error %d", sdfile.getError());
      }
      if(!sdfile.attrib(0)){Serial.println("clearing attributes failed");}
    }else{
      log_e("File error not oppened");
    }
    sdfile.close();
  }

  Serial.printf("Trip started %s\n", tripBaseName.c_str());

}

String trip_locations_buffer;
#define print_to_sdcard_limit 500
void populate_current_json() {
  if (!log_started) return;
  trip_locations_count++;
  time_t now = rtc.getEpoch();
  trip_locations_buffer += String(now);

  if(gps_location_valid){
    trip_locations_buffer += "," + String(fix_lng) + "," + String(fix_lat);
  }else{
    trip_locations_buffer += "," + String(-1) + "," + String(-1);
  }

  for(int i =0; i < pid_request_list_size_max;i++){
    if(pid_request_list[i] != 0){

      trip_locations_buffer += "," + String(pid_values[i]);
    }
  }

  trip_locations_buffer += "\n";

  
  if(trip_locations_buffer.length() > print_to_sdcard_limit){
    log_i("trip_locations_buffer size %d", trip_locations_buffer.length());
    if(sd_ready){
      if(sdfile.open(tripBaseName.c_str(), O_APPEND | O_AT_END | O_RDWR)){ // file should already exist
        log_i("File Openned %s", tripBaseName);
        uint32_t wrote = sdfile.print(trip_locations_buffer);
        if(wrote > 0 && sdfile.size() > 0){
          log_i("File wrote success. %s (%u bytes)", tripBaseName, wrote);
        }else{
          log_e("File error %d", sdfile.getError());
        }
        if(!sdfile.attrib(0)){Serial.println("clearing attributes failed");}
      }else{
        log_e("File error not oppened");
      }
      sdfile.close();
    }

    trip_locations_buffer.clear();
  }
}



void trip_end() {
  if (!log_started) return;
  log_started = false;
  fileHeader.clear();
  log_i("trip_locations_buffer size %d", trip_locations_buffer.length());
  if(trip_locations_buffer.length() > 100){
    if(sd_ready){
      if(sdfile.open(tripBaseName.c_str(), O_APPEND | O_AT_END | O_RDWR)){ // file should already exist
        log_i("File Openned %s", tripBaseName);
        uint32_t wrote = sdfile.println(trip_locations_buffer);
        if(wrote > 0 && sdfile.size() > 0){
          log_i("File wrote success. %s (%u bytes)", tripBaseName, wrote);
        }else{
          log_e("File error %d", sdfile.getError());
        }
        if(!sdfile.attrib(0)){Serial.println("clearing attributes failed");}
      }else{
        log_e("File error not oppened");
      }
      sdfile.close();
    }

    trip_locations_buffer.clear();
  }

}
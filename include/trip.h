#define CHUNK_SIZE 200
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

  // determine what objects (sensors - PIDs) to log
  JsonArray log_objs = single_trip_data["log_objs"].add<JsonArray>();
  log_objs.add("time");
  log_objs.add("lng");
  log_objs.add("lat");
  for(int i =0; i < pid_request_list_size_max;i++){
    if(pid_request_list[i] != 0){
      String pname = pid_name(pid_request_list[i]);
      log_objs.add(pname);
      log_i("\e[0;36m List of pids to log: %d - PID %d - %s \n",i, pid_request_list[i], pname);
    }
  }
  
  JsonArray location = single_trip_data["trip_locations"].add<JsonArray>();
  location.add(now);

  set_location(location);
  // location["speed"] = get_effective_speed_kmph();
  // Serial.println("List of pids to log");
  for(int i =0; i < pid_request_list_size_max;i++){
    if(pid_request_list[i] != 0){
      // String pname = pid_name(pid_request_list[i]);
      location.add((float) pid_values[i]);
      // log_i("\e[0;36m %d - PID %d - %s : %f",i, pid_request_list[i], pname, pid_values[i]);
    }
  }
  Serial.printf("Trip started %s\n", tripBaseName.c_str());
}

void split_chunk() {
  if (single_trip_data.size() == 0) return;

  char fname[40];
  snprintf(fname, sizeof(fname), "/%s_%03d.json",
           tripBaseName.c_str(), chunkIndex);

  single_trip_data["end_timestamp"] = rtc.getEpoch();

  if(sd_ready){
    if(sdfile.open(fname, O_RDWR)){
      log_i("File Openned");
      uint32_t wrote = serializeJson(single_trip_data, sdfile);
      if(wrote > 0 && sdfile.size() > 0){
        log_i("File wrote success. %s (%u bytes) — total count=%d", fname, wrote, trip_locations_count);
      }else{
        log_e("File error %d", sdfile.getError());
      }
      if(!sdfile.attrib(0)){Serial.println("clearing attributes failed");}
    }else{
      log_e("File error %d", sdfile.getError());
    }
    sdfile.close();
  }else{
    log_w("USING SPIFFS, LIMITED MEMMORY");
    File f = SPIFFS.open(fname, FILE_WRITE);
    if (f) {
      uint32_t wrote = serializeJson(single_trip_data, f);
      log_i("File wrote success. %s (%u bytes) — total count=%d\n", fname, wrote, trip_locations_count);
    }else{
      log_e("File error %d", f.getWriteError());
    }
    f.close();
  }

  // Clear and prepare for next chunk
  single_trip_data.clear();
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
  // JsonObject location = single_trip_data["trip_locations"].add<JsonObject>();
  // location["time"] = rtc.getEpoch();
  // set_location(location);
  // // location["speed"] = get_effective_speed_kmph();
  // for(int i =0; i < pid_request_list_size_max;i++){
  //   if(pid_request_list[i] != 0){
  //     String pname = pid_name(pid_request_list[i]);
  //     // Serial.println(pname);
  //     location[pname] = pid_values[i];
  //     // log_i("\e[0;36m %d - PID %d - %s : %f",i, pid_request_list[i], pname, pid_values[i]);
  //   }
  // }
  time_t now = rtc.getEpoch();
  JsonArray location = single_trip_data["trip_locations"].add<JsonArray>();
  location.add(now);

  set_location(location);
  // location["speed"] = get_effective_speed_kmph();
  // Serial.println("List of pids to log");
  for(int i =0; i < pid_request_list_size_max;i++){
    if(pid_request_list[i] != 0){
      // String pname = pid_name(pid_request_list[i]);
      location.add((float) pid_values[i]);
      // log_i("\e[0;36m %d - PID %d - %s : %f",i, pid_request_list[i], pname, pid_values[i]);
    }
  }

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

  char fname[40];
  snprintf(fname, sizeof(fname), "/%s_%03d.json",
           tripBaseName.c_str(), chunkIndex);

  single_trip_data["end_timestamp"] = rtc.getEpoch();
  if(sd_ready){
    if(sdfile.open(fname, O_RDWR | O_CREAT)){
      log_i("File Openned");
      uint32_t wrote = serializeJson(single_trip_data, sdfile);
      if(wrote > 0 && sdfile.size() > 0){
        log_i("File wrote success. %s (%u bytes) — total count=%d", fname, wrote, trip_locations_count);
      }else{
        log_e("File error %d", sdfile.getError());
      }
      if(!sdfile.attrib(0)){Serial.println("clearing attributes failed");}
    }else{
      log_e("File error %d", sdfile.getError());
    }
    sdfile.close();
  }else{
    log_w("USING SPIFFS, LIMITED MEMMORY");
    File f = SPIFFS.open(fname, FILE_WRITE);
    if (f) {
      uint32_t wrote = serializeJson(single_trip_data, f);
      log_i("File wrote success. %s (%u bytes) — total count=%d\n", fname, wrote, trip_locations_count);
    }else{
      log_e("File error %d", f.getWriteError());
    }
    f.close();
  }

  single_trip_data.clear();  // free RAM
  Serial.println("🧹 JSON cleared from RAM");
}
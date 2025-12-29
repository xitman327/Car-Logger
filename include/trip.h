#define CHUNK_SIZE 200
int chunkIndex = 0;
String tripBaseName;     // formatted datetime used as filename root



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
  // single_trip_data["start_timestamp"] = rtc.getEpoch();
  // single_trip_data["trip_locations_count"] = 0;  // Reset for this chunk
  
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
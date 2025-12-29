#define light_sensor 13
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
      // deinitializeApp(app);
      time_corrected_this_trip = false;
      break;

    default:
      break;
  }
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
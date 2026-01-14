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
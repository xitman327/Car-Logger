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

void sync_time_from_wifi() {
  if (!WiFi.isConnected()) {
    return;
  }
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo);
    wifi_time_synced = true;
    Serial.print("Local Time: ");Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }else{
    log_e("cannot get time");
  }
}

void force_connect_wifi(){
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin("Xitos_Home", "xitosman327");
}
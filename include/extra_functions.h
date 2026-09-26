enum wifi_status_t
{
  WIFI_IDLE = 0,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED
};

wifi_status_t current_wifi_status = WIFI_IDLE;

// Connection tracking variables
static int current_network_index = 0;
static int attempts_for_current_network = 0;
static unsigned long last_attempt_time = 0;
static const unsigned long CONNECTION_TIMEOUT = 10000; // 10 seconds timeout
static String last_connected_network = "";
static int last_connected_index = -1;

// Helper function to manually trigger reconnection
void request_wifi_connection()
{
  wifi_connect_request = true;
  if (current_wifi_status == WIFI_FAILED)
  {
    current_wifi_status = WIFI_IDLE;
  }
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
  switch (event)
  {
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
    // firebase_initialized = false;
    // deinitializeApp(app);
    time_corrected_this_trip = false;
    break;

  default:
    break;
  }
}

void sync_time_from_wifi()
{
  if (!WiFi.isConnected())
  {
    return;
  }
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
  tzset();
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    rtc.setTimeStruct(timeinfo);
    wifi_time_synced = true;
    Serial.print("Local Time: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
  else
  {
    log_e("cannot get time");
  }
}

void force_connect_wifi()
{
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin("Xitos_Home", "xitosman327");
}

void task_upload_data()
{
  switch (upload_stage)
  {
  case UploadIdle:
    if (upload_request)
    {
      log_i("upload requested");
      if (WiFi.isConnected())
      {
        upload_stage = UploadSend;
      }
      else
      {
        upload_stage = UploadConnectWiFi;
      }
    }
    break;
  case UploadConnectWiFi:
    if (WiFi.isConnected())
    {
      upload_stage = UploadSend;
      break;
    }

    if (current_wifi_status == WIFI_IDLE && wifi_connect_request == false)
    {
      log_i("wifi not connected, connecting");
      request_wifi_connection();
      break;
    }

    break;
  case UploadSend:
    if (num_of_files < 1)
    {
      upload_stage = UploadIdle;
      upload_request = false;
      log_i("No files to upload");
      break;
    }
    log_i("uploading files");
    uploadPendingFiles();
    if (lastUploadFail == UPLOAD_OK)
    {
      upload_stage = UploadIdle;
    }
    upload_stage = UploadIdle;
    upload_request = false;

    break;
  }
}

// Function to reset connection attempts
void reset_wifi_connection_attempts()
{
  current_network_index = 0;
  attempts_for_current_network = 0;
  last_attempt_time = 0;
}

// Function to try connecting to a specific network
bool try_connect_to_network(int index)
{
  if (index >= 4 || !Wifi_credentials[index].enabled)
  {
    return false;
  }

  log_i("Attempting to connect to: %s (Attempt %d)\n",
                Wifi_credentials[index].WiFi_Name.c_str(),
                attempts_for_current_network + 1);

  WiFi.begin(Wifi_credentials[index].WiFi_Name.c_str(),
             Wifi_credentials[index].WiFi_Pass.c_str());
  return true;
}

// Main non-blocking WiFi management function
void manage_wifi()
{
  static unsigned long last_update = 0;
  unsigned long current_time = millis();

  // Update every 100ms to prevent excessive checking
  if (current_time - last_update < 100)
  {
    return;
  }
  last_update = current_time;

  switch (current_wifi_status)
  {
  case WIFI_IDLE:
    // Check if connection requested
    if (wifi_connect_request)
    {
      current_wifi_status = WIFI_CONNECTING;
      reset_wifi_connection_attempts();

      // If we have a last connected network, try it first
      if (last_connected_index >= 0 &&
          last_connected_index < 4 &&
          Wifi_credentials[last_connected_index].enabled)
      {
        current_network_index = last_connected_index;
      }
      else
      {
        // Find first enabled network
        current_network_index = 0;
        while (current_network_index < 4 && !Wifi_credentials[current_network_index].enabled)
        {
          current_network_index++;
        }
      }

      if (current_network_index < 4)
      {
        try_connect_to_network(current_network_index);
        last_attempt_time = current_time;
      }
      else
      {
        current_wifi_status = WIFI_FAILED;
        wifi_connect_request = false;
      }
    }
    break;

  case WIFI_CONNECTING:
    // Check if connection timeout
    if (current_time - last_attempt_time > CONNECTION_TIMEOUT)
    {
      attempts_for_current_network++;

      // If we haven't exhausted attempts for this network
      if (attempts_for_current_network < 2)
      {
        // Retry same network
        try_connect_to_network(current_network_index);
        last_attempt_time = current_time;
      }
      else
      {
        // Move to next network
        current_network_index++;
        attempts_for_current_network = 0;

        // Find next enabled network
        while (current_network_index < 4 && !Wifi_credentials[current_network_index].enabled)
        {
          current_network_index++;
        }

        if (current_network_index < 4)
        {
          try_connect_to_network(current_network_index);
          last_attempt_time = current_time;
        }
        else
        {
          // No more networks to try
          current_wifi_status = WIFI_FAILED;
          wifi_connect_request = false;
          Serial.println("All networks failed to connect");
        }
      }
    }

    // Check if connected
    if (WiFi.status() == WL_CONNECTED)
    {
      current_wifi_status = WIFI_CONNECTED;
      wifi_connect_request = false;
      last_connected_network = Wifi_credentials[current_network_index].WiFi_Name;
      last_connected_index = current_network_index;
      Serial.printf("Connected to: %s, IP: %s\n",
                    last_connected_network.c_str(),
                    WiFi.localIP().toString().c_str());
    }
    break;

  case WIFI_CONNECTED:
    // Monitor connection
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi connection lost!");
      current_wifi_status = WIFI_IDLE;
      // Optional: Auto-reconnect
      // wifi_connect_request = true;
    }
    break;

  case WIFI_FAILED:
    // Stay in failed state until new connection request
    if (wifi_connect_request)
    {
      current_wifi_status = WIFI_IDLE;
      // Reset the request flag to trigger new connection attempt
      // The next loop iteration will handle it
    }
    break;
  }
}

// Helper function to get current status as string
String get_wifi_status_string()
{
  switch (current_wifi_status)
  {
  case WIFI_IDLE:
    return "IDLE";
  case WIFI_CONNECTING:
    return "CONNECTING";
  case WIFI_CONNECTED:
    return "CONNECTED";
  case WIFI_FAILED:
    return "FAILED";
  default:
    return "UNKNOWN";
  }
}

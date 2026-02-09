// Define your structs
typedef struct{
  String WiFi_Name;
  String WiFi_Pass;
  bool enabled;
} wifi_preferences_t; // Fixed spelling

typedef struct{
  String Node_User;
  String Node_Pass;
  String Node_URL;
} nodered_preferences_t;

// Global instances
wifi_preferences_t Wifi_credentials[4];
nodered_preferences_t nodeRed_credentials;

void default_settings();

void load_settings(){
  if(!sd_ready){
    log_e("SD card not ready");
    return;
  }
  
  // if(!sdfile.exists("config.ini")){
  //   log_e("config.ini does not exist");
  //   // Consider calling default_settings() here if file doesn't exist
  //   default_settings();
  //   return;
  // }
  
  if(!sdfile.open("config.ini", O_RDONLY)){  // Changed to O_RDONLY for reading
    log_e("config.ini can not open");
    return;
  }
  
  const size_t fileSize = sdfile.size();
  if (fileSize == 0) {
    log_e("SD File is empty");
    sdfile.close();
    default_settings();  // Create default if empty
    return;
  }
  
  // Use appropriate document type based on file size
  JsonDocument configs;
  
  DeserializationError error = deserializeJson(configs, sdfile);
  sdfile.close();
  
  if(error){
    log_e("deserialization error: %s", error.c_str());
    return;
  }
  
  // Check if wifi array exists
  if(configs["wifi"].is<JsonArray>()){
    JsonArray wifiArray = configs["wifi"].as<JsonArray>();
    int maxEntries = 4;
    
    for(int i = 0; i < maxEntries; i++){
      JsonObject wifiObj = wifiArray[i];
        Wifi_credentials[i].WiFi_Name = wifiObj["name"].as<String>();
        Wifi_credentials[i].WiFi_Pass = wifiObj["pass"].as<String>();
        Wifi_credentials[i].enabled = wifiObj["enabled"].as<bool>();
    }
  } else {
    log_w("No wifi credentials found in config");
  }
  
  // Load Node-RED credentials with null checks
    JsonObject nrObj = configs["nodered"];
      nodeRed_credentials.Node_User = nrObj["user"].as<String>();
      nodeRed_credentials.Node_Pass = nrObj["pass"].as<String>();
      nodeRed_credentials.Node_URL = nrObj["url"].as<String>();
  
  // Load PID request list
  if(configs["pid_req"].is<JsonArray>()){
    JsonArray pidArray = configs["pid_req"].as<JsonArray>();
    
    for(int i = 0; i < pid_request_list_size; i++){
      pid_request_list[i] = pidArray[i].as<uint8_t>();
    }
  }
  
  // Load trip start condition
    trip_start_condition = configs["trip_start"].as<int>();
  
  log_i("Settings loaded successfully");
}

void save_settings(){
  if(!sd_ready){
    log_e("SD card not ready");
    return;
  }
  
  // Create a new document to avoid memory fragmentation
JsonDocument configs;  // Adjust size as needed
  
  // Create wifi array
  JsonArray wifiArray = configs.createNestedArray("wifi");
  
  for(int i = 0; i < 4; i++){
    JsonObject wifiObj = wifiArray.createNestedObject();
    wifiObj["name"] = Wifi_credentials[i].WiFi_Name;
    wifiObj["pass"] = Wifi_credentials[i].WiFi_Pass;
    wifiObj["enabled"] = Wifi_credentials[i].enabled;
  }
  
  // Create nodered object
  JsonObject nrObj = configs.createNestedObject("nodered");
  nrObj["user"] = nodeRed_credentials.Node_User;
  nrObj["pass"] = nodeRed_credentials.Node_Pass;
  nrObj["url"] = nodeRed_credentials.Node_URL;
  
  // Create pid_req array
  JsonArray pidArray = configs.createNestedArray("pid_req");
  for(int i = 0; i < pid_request_list_size; i++){
    pidArray.add(pid_request_list[i]);
  }
  
  // Add trip start condition
  configs["trip_start"] = trip_start_condition;
  
  // Save to file
  if(!sdfile.open("config.ini", O_CREAT | O_WRONLY | O_CREAT)){
    log_e("config.ini can not open for writing");
    return;
  }

  size_t bytesWritten = serializeJson(configs, sdfile);
  sdfile.close();
  
  if(bytesWritten > 0){
    log_i("Settings saved successfully (%d bytes)", bytesWritten);
  } else {
    log_e("Failed to save settings");
  }
}

void default_settings(){
  // Initialize with empty strings first
  for(int i = 0; i < 4; i++){
    Wifi_credentials[i].WiFi_Name = "";
    Wifi_credentials[i].WiFi_Pass = "";
    Wifi_credentials[i].enabled = false;
  }
  
  // Set default values
  Wifi_credentials[0] = {"Xitos1", "123456789", true};
  Wifi_credentials[1] = {"Xitos2", "123456789", false};
  Wifi_credentials[2] = {"Xitos3", "123456789", false};
  Wifi_credentials[3] = {"Xitos4", "123456789", false};
  
  nodeRed_credentials.Node_User = "xitos";
  nodeRed_credentials.Node_Pass = "xitos123456789";
  nodeRed_credentials.Node_URL = "https://noderedtest.com/test";
  
  uint8_t pid_request_list_temp[pid_request_list_size_max] = {
    PID_ENGINE_RPM, 
    PID_VEHICLE_SPEED, 
    PID_MAF_AIR_FLOW, 
    PID_THROTTLE_POSITION, 
    PID_ENGINE_COOLANT_TEMP, 
    PID_CONTROL_MODULE_VOLTAGE
  };
  
  for(int i = 0; i < pid_request_list_size_max; i++){
    pid_request_list[i] = pid_request_list_temp[i];
  }
  
  trip_start_condition = ENG_RPM;
  
  log_i("Default settings applied");
  save_settings();
}











// #include <Preferences.h>

// Preferences myPreferences;

// typedef struct{
//   String WiFi_Name;
//   String WiFi_Pass;
//   bool enabled;
// }wifi_preferenses_t;

// typedef struct{
//   String Node_User;
//   String Node_Pass;
//   String Node_URL;
// }nodered_preferences_t;

// wifi_preferenses_t Wifi_credentials[4];
// nodered_preferences_t nodeRed_credentials;

// JsonDocument configs;


// void load_settings(){
//   if(sd_ready){
//     if(!sdfile.exists("config.ini")){log_e("config.ini does not exist"); return;}
//     if(!sdfile.open("config.ini", O_RDWR)){log_e("config.ini can not open"); return;}
//     const size_t fileSize = sdfile.size();
//     if (fileSize == 0) {log_e("SD File is empty"); sdfile.close(); return;}
//     configs.clear();
//     if(deserializeJson(configs, sdfile) != DeserializationError::Ok){log_e("deserialization error"); return;}
//     sdfile.close();

//      for(int i=0;i<4;i++){
//       Wifi_credentials[i].WiFi_Name = configs["wifi"][i]["name"];
//       Wifi_credentials[i].WiFi_Pass = configs["wifi"][i]["pass"];
//       Wifi_credentials[i].enabled = configs["wifi"][i]["enabled"];
//     }

//     nodeRed_credentials.Node_User = configs["nodered"]["user"];
//     nodeRed_credentials.Node_Pass = configs["nodered"]["pass"];
//     nodeRed_credentials.Node_URL = configs["nodered"]["url"];

//     for(int i=0;i<pid_request_list_size;i++){
//       pid_request_list[i] = configs["pid_req"][i];
//     }

//     trip_start_condition = configs["trip_start"];

//     // configs.clear();
//   }
// }

// void save_settings(){
//   if(sd_ready){

//     // JsonDocument configs;
//     for(int i=0;i<4;i++){
//       configs["wifi"][i]["name"] = Wifi_credentials[i].WiFi_Name;
//       configs["wifi"][i]["pass"] = Wifi_credentials[i].WiFi_Pass;
//       configs["wifi"][i]["enabled"] = Wifi_credentials[i].enabled;
//     }

//     configs["nodered"]["user"] = nodeRed_credentials.Node_User;
//     configs["nodered"]["pass"] = nodeRed_credentials.Node_Pass;
//     configs["nodered"]["url"] = nodeRed_credentials.Node_URL;

//     for(int i=0;i<pid_request_list_size;i++){
//       configs["pid_req"][i] = pid_request_list[i];
//     }
    
//     configs["trip_start"] = trip_start_condition;

//     if(!sdfile.open("config.ini", O_CREAT | O_RDWR)){log_e("config.ini can not open"); return;}
//     serializeJsonPretty(configs, sdfile);
//     sdfile.close();
//     // configs.clear();


//   }
// }

// void default_settings(){
//   Wifi_credentials[0] = {"Xitos1", "123456789", true};
//   Wifi_credentials[1] = {"Xitos2", "123456789", false};
//   Wifi_credentials[2] = {"Xitos3", "123456789", false};
//   Wifi_credentials[3] = {"Xitos4", "123456789", false};
//   nodeRed_credentials = {"xitos", "xitos123456789", "https://noderedtest.com/test"};
//   uint8_t pid_request_list_temp[pid_request_list_size_max] = {PID_ENGINE_RPM, PID_VEHICLE_SPEED, PID_MAF_AIR_FLOW, PID_THROTTLE_POSITION, PID_ENGINE_COOLANT_TEMP, PID_CONTROL_MODULE_VOLTAGE};
//   for(int i=0;i<pid_request_list_size;i++){
//       pid_request_list[i] = pid_request_list_temp[i];
//     }
//   trip_start_condition = ENG_RPM;

//   save_settings();

// }



// #define wifi_creds "wifi_creds"
// #define node_creds "node_creds"
// #define pid_val "pid_values"
// #define log_start_reason_name "log_reason"
// #define settings_v2 "settings_v2"

// void set_default_credentials(){
//   Wifi_credentials[0] = {"Xitos1", "123456789", true};
//   Wifi_credentials[1] = {"Xitos2", "123456789", false};
//   Wifi_credentials[2] = {"Xitos3", "123456789", false};
//   Wifi_credentials[3] = {"Xitos4", "123456789", false};
//   nodeRed_credentials = {"xitos", "xitos123456789", "https://noderedtest.com/test"};
//   uint8_t pid_request_list_temp[pid_request_list_size_max] = {PID_ENGINE_RPM, PID_VEHICLE_SPEED, PID_MAF_AIR_FLOW, PID_THROTTLE_POSITION, PID_ENGINE_COOLANT_TEMP, PID_CONTROL_MODULE_VOLTAGE};
//   for(int i=0;i<pid_request_list_size;i++){
//       pid_request_list[i] = pid_request_list_temp[i];
//     }
//   trip_start_condition = ENG_RPM;
// }























// void pref_key(char *out, size_t out_len, const char *prefix, int index){
//   snprintf(out, out_len, "%s%d", prefix, index);
// }

// void default_settings(){

//   set_default_credentials();

//   uint8_t pid_request_list_temp[pid_request_list_size_max] = {PID_ENGINE_RPM, PID_VEHICLE_SPEED, PID_MAF_AIR_FLOW, PID_THROTTLE_POSITION, PID_ENGINE_COOLANT_TEMP, PID_CONTROL_MODULE_VOLTAGE};

//   trip_start_condition = ENG_RPM;

//   myPreferences.begin("default", false);
//   myPreferences.putBool(settings_v2, true);
//   for(int i = 0; i < 4; i++){
//     char key[16];
//     pref_key(key, sizeof(key), "w_ssid_", i);
//     myPreferences.putString(key, Wifi_credentials[i].WiFi_Name);
//     pref_key(key, sizeof(key), "w_pass_", i);
//     myPreferences.putString(key, Wifi_credentials[i].WiFi_Pass);
//     pref_key(key, sizeof(key), "w_en_", i);
//     myPreferences.putBool(key, Wifi_credentials[i].enabled);
//   }
//   myPreferences.putString("nr_user", nodeRed_credentials.Node_User.c_str());
//   myPreferences.putString("nr_pass", nodeRed_credentials.Node_Pass);
//   myPreferences.putString("nr_url", nodeRed_credentials.Node_URL);
//   if(myPreferences.putBytes(pid_val, &pid_request_list_temp, sizeof(pid_request_list_temp)) == 0){log_e("pid_request_list failled to save");}
//   if(myPreferences.putBytes(log_start_reason_name, &trip_start_condition, sizeof(trip_start_condition)) == 0){log_e("trip_start_condition failled to save");}
//   myPreferences.end();

//   log_i("\e[0;32m Default Settings saved");
// }

// void save_settings(){
//   myPreferences.begin("default", false);
//   myPreferences.putBool(settings_v2, true);
//   for(int i = 0; i < 4; i++){
//     char key[16];
//     pref_key(key, sizeof(key), "w_ssid_", i);
//     myPreferences.putString(key, Wifi_credentials[i].WiFi_Name);
//     pref_key(key, sizeof(key), "w_pass_", i);
//     myPreferences.putString(key, Wifi_credentials[i].WiFi_Pass);
//     pref_key(key, sizeof(key), "w_en_", i);
//     myPreferences.putBool(key, Wifi_credentials[i].enabled);
//   }
//   myPreferences.putString("nr_user", nodeRed_credentials.Node_User);
//   myPreferences.putString("nr_pass", nodeRed_credentials.Node_Pass);
//   myPreferences.putString("nr_url", nodeRed_credentials.Node_URL);
//   if(myPreferences.putBytes(pid_val, &pid_request_list, sizeof(pid_request_list)) == 0){log_e("pid_request_list failled to save");}
//   if(myPreferences.putBytes(log_start_reason_name, &trip_start_condition, sizeof(trip_start_condition)) == 0){log_e("pid_request_list failled to save");}
//   myPreferences.end();

//   log_i("\e[0;32m Settings saved");
// }

// void load_settings(){
//   myPreferences.begin("default", false);
//   bool v2 = myPreferences.getBool(settings_v2, false);
//   if(!v2){
//     myPreferences.end();
//     default_settings();
//     return;
//   }

//   set_default_credentials();
//   for(int i = 0; i < 4; i++){
//     char key[16];
//     pref_key(key, sizeof(key), "w_ssid_", i);
//     Wifi_credentials[i].WiFi_Name = myPreferences.getString(key, Wifi_credentials[i].WiFi_Name);
//     pref_key(key, sizeof(key), "w_pass_", i);
//     Wifi_credentials[i].WiFi_Pass = myPreferences.getString(key, Wifi_credentials[i].WiFi_Pass);
//     pref_key(key, sizeof(key), "w_en_", i);
//     Wifi_credentials[i].enabled = myPreferences.getBool(key, Wifi_credentials[i].enabled);
//   }
//   nodeRed_credentials.Node_User = myPreferences.getString("nr_user", nodeRed_credentials.Node_User);
//   nodeRed_credentials.Node_Pass = myPreferences.getString("nr_pass", nodeRed_credentials.Node_Pass);
//   nodeRed_credentials.Node_URL = myPreferences.getString("nr_url", nodeRed_credentials.Node_URL);
//   if(myPreferences.getBytes(pid_val, &pid_request_list, sizeof(pid_request_list)) == 0 ){log_e("pid_request_list failled to load");}
//   if(myPreferences.getBytes(log_start_reason_name, &trip_start_condition, sizeof(trip_start_condition)) == 0 ){log_e("pid_request_list failled to load");}

//   myPreferences.end();
//   log_i("\e[0;32m settings loaded");
// }

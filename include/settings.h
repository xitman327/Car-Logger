
#include <Preferences.h>

Preferences myPreferences;

typedef struct{
  String WiFi_Name;
  String WiFi_Pass;
  bool enabled;
}wifi_preferenses_t;

typedef struct{
  String Node_User;
  String Node_Pass;
  String Node_URL;
}nodered_preferences_t;

wifi_preferenses_t Wifi_credentials[4];
nodered_preferences_t nodeRed_credentials;

#define wifi_creds "wifi_creds"
#define node_creds "node_creds"
#define pid_val "pid_values"
#define log_start_reason_name "log_reason"
#define settings_v2 "settings_v2"

void set_default_credentials(){
  Wifi_credentials[0] = {"Xitos1", "123456789", true};
  Wifi_credentials[1] = {"Xitos2", "123456789", false};
  Wifi_credentials[2] = {"Xitos3", "123456789", false};
  Wifi_credentials[3] = {"Xitos4", "123456789", false};
  nodeRed_credentials = {"xitos", "xitos123456789", "https://noderedtest.com/test"};
}

void pref_key(char *out, size_t out_len, const char *prefix, int index){
  snprintf(out, out_len, "%s%d", prefix, index);
}

void default_settings(){

  set_default_credentials();

  uint8_t pid_request_list_temp[pid_request_list_size_max] = {PID_ENGINE_RPM, PID_VEHICLE_SPEED, PID_MAF_AIR_FLOW, PID_THROTTLE_POSITION, PID_ENGINE_COOLANT_TEMP, PID_CONTROL_MODULE_VOLTAGE};

  trip_start_condition = ENG_RPM;

  myPreferences.begin("default", false);
  myPreferences.putBool(settings_v2, true);
  for(int i = 0; i < 4; i++){
    char key[16];
    pref_key(key, sizeof(key), "w_ssid_", i);
    myPreferences.putString(key, Wifi_credentials[i].WiFi_Name);
    pref_key(key, sizeof(key), "w_pass_", i);
    myPreferences.putString(key, Wifi_credentials[i].WiFi_Pass);
    pref_key(key, sizeof(key), "w_en_", i);
    myPreferences.putBool(key, Wifi_credentials[i].enabled);
  }
  myPreferences.putString("nr_user", nodeRed_credentials.Node_User);
  myPreferences.putString("nr_pass", nodeRed_credentials.Node_Pass);
  myPreferences.putString("nr_url", nodeRed_credentials.Node_URL);
  if(myPreferences.putBytes(pid_val, &pid_request_list_temp, sizeof(pid_request_list_temp)) == 0){log_e("pid_request_list failled to save");}
  if(myPreferences.putBytes(log_start_reason_name, &trip_start_condition, sizeof(trip_start_condition)) == 0){log_e("trip_start_condition failled to save");}
  myPreferences.end();

  log_i("\e[0;32m Default Settings saved");
}

void save_settings(){
  myPreferences.begin("default", false);
  myPreferences.putBool(settings_v2, true);
  for(int i = 0; i < 4; i++){
    char key[16];
    pref_key(key, sizeof(key), "w_ssid_", i);
    myPreferences.putString(key, Wifi_credentials[i].WiFi_Name);
    pref_key(key, sizeof(key), "w_pass_", i);
    myPreferences.putString(key, Wifi_credentials[i].WiFi_Pass);
    pref_key(key, sizeof(key), "w_en_", i);
    myPreferences.putBool(key, Wifi_credentials[i].enabled);
  }
  myPreferences.putString("nr_user", nodeRed_credentials.Node_User);
  myPreferences.putString("nr_pass", nodeRed_credentials.Node_Pass);
  myPreferences.putString("nr_url", nodeRed_credentials.Node_URL);
  if(myPreferences.putBytes(pid_val, &pid_request_list, sizeof(pid_request_list)) == 0){log_e("pid_request_list failled to save");}
  if(myPreferences.putBytes(log_start_reason_name, &trip_start_condition, sizeof(trip_start_condition)) == 0){log_e("pid_request_list failled to save");}
  myPreferences.end();

  log_i("\e[0;32m Settings saved");
}

void load_settings(){
  myPreferences.begin("default", false);
  bool v2 = myPreferences.getBool(settings_v2, false);
  if(!v2){
    myPreferences.end();
    default_settings();
    return;
  }

  set_default_credentials();
  for(int i = 0; i < 4; i++){
    char key[16];
    pref_key(key, sizeof(key), "w_ssid_", i);
    Wifi_credentials[i].WiFi_Name = myPreferences.getString(key, Wifi_credentials[i].WiFi_Name);
    pref_key(key, sizeof(key), "w_pass_", i);
    Wifi_credentials[i].WiFi_Pass = myPreferences.getString(key, Wifi_credentials[i].WiFi_Pass);
    pref_key(key, sizeof(key), "w_en_", i);
    Wifi_credentials[i].enabled = myPreferences.getBool(key, Wifi_credentials[i].enabled);
  }
  nodeRed_credentials.Node_User = myPreferences.getString("nr_user", nodeRed_credentials.Node_User);
  nodeRed_credentials.Node_Pass = myPreferences.getString("nr_pass", nodeRed_credentials.Node_Pass);
  nodeRed_credentials.Node_URL = myPreferences.getString("nr_url", nodeRed_credentials.Node_URL);
  if(myPreferences.getBytes(pid_val, &pid_request_list, sizeof(pid_request_list)) == 0 ){log_e("pid_request_list failled to load");}
  if(myPreferences.getBytes(log_start_reason_name, &trip_start_condition, sizeof(trip_start_condition)) == 0 ){log_e("pid_request_list failled to load");}

  myPreferences.end();
  log_i("\e[0;32m settings loaded");
}

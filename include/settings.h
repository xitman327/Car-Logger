
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


void save_settings(){
  myPreferences.begin("default", false);
  if(myPreferences.putBytes(wifi_creds, Wifi_credentials, 4*sizeof(wifi_preferenses_t)) == 0 ){log_e("WiFi credentials failled to save");}
  if(myPreferences.putBytes(node_creds, &nodeRed_credentials, sizeof(nodered_preferences_t))== 0 ){log_e("NodeRed credentials failled to save");}
  if(myPreferences.putBytes(pid_val, &pid_request_list, sizeof(pid_request_list)) == 0){log_e("pid_request_list failled to save");}
  myPreferences.end();

  log_i("\e[0;32m Settings saved");
}

void load_settings(){
  myPreferences.begin("default", false);
  size_t pref_len = myPreferences.getBytesLength(wifi_creds);
  if(myPreferences.getBytes(wifi_creds, Wifi_credentials, 4*sizeof(wifi_preferenses_t)) == 0 ){log_e("WiFi credentials failled to load");}
  pref_len = myPreferences.getBytesLength(node_creds);
  if(myPreferences.getBytes(node_creds, &nodeRed_credentials, sizeof(nodered_preferences_t)) == 0 ){log_e("NodeRed credentials failled to load");}
  pref_len = myPreferences.getBytesLength(pid_val);
  if(myPreferences.getBytes(pid_val, &pid_request_list, sizeof(pid_request_list)) == 0 ){log_e("pid_request_list failled to load");}

  log_i("\e[0;32m settings loaded");
}
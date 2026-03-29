#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESP32Time.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Forward declarations for state defined in main.cpp / obd.h
extern ESP32Time rtc;
extern bool gps_location_valid;
extern uint8_t last_sat_count;
extern float gps_speed_kmph;
extern bool gps_speed_valid;
extern bool log_started;
extern float trip_distance_km;
extern uint32_t trip_locations_count;
extern JsonDocument single_trip_data;
extern bool engine_on;
extern float rpmn;
extern float kmph;
extern float engine_temp;
extern float fuel_level;
extern float battery_voltage;
extern bool lpg_likely;
extern bool upload_in_progress;
extern int current_upload_file_index;
extern int num_of_files;
extern String ELMprotocol;
extern int wifi_signal_percent();
extern wifi_preferences_t Wifi_credentials[4];
extern nodered_preferences_t nodeRed_credentials;
extern byte trip_start_condition;
extern byte temp_trip_start_condition;

enum UploadStage : uint8_t;
extern UploadStage upload_stage;
extern const char *uploadStageName(UploadStage stage);

namespace
{
  NimBLEServer *pServer = nullptr;

  NimBLECharacteristic *chrMetrics1 = nullptr;
  NimBLECharacteristic *chrMetrics2 = nullptr;
  NimBLECharacteristic *pid_rq_list_a = nullptr;
  NimBLECharacteristic *pid_rq_list_b = nullptr;
  NimBLECharacteristic *chrWifi1Ssid = nullptr;
  NimBLECharacteristic *chrWifi1Pass = nullptr;
  NimBLECharacteristic *chrWifi1Enabled = nullptr;
  NimBLECharacteristic *chrWifi2Ssid = nullptr;
  NimBLECharacteristic *chrWifi2Pass = nullptr;
  NimBLECharacteristic *chrWifi2Enabled = nullptr;
  NimBLECharacteristic *chrWifi3Ssid = nullptr;
  NimBLECharacteristic *chrWifi3Pass = nullptr;
  NimBLECharacteristic *chrWifi3Enabled = nullptr;
  NimBLECharacteristic *chrWifi4Ssid = nullptr;
  NimBLECharacteristic *chrWifi4Pass = nullptr;
  NimBLECharacteristic *chrWifi4Enabled = nullptr;
  NimBLECharacteristic *chrNodeUrl = nullptr;
  NimBLECharacteristic *chrNodeUser = nullptr;
  NimBLECharacteristic *chrNodePass = nullptr;
  NimBLECharacteristic *chrTripStartCondition = nullptr;
  //   NimBLECharacteristic* chrLegacyLed = nullptr;  // backward-compat with tutorial example

  bool deviceConnected = false;
  bool oldDeviceConnected = false;
  uint32_t lastBleUpdateMs = 0;
  constexpr uint32_t kBleUpdateIntervalMs = 200;

  NimBLECharacteristic *createReadCharacteristic(NimBLEService *service, const char *uuid)
  {
    auto *chr = service->createCharacteristic(
        uuid,
        NIMBLE_PROPERTY::READ);
    return chr;
  }

  NimBLECharacteristic *createReadWriteCharacteristic(NimBLEService *service, const char *uuid)
  {
    auto *chr = service->createCharacteristic(
        uuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    return chr;
  }
  NimBLECharacteristic *createReadWriteNotifyCharacteristic(NimBLEService *service, const char *uuid)
  {
    auto *chr = service->createCharacteristic(
        uuid,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    return chr;
  }

  void setValue(NimBLECharacteristic *chr, const String &value)
  {
    if (!chr)
      return;
    // Force a stable copy into the characteristic to avoid dangling pointers
    std::string val(value.c_str());
    chr->setValue(val);
  }

  class MyServerCallbacks : public NimBLEServerCallbacks
  {
    void onConnect(NimBLEServer *server, ble_gap_conn_desc* desc) override
    {
      deviceConnected = true;
      server->updateConnParams(desc->conn_handle, 6, 12, 0, 100); // 7.5ms - 15ms interval
      Serial.println("BLE client connected");
    }

    void onDisconnect(NimBLEServer *server) override
    {
      deviceConnected = false;
      Serial.println("BLE client disconnected");
    }
  };

  class Chunk0Callbacks : public NimBLECharacteristicCallbacks
  {
    void onRead(NimBLECharacteristic *ch) override
    {
      ch->setValue(pid_request_list, 20);
    }
    void onWrite(NimBLECharacteristic *ch) override
    {
      const std::string &v = ch->getValue();
      if (v.size() == 20)
      {
        log_need_restart = 1;
        memcpy(&pid_request_list[0], v.data(), 20);
        save_settings();
        ch->notify();
      }
    }
  };

  class Chunk1Callbacks : public NimBLECharacteristicCallbacks
  {
    void onRead(NimBLECharacteristic *ch) override
    {
      ch->setValue(&pid_request_list[20], 20);
    }
    void onWrite(NimBLECharacteristic *ch) override
    {
      const std::string &v = ch->getValue();
      if (v.size() == 20)
      {
        log_need_restart = 1;
        memcpy(&pid_request_list[20], v.data(), 20);
        save_settings();
        ch->notify();
      }
    }
  };

  bool parseBoolValue(const std::string &value, bool fallback)
  {
    if (value.empty())
      return fallback;
    char c = value[0];
    if (c == '1' || c == 't' || c == 'T' || c == 'y' || c == 'Y')
      return true;
    if (c == '0' || c == 'f' || c == 'F' || c == 'n' || c == 'N')
      return false;
    return fallback;
  }

  class StringPreferenceCallbacks : public NimBLECharacteristicCallbacks
  {
  public:
    explicit StringPreferenceCallbacks(String *target, size_t max_len = 0)
        : target_(target), max_len_(max_len) {}

    void onRead(NimBLECharacteristic *ch) override
    {
      if (!target_)
        return;
      setValue(ch, *target_);
    }

    void onWrite(NimBLECharacteristic *ch) override
    {
      if (!target_)
        return;
      const std::string &v = ch->getValue();
      String next(v.c_str());
      if (max_len_ > 0 && next.length() > max_len_)
      {
        next = next.substring(0, max_len_);
      }
      *target_ = next;
      save_settings();
    }

  private:
    String *target_;
    size_t max_len_;
  };

  class BoolPreferenceCallbacks : public NimBLECharacteristicCallbacks
  {
  public:
    explicit BoolPreferenceCallbacks(bool *target) : target_(target) {}

    void onRead(NimBLECharacteristic *ch) override
    {
      if (!target_)
        return;
      setValue(ch, *target_ ? "1" : "0");
    }

    void onWrite(NimBLECharacteristic *ch) override
    {
      if (!target_)
        return;
      const std::string &v = ch->getValue();
      *target_ = parseBoolValue(v, *target_);
      save_settings();
    }

  private:
    bool *target_;
  };

  class TripStartConditionCallbacks : public NimBLECharacteristicCallbacks
  {
  public:
    explicit TripStartConditionCallbacks(byte *target)
        : target_(target) {}

    void onRead(NimBLECharacteristic *ch) override
    {
      if (!target_)
        return;
      setValue(ch, String(*target_));
    }

    void onWrite(NimBLECharacteristic *ch) override
    {
      if (!target_)
        return;
      const std::string &v = ch->getValue();
      int next = atoi(v.c_str());
      if (next < 0)
        next = 0;
      if (next > 6)
        next = 6;
      *target_ = static_cast<byte>(next);
      save_settings();
    }

  private:
    byte *target_;
  };
} // namespace

void updateAllCharacteristics()
{
  // Metrics part 1
  JsonDocument metrics1_doc;
  metrics1_doc["time"] = rtc.getEpoch();

  JsonObject gps = metrics1_doc.createNestedObject("gps");
  gps["fix"] = gps_location_valid;
  gps["sats"] = last_sat_count;
  gps["speed"] = round(gps_speed_kmph * 10) / 10.0;
  char gps_loc[50];
  sprintf(gps_loc, "%.6f,%.6f", fix_lat, fix_lng);
  gps["pos"] = gps_loc;

  JsonObject wifi = metrics1_doc.createNestedObject("wifi");
  wifi["status"] = WiFi.isConnected();
  wifi["ip"] = WiFi.isConnected() ? WiFi.localIP().toString() : "0.0.0.0";
  wifi["signal"] = wifi_signal_percent();

  metrics1_doc["obd_protocol"] = ELMprotocol;

  JsonObject upload = metrics1_doc.createNestedObject("upload");
  upload["stage"] = uploadStageName(upload_stage);
  upload["in_progress"] = upload_in_progress;
  upload["current_idx"] = current_upload_file_index;
  upload["files"] = num_of_files;

  JsonObject log = metrics1_doc.createNestedObject("log");
  log["started"] = log_started;
  log["trip_dist"] = round(trip_distance_km * 100) / 100.0;
  log["points"] = trip_locations_count;
  log["start_ts"] = single_trip_data["start_timestamp"] | 0L;

  String metrics1_str;
  serializeJson(metrics1_doc, metrics1_str);
  setValue(chrMetrics1, metrics1_str);

  // Metrics part 2
  JsonDocument metrics2_doc;
  JsonObject car = metrics2_doc.createNestedObject("car");
  car["eng_on"] = engine_on;
  car["rpm"] = round(rpmn);
  car["kmph"] = round(kmph * 10) / 10.0;
  car["gps_kmph"] = round(gps_speed_kmph * 10) / 10.0;
  car["temp"] = round(engine_temp * 10) / 10.0;
  car["fuel"] = round(fuel_level * 10) / 10.0;
  car["batt"] = round(battery_voltage * 100) / 100.0;
  car["vin"] = round(vin * 100) / 100.0;
  car["lpg"] = lpg_likely;
  

  JsonObject ram = metrics2_doc.createNestedObject("ram");
  size_t total_heap = ESP.getHeapSize();
  size_t free_heap = ESP.getFreeHeap();
  ram["free_kb"] = round(free_heap / 102.4) / 10.0;
  ram["total_kb"] = round(total_heap / 102.4) / 10.0;
  ram["used_pct"] = total_heap ? static_cast<uint8_t>(((total_heap - free_heap) * 100) / total_heap) : 0;

  String metrics2_str;
  serializeJson(metrics2_doc, metrics2_str);
  setValue(chrMetrics2, metrics2_str);

  pid_rq_list_a->setValue(pid_request_list, 20);
  pid_rq_list_b->setValue(&pid_request_list[20], 20);

  setValue(chrWifi1Ssid, Wifi_credentials[0].WiFi_Name);
  setValue(chrWifi1Pass, Wifi_credentials[0].WiFi_Pass);
  setValue(chrWifi1Enabled, Wifi_credentials[0].enabled ? "1" : "0");
  setValue(chrWifi2Ssid, Wifi_credentials[1].WiFi_Name);
  setValue(chrWifi2Pass, Wifi_credentials[1].WiFi_Pass);
  setValue(chrWifi2Enabled, Wifi_credentials[1].enabled ? "1" : "0");
  setValue(chrWifi3Ssid, Wifi_credentials[2].WiFi_Name);
  setValue(chrWifi3Pass, Wifi_credentials[2].WiFi_Pass);
  setValue(chrWifi3Enabled, Wifi_credentials[2].enabled ? "1" : "0");
  setValue(chrWifi4Ssid, Wifi_credentials[3].WiFi_Name);
  setValue(chrWifi4Pass, Wifi_credentials[3].WiFi_Pass);
  setValue(chrWifi4Enabled, Wifi_credentials[3].enabled ? "1" : "0");
  setValue(chrNodeUrl, nodeRed_credentials.Node_URL);
  setValue(chrNodeUser, nodeRed_credentials.Node_User);
  setValue(chrNodePass, nodeRed_credentials.Node_Pass);
  setValue(chrTripStartCondition, String(trip_start_condition));
}

#define SERVICE_UUID "19b10000-e8f2-537e-4f6c-d104768a1214"

inline void ble_setup()
{

  
  String ble_name = "Car-Logger_" + WiFi.macAddress();
  NimBLEDevice::init(ble_name.c_str());
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Increase signal strength for faster discovery
  //   NimBLEDevice::setSecurityAuth(false, false, false);
  //   NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setMTU(256);  // Increased MTU reduces fragmentation for large JSON metrics

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  chrMetrics1 = createReadCharacteristic(pService, "19b10001-e8f2-537e-4f6c-d104768a1214");
  chrMetrics2 = createReadCharacteristic(pService, "19b10002-e8f2-537e-4f6c-d104768a1214");

  pid_rq_list_a = createReadWriteNotifyCharacteristic(pService, "19b1001d-e8f2-537e-4f6c-d104768a1214");
  pid_rq_list_b = createReadWriteNotifyCharacteristic(pService, "19b1001e-e8f2-537e-4f6c-d104768a1214");

  pid_rq_list_a->setCallbacks(new Chunk0Callbacks());
  pid_rq_list_b->setCallbacks(new Chunk1Callbacks());

  chrWifi1Ssid = createReadWriteCharacteristic(pService, "19b1001f-e8f2-537e-4f6c-d104768a1214");
  chrWifi1Pass = createReadWriteCharacteristic(pService, "19b10020-e8f2-537e-4f6c-d104768a1214");
  chrWifi1Enabled = createReadWriteCharacteristic(pService, "19b10021-e8f2-537e-4f6c-d104768a1214");
  chrWifi2Ssid = createReadWriteCharacteristic(pService, "19b10022-e8f2-537e-4f6c-d104768a1214");
  chrWifi2Pass = createReadWriteCharacteristic(pService, "19b10023-e8f2-537e-4f6c-d104768a1214");
  chrWifi2Enabled = createReadWriteCharacteristic(pService, "19b10024-e8f2-537e-4f6c-d104768a1214");
  chrWifi3Ssid = createReadWriteCharacteristic(pService, "19b10025-e8f2-537e-4f6c-d104768a1214");
  chrWifi3Pass = createReadWriteCharacteristic(pService, "19b10026-e8f2-537e-4f6c-d104768a1214");
  chrWifi3Enabled = createReadWriteCharacteristic(pService, "19b10027-e8f2-537e-4f6c-d104768a1214");
  chrWifi4Ssid = createReadWriteCharacteristic(pService, "19b10028-e8f2-537e-4f6c-d104768a1214");
  chrWifi4Pass = createReadWriteCharacteristic(pService, "19b10029-e8f2-537e-4f6c-d104768a1214");
  chrWifi4Enabled = createReadWriteCharacteristic(pService, "19b1002a-e8f2-537e-4f6c-d104768a1214");
  chrNodeUrl = createReadWriteCharacteristic(pService, "19b1002b-e8f2-537e-4f6c-d104768a1214");
  chrNodeUser = createReadWriteCharacteristic(pService, "19b1002c-e8f2-537e-4f6c-d104768a1214");
  chrNodePass = createReadWriteCharacteristic(pService, "19b1002d-e8f2-537e-4f6c-d104768a1214");
  chrTripStartCondition = createReadWriteCharacteristic(pService, "19b1002e-e8f2-537e-4f6c-d104768a1214");

  chrWifi1Ssid->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[0].WiFi_Name));
  chrWifi1Pass->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[0].WiFi_Pass));
  chrWifi1Enabled->setCallbacks(new BoolPreferenceCallbacks(&Wifi_credentials[0].enabled));
  chrWifi2Ssid->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[1].WiFi_Name));
  chrWifi2Pass->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[1].WiFi_Pass));
  chrWifi2Enabled->setCallbacks(new BoolPreferenceCallbacks(&Wifi_credentials[1].enabled));
  chrWifi3Ssid->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[2].WiFi_Name));
  chrWifi3Pass->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[2].WiFi_Pass));
  chrWifi3Enabled->setCallbacks(new BoolPreferenceCallbacks(&Wifi_credentials[2].enabled));
  chrWifi4Ssid->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[3].WiFi_Name));
  chrWifi4Pass->setCallbacks(new StringPreferenceCallbacks(&Wifi_credentials[3].WiFi_Pass));
  chrWifi4Enabled->setCallbacks(new BoolPreferenceCallbacks(&Wifi_credentials[3].enabled));
  chrNodeUrl->setCallbacks(new StringPreferenceCallbacks(&nodeRed_credentials.Node_URL));
  chrNodeUser->setCallbacks(new StringPreferenceCallbacks(&nodeRed_credentials.Node_User));
  chrNodePass->setCallbacks(new StringPreferenceCallbacks(&nodeRed_credentials.Node_Pass));
  chrTripStartCondition->setCallbacks(new TripStartConditionCallbacks(&trip_start_condition));

  // Keep the tutorial LED characteristic around so clients expecting it can still connect.
  //   chrLegacyLed = createReadWriteCharacteristic(pService, "beb5483e-36e1-4688-b7f5-ea07361b26a8");

  pService->start();
  updateAllCharacteristics(); // ensure fresh values are available immediately

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setMinInterval(32); // 20ms advertising interval
  pAdvertising->setMaxInterval(64); // 40ms advertising interval
  // Hint to the client to use fast connection parameters
  pAdvertising->setMinPreferred(0x06); // 7.5ms
  pAdvertising->setMaxPreferred(0x12); // 22.5ms
  pAdvertising->setScanResponse(true);
  NimBLEDevice::startAdvertising();
}

inline void ble_loop()
{
  uint32_t now = millis();
  if (now - lastBleUpdateMs >= kBleUpdateIntervalMs)
  {
    lastBleUpdateMs = now;
    if(deviceConnected){
      updateAllCharacteristics();
    }
    
  }

  if (!deviceConnected && oldDeviceConnected)
  {
    // delay(500);
    if (!(pServer->getAdvertising()->isAdvertising()))
    {
      pServer->startAdvertising();
    }
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected)
  {
    oldDeviceConnected = deviceConnected;
  }
}

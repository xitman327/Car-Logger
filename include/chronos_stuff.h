#include <ChronosESP32.h>

ChronosESP32 chrono;
bool change = false;
uint32_t nav_crc = 0xFFFFFFFF;

void connectionCallback(bool state)
{
  Serial.print("Connection state: ");
  Serial.println(state ? "Connected" : "Disconnected");
  // bool connected = watch.isConnected();
}

void notificationCallback(Notification notification)
{
  Serial.print("Notification received at ");
  Serial.println(notification.time);
  Serial.print("From: ");
  Serial.print(notification.app);
  Serial.print("\tIcon: ");
  Serial.println(notification.icon);
  Serial.println(notification.title);
  Serial.println(notification.message);
  // see loop on how to access notifications
}

void ringerCallback(String caller, bool state)
{
  if (state)
  {
    Serial.print("Ringer: Incoming call from ");
    Serial.println(caller);
  }
  else
  {
    Serial.println("Ringer dismissed");
  }
}

void dataCallback(uint8_t *data, int length)
{
  Serial.println("Received Data");
  for (int i = 0; i < length; i++)
  {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}

void configCallback(Config config, uint32_t a, uint32_t b)
{
    switch (config)
    {
    case CF_NAV_DATA:
        Serial.print("Navigation state: ");
        Serial.println(a ? "Active" : "Inactive");
        // change = true;
        if (a)
        {
            Navigation nav = chrono.getNavigation();
            Serial.println(nav.directions);
            Serial.println(nav.eta);
            Serial.println(nav.duration);
            Serial.println(nav.distance);
            Serial.println(nav.title);
            Serial.println(nav.speed);
        }else{
          //clear icon
          fillRect_helper(48, 48, 10, 10, TFT_BLACK);
        }
        break;
    case CF_NAV_ICON:
        Serial.print("Navigation Icon data, position: ");
        Serial.println(a);
        Serial.print("Icon CRC: ");
        Serial.printf("0x%04X\n", b);
        if (a == 2){
            Navigation nav = chrono.getNavigation();
            if (nav_crc != nav.iconCRC)
            {
                nav_crc = nav.iconCRC;

                drawBitmap_helper(nav.icon, 48, 48, 10, 10, TFT_WHITE, TFT_BLACK);
            }
        }
        break;
    }
}


void init_chronos(){
    chrono.setName("ESP32S3_Chronos");

    chrono.setConnectionCallback(connectionCallback);
    chrono.setRingerCallback(ringerCallback);
    chrono.setDataCallback(dataCallback);
    chrono.setNotificationCallback(notificationCallback);
    chrono.setConfigurationCallback(configCallback);

    chrono.begin();
    chrono.setBattery(55, false);
}

void loop_chronos(){
    chrono.loop();
}
#include <RadioLib.h>
#include "mbedtls/aes.h"
#include <time.h>
#include <WiFi.h>

// ---------------- LoRa pins ----------------

// #define MOSI      35
// #define MISO      36
// #define SCK       37

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

// ---------------- Device config ----------------
#define DEVICE_ID 1

// ---------------- WiFi (for NTP) ----------------
// const char* ssid = "YOUR_WIFI";
// const char* password = "YOUR_PASS";

// ---------------- AES KEY ----------------
const uint8_t AES_KEY[16] = {
  0x10, 0x22, 0x33, 0x44,
  0x55, 0x66, 0x77, 0x88,
  0x99, 0xaa, 0xbb, 0xcc,
  0xdd, 0xee, 0xff, 0x01
};

// ---------------- AES CTR encrypt ----------------
void aesEncryptCTR(uint8_t* input, uint8_t* output, size_t len, uint8_t* iv) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, AES_KEY, 128);

  size_t nc_off = 0;
  uint8_t stream_block[16];

  mbedtls_aes_crypt_ctr(&aes, len, &nc_off, iv, stream_block, input, output);

  mbedtls_aes_free(&aes);
}

// ---------------- Generate IV ----------------
void generateIV(uint8_t* iv) {
  for (int i = 0; i < 16; i++) {
    iv[i] = random(0, 256);
  }
}

// ---------------- Setup ----------------
void setup_lora() {
//   Serial.begin(115200);
//   delay(2000);

  log_i("Starting LoRa sensor...");

  // --- WiFi for NTP ---
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     delay(500);
//   }
//   Serial.println(" connected");

  // --- NTP ---
//   configTime(0, 0, "pool.ntp.org", "time.nist.gov");

//   Serial.print("Waiting for time sync");
//   while (time(nullptr) < 100000) {
//     Serial.print(".");
//     delay(500);
//   }
//   Serial.println(" OK");

    if(WiFi.isConnected()){
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
        tzset();
        struct tm timeinfo;
        if(getLocalTime(&timeinfo)){
            Serial.print("Local Time: ");Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        }else{
            log_e("cannot get time");
        }
    }else{
        log_e("Wifi con connected for ntp time sync");
    }
  

  // --- Init LoRa ---
  int state = radio.begin(868.0);
  if (state != RADIOLIB_ERR_NONE) {
    log_e("Lora failled %d", state);
    while (true);
  }

  radio.setBandwidth(125.0);
  radio.setSpreadingFactor(9);
  radio.setCodingRate(5);
  radio.setOutputPower(14);

  log_i("LoRa ready");
}
#define lora_loop_ms 2000
uint32_t tm_lora;
uint32_t tm_lora_random;

struct Payload {
  uint64_t ts;
  double temp;   // *100
  double hum;
  double vbat;
  uint8_t flags;
};

// ---------------- Loop ----------------
void loop_lora() {

    if(millis() - tm_lora > lora_loop_ms + tm_lora_random){
        tm_lora = millis();
        tm_lora_random = random(0, 10000); // randomize the inderval that we sent to the gateway (for security !) )
    
        // radio.transmit((uint8_t*)"TEST123", 7);

        // -------- Timestamp --------
        uint64_t ts = time(nullptr);

        // -------- Fake sensor data --------
        float temp = 20.0 + (random(-50, 50) / 10.0);

        // -------- Build JSON --------
        // char payload[80];
        // snprintf(payload, sizeof(payload),
        //         "{\"ts\":%llu,\"temp\":%.1f}",
        //         ts, temp);

        Payload p;
        p.ts = time(nullptr);
        p.temp = 22.45;
        p.hum = 55;
        p.vbat = 92;   // 3.92V → scaled
        p.flags = 0;

        size_t len = sizeof(p);

        // -------- Generate IV --------
        uint8_t iv[16];
        generateIV(iv);

        uint8_t iv_copy[16];
        memcpy(iv_copy, iv, 16);
        // -------- Encrypt --------
        uint8_t ciphertext[128];
        aesEncryptCTR((uint8_t*)&p, ciphertext, len, iv_copy);

        // -------- Build packet --------
        uint8_t packet[256];
        size_t index = 0;

        // DEV ID
        packet[index++] = DEVICE_ID;

        // IV
        memcpy(&packet[index], iv, 16);
        index += 16;

        // CIPHERTEXT
        memcpy(&packet[index], ciphertext, len);
        index += len;

        // -------- Send --------
        int state = radio.transmit(packet, index);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("Packet sent");
            Serial.print("size: ");
            Serial.println(len);
        } else {
            Serial.print("Send failed: ");
            Serial.println(state);
        }
    }
  // -------- Random delay (important) --------
//   delay(30000 + random(0, 30000)); // 30–60 sec
}
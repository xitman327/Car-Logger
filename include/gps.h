
#define gpsSerial Serial2

#include <TinyGPSPlus.h>

TinyGPSPlus gps;

struct BaseLocation
{
  bool valid = false;
  int32_t lat_e7 = 0;
  int32_t lon_e7 = 0;
};

struct BaseDetectorConfig
{
  // Your requested values:
  uint32_t stationaryTimeMs = 10UL * 60UL * 1000UL; // 10 minutes
  float stationaryRadiusM = 80.0f;                  // set 50..100 as you like

  // Hysteresis for leave detection (recommend larger than stationaryRadiusM):
  float atBaseRadiusM = 120.0f;
  float leaveRadiusM = 250.0f;

  // Fix quality gates (optional; used only if fields are valid/enabled in NeoGPS config)
  float maxHDOP = 3.0f;      // relax vs strict (tunnels/urban)
  uint8_t minSatellites = 5; // used only if fix.valid.satellites

  // Detect "serial stuck" (no new sentences read)
  uint32_t serialStuckMs = 8000; // if you call every 0.5-5s, 8s is a reasonable default

  // Detect "time stuck" (sentences arrive but GPS time doesn't advance)
  uint32_t timeStuckMs = 15000;

  // Allow short outages without resetting the stationary candidate anchor
  uint32_t maxPauseBeforeResetMs = 3UL * 60UL * 1000UL; // if paused too long, restart candidate when GOOD returns
};

// ---- Helpers ----
static inline float deg2rad(float d) { return d * 0.01745329251994329577f; }

static float distanceMetersE7(int32_t lat1_e7, int32_t lon1_e7,
                              int32_t lat2_e7, int32_t lon2_e7)
{
  const float lat1 = lat1_e7 / 1e7f;
  const float lon1 = lon1_e7 / 1e7f;
  const float lat2 = lat2_e7 / 1e7f;
  const float lon2 = lon2_e7 / 1e7f;

  const float R = 6371000.0f;
  const float dLat = deg2rad(lat2 - lat1);
  const float dLon = deg2rad(lon2 - lon1);

  const float a =
      sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
      cosf(deg2rad(lat1)) * cosf(deg2rad(lat2)) *
          sinf(dLon * 0.5f) * sinf(dLon * 0.5f);

  return R * 2.0f * atanf(sqrtf(a) / sqrtf(1.0f - a));
}

// Compare NeoGPS clock to detect "time advancing".
// Uses fix.dateTime if enabled & valid; otherwise returns false.
static bool gpsDateTimeToSeconds(TinyGPSPlus &gps, uint32_t &outSeconds)
{
  if (!gps.date.isValid() || !gps.time.isValid())
    return false;

  // TinyGPSTime sod = gps.time;
  const uint32_t sod = gps.time.value();
      // gps.time.hour() * 3600UL +
      // gps.time.minute() * 60UL +
      // gps.time.second();
  // TinyGPSDate ymd = gps.date;
  const uint32_t ymd = gps.date.value();
      // gps.date.year() * 10000UL +
      // gps.date.month() * 100UL +
      // gps.date.day();

  outSeconds = ymd * 86400UL + sod;
  return true;
}

class BaseDetector
{
public:
  BaseDetectorConfig cfg;
  BaseLocation base;

  // Events (latched true for one update cycle)
  bool baseJustSet = false;
  bool leftBaseEvent = false;
  bool arrivedBaseEvent = false;

  // Status you may want for diagnostics
  bool gpsSerialStuck = false;
  bool gpsTimeStuck = false;
  bool fixGood = false;

  void resetBase()
  {
    base = BaseLocation{};
    isAtBase = false;
    resetCandidate();
  }

  // Call this whenever you have access to the GPS parser.
  // It drains all available sentences and remembers the newest fix.
  // template <typename StreamT>
  int feedGPS(TinyGPSPlus &gps, Stream &gpsPort)
  {
    baseJustSet = leftBaseEvent = arrivedBaseEvent = false;

    bool gotAny = false;
    while (gpsPort.available())
    {
      if (gps.encode(gpsPort.read()))
      {
        gotAny = true;
        lastSentenceMs = millis();

        uint32_t s;
        if (gpsDateTimeToSeconds(gps, s))
        {
          if (!haveGpsSeconds || s != lastGpsSeconds)
          {
            haveGpsSeconds = true;
            lastGpsSeconds = s;
            lastGpsTimeAdvanceMs = millis();
          }
        }
      }
    }

    if (gotAny)
      haveFix = true;
    return gotAny ? 1 : 0;
  }

  // Call this at your cadence (500 ms to 5 s).
  // Returns true once when base gets set.
  bool updateStationaryAndMaybeSetBase(TinyGPSPlus &gps)
  {
    baseJustSet = false;
    const uint32_t now = millis();

    gpsSerialStuck = (now - lastSentenceMs) > cfg.serialStuckMs;
    gpsTimeStuck = haveGpsSeconds && (now - lastGpsTimeAdvanceMs) > cfg.timeStuckMs;

    fixGood = isFixGood(gps, now);
    if (base.valid)
      return false;

    if (!fixGood)
    {
      if (candidateValid && pauseStartMs == 0)
        pauseStartMs = now;
      return false;
    }

    if (pauseStartMs)
    {
      if ((now - pauseStartMs) > cfg.maxPauseBeforeResetMs)
        resetCandidate();
      pauseStartMs = 0;
    }

    int32_t lat_e7 = gps.location.lat() * 1e7;
    int32_t lon_e7 = gps.location.lng() * 1e7;

    if (!candidateValid)
    {
      candidateValid = true;
      candLat_e7 = lat_e7;
      candLon_e7 = lon_e7;
      accumulatedGoodMs = 0;
      lastGoodMs = now;
      return false;
    }

    float d = distanceMetersE7(candLat_e7, candLon_e7, lat_e7, lon_e7);

    if (d <= cfg.stationaryRadiusM)
    {
      uint32_t delta = now - lastGoodMs;
      lastGoodMs = now;
      if (delta > 6000)
        delta = 6000;
      accumulatedGoodMs += delta;

      if (accumulatedGoodMs >= cfg.stationaryTimeMs)
      {
        base.valid = true;
        base.lat_e7 = candLat_e7;
        base.lon_e7 = candLon_e7;
        baseJustSet = true;
        resetCandidate();
        return true;
      }
    }
    else
    {
      candLat_e7 = lat_e7;
      candLon_e7 = lon_e7;
      accumulatedGoodMs = 0;
      lastGoodMs = now;
    }

    return false;
  }

  // Returns true once when "left base" triggers.
  bool updateLeaveBase(TinyGPSPlus &gps)
  {
    leftBaseEvent = arrivedBaseEvent = false;
    if (!base.valid)
      return false;

    const uint32_t now = millis();
    gpsSerialStuck = (now - lastSentenceMs) > cfg.serialStuckMs;
    gpsTimeStuck = haveGpsSeconds && (now - lastGpsTimeAdvanceMs) > cfg.timeStuckMs;

    if (!isFixGood(gps, now))
      return false;

    int32_t lat_e7 = gps.location.lat() * 1e7;
    int32_t lon_e7 = gps.location.lng() * 1e7;

    float d = distanceMetersE7(base.lat_e7, base.lon_e7, lat_e7, lon_e7);

    if (!isAtBase)
    {
      if (d <= cfg.atBaseRadiusM)
      {
        isAtBase = true;
        arrivedBaseEvent = true;
      }
    }
    else
    {
      if (d >= cfg.leaveRadiusM)
      {
        isAtBase = false;
        leftBaseEvent = true;
        return true;
      }
    }
    return false;
  }

  // Optional getters for debugging
  uint32_t getAccumulatedGoodMs() const { return accumulatedGoodMs; }
  bool hasCandidate() const { return candidateValid; }

private:
  bool haveFix = false;

  // Sentence/time tracking
  uint32_t lastSentenceMs = 0;

  bool haveGpsSeconds = false;
  uint32_t lastGpsSeconds = 0;
  uint32_t lastGpsTimeAdvanceMs = 0;

  // Stationary candidate
  bool candidateValid = false;
  int32_t candLat_e7 = 0;
  int32_t candLon_e7 = 0;

  uint32_t accumulatedGoodMs = 0; // counts only while fixGood==true and within radius
  uint32_t lastGoodMs = 0;

  uint32_t pauseStartMs = 0;

  // Base presence
  bool isAtBase = false;

  void resetCandidate()
  {
    candidateValid = false;
    accumulatedGoodMs = 0;
    lastGoodMs = 0;
    pauseStartMs = 0;
  }

  bool isFixGood(TinyGPSPlus &gps, uint32_t now)
  {
    if (!haveFix)
      return false;
    if (gpsSerialStuck)
      return false;
    if (gpsTimeStuck)
      return false;
    if (!gps.location.isValid())
      return false;

    if (gps.hdop.isValid())
    {
      float hdop = gps.hdop.value() / 100.0f;
      if (hdop > cfg.maxHDOP)
        return false;
    }

    if (gps.satellites.isValid())
    {
      if (gps.satellites.value() < cfg.minSatellites)
        return false;
    }

    return true;
  }
};

extern void correct_trip_time_if_needed();

void sync_rtc_from_gps(TinyGPSPlus &gps)
{
  if (gps.time.isValid() && gps.date.isValid())
  {
    rtc.setTime(
        gps.time.second(),
        gps.time.minute(),
        gps.time.hour(),
        gps.date.day(),
        gps.date.month(),
        gps.date.year());
    rtc.setTime(rtc.getEpoch() + timezone_offset_seconds);
    gps_time_synced = true;
    correct_trip_time_if_needed();
  }

  gps_speed_valid = gps.speed.isValid();
  gps_speed_kmph = gps_speed_valid ? gps.speed.kmph() : 0;

  gps_location_valid = gps.location.isValid();
  if (gps_location_valid)
  {
    fix_lat = gps.location.lat();
    fix_lng = gps.location.lng();
    last_gps_fix_time = rtc.getEpoch();
  }

  if (gps.satellites.isValid())
    last_sat_count = gps.satellites.value();
}

BaseDetector detector;

void setup_gps()
{

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  detector.cfg.stationaryRadiusM = 80.0f;            // 50..100
  detector.cfg.stationaryTimeMs  = 10UL * 60UL * 1000UL;

  detector.cfg.serialStuckMs = 8000;                // adjust to your GPS update rate
  detector.cfg.timeStuckMs   = 15000;

  detector.cfg.atBaseRadiusM = 120.0f;
  detector.cfg.leaveRadiusM  = 250.0f;

  detector.cfg.maxHDOP = 3.0f;
  detector.cfg.minSatellites = 5;
}

// test function
// void print_gps(Stream &port){
//   while(port.available()){
//     Serial.write(port.read());
//   }
// }

uint32_t last_gps, last_gps2;
void loop_gps()
{

  if (detector.feedGPS(gps, gpsSerial)) {
    uint32_t now = millis();
    last_gps = now - last_gps2;
    last_gps2 = now;

    digitalWrite(LEDA, !digitalRead(LEDA));

    sync_rtc_from_gps(gps);

    if (gps.location.isValid()) {
      fix_lat = gps.location.lat();
      fix_lng = gps.location.lng();
      gps_location_valid = true;
    } else {
      gps_location_valid = false;
    }
  }
}
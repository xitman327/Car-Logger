#define NMEAGPS_RECOGNIZE_ALL
#define GPS_FIX_HDOP
#include <NMEAGPS.h>
NMEAGPS  gps;
gps_fix  fix;
#define GPSfix gps_fix
HardwareSerial gpsSerial(2);


#include <NMEAGPS.h>
#include <GPSfix.h>

struct BaseLocation {
  bool     valid = false;
  int32_t  lat_e7 = 0;
  int32_t  lon_e7 = 0;
};

struct BaseDetectorConfig {
  // Your requested values:
  uint32_t stationaryTimeMs   = 10UL * 60UL * 1000UL; // 10 minutes
  float    stationaryRadiusM  = 80.0f;                // set 50..100 as you like

  // Hysteresis for leave detection (recommend larger than stationaryRadiusM):
  float    atBaseRadiusM      = 120.0f;
  float    leaveRadiusM       = 250.0f;

  // Fix quality gates (optional; used only if fields are valid/enabled in NeoGPS config)
  float    maxHDOP            = 3.0f;    // relax vs strict (tunnels/urban)
  uint8_t  minSatellites      = 5;       // used only if fix.valid.satellites

  // Detect "serial stuck" (no new sentences read)
  uint32_t serialStuckMs      = 8000;    // if you call every 0.5-5s, 8s is a reasonable default

  // Detect "time stuck" (sentences arrive but GPS time doesn't advance)
  uint32_t timeStuckMs        = 15000;

  // Allow short outages without resetting the stationary candidate anchor
  uint32_t maxPauseBeforeResetMs = 3UL * 60UL * 1000UL; // if paused too long, restart candidate when GOOD returns
};

// ---- Helpers ----
static inline float deg2rad(float d) { return d * 0.01745329251994329577f; }

static float distanceMetersE7(int32_t lat1_e7, int32_t lon1_e7, int32_t lat2_e7, int32_t lon2_e7) {
  const float lat1 = (float)lat1_e7 / 1e7f;
  const float lon1 = (float)lon1_e7 / 1e7f;
  const float lat2 = (float)lat2_e7 / 1e7f;
  const float lon2 = (float)lon2_e7 / 1e7f;

  const float R = 6371000.0f;
  const float dLat = deg2rad(lat2 - lat1);
  const float dLon = deg2rad(lon2 - lon1);

  const float a =
    sinf(dLat * 0.5f) * sinf(dLat * 0.5f) +
    cosf(deg2rad(lat1)) * cosf(deg2rad(lat2)) *
    sinf(dLon * 0.5f) * sinf(dLon * 0.5f);

  const float c = 2.0f * atanf(sqrtf(a) / sqrtf(1.0f - a));
  return R * c;
}

// Compare NeoGPS clock to detect "time advancing".
// Uses fix.dateTime if enabled & valid; otherwise returns false.
static bool gpsDateTimeToSeconds(const GPSfix &fix, uint32_t &outSeconds) {
  if (!(fix.valid.date && fix.valid.time)) return false;

  // NeoGPS dateTime has fields: year, month, day, hours, minutes, seconds
  // We do a simple monotonic-ish mapping to seconds-of-day plus day count approximation.
  // For "stuck detection" we only need to see it advance, not absolute epoch correctness.
  const uint32_t sod = (uint32_t)fix.dateTime.hours * 3600UL +
                       (uint32_t)fix.dateTime.minutes * 60UL +
                       (uint32_t)fix.dateTime.seconds;

  // Day stamp: YYYYMMDD into a number, then multiply by 86400 and add seconds-of-day.
  // This avoids implementing full epoch conversion.
  const uint32_t ymd = (uint32_t)fix.dateTime.year * 10000UL +
                       (uint32_t)fix.dateTime.month * 100UL +
                       (uint32_t)fix.dateTime.day;

  outSeconds = ymd * 86400UL + sod;
  return true;
}

class BaseDetector {
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

  void resetBase() { base = BaseLocation{}; isAtBase = false; resetCandidate(); }

  // Call this whenever you have access to the GPS parser.
  // It drains all available sentences and remembers the newest fix.
  template <typename StreamT>
  int feedGPS(NMEAGPS &gps, StreamT &gpsPort, GPSfix &fix) {
    baseJustSet = leftBaseEvent = arrivedBaseEvent = false;
    int val = 0;
    bool gotAny = false;
    while (gps.available(gpsPort)) {
      fix = gps.read();
      val = 1;
      lastFix = fix;
      gotAny = true;
      lastSentenceMs = millis();

      // Track whether GPS time advances (if date+time are enabled)
      uint32_t s;
      if (gpsDateTimeToSeconds(lastFix, s)) {
        if (haveGpsSeconds) {
          if (s != lastGpsSeconds) {
            lastGpsSeconds = s;
            lastGpsTimeAdvanceMs = millis();
          }
        } else {
          haveGpsSeconds = true;
          lastGpsSeconds = s;
          lastGpsTimeAdvanceMs = millis();
        }
      }
    }
    if (gotAny) haveFix = true;
    return val;
  }

  // Call this at your cadence (500 ms to 5 s).
  // Returns true once when base gets set.
  bool updateStationaryAndMaybeSetBase() {
    baseJustSet = false;
    const uint32_t now = millis();

    // Determine "serial stuck"
    gpsSerialStuck = (now - lastSentenceMs) > cfg.serialStuckMs;

    // Determine "time stuck"
    // If we have no GPS time fields, we cannot time-stuck-check; treat as not stuck.
    gpsTimeStuck = haveGpsSeconds && ((now - lastGpsTimeAdvanceMs) > cfg.timeStuckMs);

    // Determine if fix is GOOD enough to be trusted for stationary accumulation
    fixGood = isFixGood(lastFix, now);

    if (base.valid) {
      // once base exists, stationary detection is no-op
      return false;
    }

    // If we don't have good fix, PAUSE accumulation (do not reset immediately)
    if (!fixGood) {
      // Track how long we've been paused. If too long, we'll reset candidate on recovery.
      if (candidateValid && pauseStartMs == 0) pauseStartMs = now;
      return false;
    }

    // We have GOOD fix. If we were paused, decide whether to reset candidate.
    if (pauseStartMs != 0) {
      const uint32_t pausedFor = now - pauseStartMs;
      pauseStartMs = 0;
      if (pausedFor > cfg.maxPauseBeforeResetMs) {
        // Too long without trustworthy GPS; restart candidate to avoid false base.
        resetCandidate();
      }
    }

    int32_t lat_e7 = lastFix.latitudeL();
    int32_t lon_e7 = lastFix.longitudeL();

    if (!candidateValid) {
      // Start candidate anchor at first good fix
      candidateValid = true;
      candLat_e7 = lat_e7;
      candLon_e7 = lon_e7;
      accumulatedGoodMs = 0;
      lastGoodMs = now;
      return false;
    }

    const float d = distanceMetersE7(candLat_e7, candLon_e7, lat_e7, lon_e7);

    if (d <= cfg.stationaryRadiusM) {
      // Accumulate GOOD time (not wall time)
      const uint32_t delta = (lastGoodMs == 0) ? 0 : (now - lastGoodMs);
      lastGoodMs = now;

      // Cap delta so a long scheduling gap doesn't “jump” the timer incorrectly
      const uint32_t cappedDelta = (delta > 6000UL) ? 6000UL : delta; // cap at 6s
      accumulatedGoodMs += cappedDelta;

      if (accumulatedGoodMs >= cfg.stationaryTimeMs) {
        log_i("base set %d,%d", candLat_e7, candLon_e7);
        base.valid = true;
        base.lat_e7 = candLat_e7;
        base.lon_e7 = candLon_e7;
        baseJustSet = true;
        resetCandidate(); // optional; keeps state clean
        return true;
      }
    } else {
      // We are confidently away from candidate anchor with GOOD fix -> restart
      candLat_e7 = lat_e7;
      candLon_e7 = lon_e7;
      accumulatedGoodMs = 0;
      lastGoodMs = now;
    }

    return false;
  }

  // Returns true once when "left base" triggers.
  bool updateLeaveBase() {
    leftBaseEvent = arrivedBaseEvent = false;

    if (!base.valid) return false;

    // Use the same robust gating: if not GOOD, do not change at/left state.
    const uint32_t now = millis();
    gpsSerialStuck = (now - lastSentenceMs) > cfg.serialStuckMs;
    gpsTimeStuck   = haveGpsSeconds && ((now - lastGpsTimeAdvanceMs) > cfg.timeStuckMs);
    fixGood        = isFixGood(lastFix, now);
    if (!fixGood) return false;

    const int32_t lat_e7 = lastFix.latitudeL();
    const int32_t lon_e7 = lastFix.longitudeL();
    const float dBase = distanceMetersE7(base.lat_e7, base.lon_e7, lat_e7, lon_e7);

    if (!isAtBase) {
      if (dBase <= cfg.atBaseRadiusM) {
        isAtBase = true;
        arrivedBaseEvent = true;
      }
      return false;
    } else {
      if (dBase >= cfg.leaveRadiusM) {
        isAtBase = false;
        leftBaseEvent = true;
        return true;
      }
      return false;
    }
  }

  // Optional getters for debugging
  uint32_t getAccumulatedGoodMs() const { return accumulatedGoodMs; }
  bool hasCandidate() const { return candidateValid; }

private:
  GPSfix   lastFix;
  bool     haveFix = false;

  // Sentence/time tracking
  uint32_t lastSentenceMs = 0;

  bool     haveGpsSeconds = false;
  uint32_t lastGpsSeconds = 0;
  uint32_t lastGpsTimeAdvanceMs = 0;

  // Stationary candidate
  bool     candidateValid = false;
  int32_t  candLat_e7 = 0;
  int32_t  candLon_e7 = 0;

  uint32_t accumulatedGoodMs = 0; // counts only while fixGood==true and within radius
  uint32_t lastGoodMs = 0;

  uint32_t pauseStartMs = 0;

  // Base presence
  bool     isAtBase = false;

  void resetCandidate() {
    candidateValid = false;
    accumulatedGoodMs = 0;
    lastGoodMs = 0;
    pauseStartMs = 0;
  }

  bool isFixGood(const GPSfix &fix, uint32_t /*now*/) {
    if (!haveFix) return false;
    // Serial stuck => no new data, do not trust
    if (gpsSerialStuck) return false;

    // If GPS time is available and it's not advancing, treat as stuck
    if (gpsTimeStuck) return false;

    // Must have location
    if (!fix.valid.location) return false;

    // Optional quality gates if enabled in your NeoGPS config
    if (fix.valid.hdop) {
      // NeoGPS hdop is typically an integer scaled by 100 (e.g., 150 == 1.50)
      const float hdop = (float)fix.hdop / 100.0f;
      if (hdop > cfg.maxHDOP) return false;
    }

    if (fix.valid.satellites) {
      if (fix.satellites < cfg.minSatellites) return false;
    }

    return true;
  }
};




extern void correct_trip_time_if_needed();

void sync_rtc_from_gps_fix(const gps_fix &latest_fix) {
  if (latest_fix.valid.time && latest_fix.valid.date) {
    int year = latest_fix.dateTime.year;
    if (year < 100) {
      year += 2000;
    }
    rtc.setTime(
      latest_fix.dateTime.seconds,
      latest_fix.dateTime.minutes,
      latest_fix.dateTime.hours,
      latest_fix.dateTime.date,
      latest_fix.dateTime.month,
      year
    );
    rtc.setTime(rtc.getEpoch() + timezone_offset_seconds);
    gps_time_synced = true;
    correct_trip_time_if_needed();
  }

  gps_speed_valid = latest_fix.valid.speed;
  gps_speed_kmph = gps_speed_valid ? latest_fix.speed_kph() : 0;

  gps_location_valid = latest_fix.valid.location;
  if(gps_location_valid){
    fix_lat = latest_fix.latitude();
    fix_lng = latest_fix.longitude();
    last_gps_fix_time = rtc.getEpoch();
  }
  if(latest_fix.valid.satellites){
    last_sat_count = latest_fix.satellites;
  }
}

BaseDetector detector;

void setup_gps(){
    gpsSerial.begin(9600, SERIAL_8N1, 38, 37);
    detector.cfg.stationaryRadiusM = 80.0f;            // 50..100
    detector.cfg.stationaryTimeMs  = 10UL * 60UL * 1000UL;

    detector.cfg.serialStuckMs = 8000;                // adjust to your GPS update rate
    detector.cfg.timeStuckMs   = 15000;

    detector.cfg.atBaseRadiusM = 120.0f;
    detector.cfg.leaveRadiusM  = 250.0f;

    detector.cfg.maxHDOP = 3.0f;
    detector.cfg.minSatellites = 5;
}


void loop_gps(){
    if(detector.feedGPS(gps, gpsSerial, fix)){
        sync_rtc_from_gps_fix(fix);
        if(fix.valid.location){
        fix_lat = fix.latitude();
        fix_lng = fix.longitude();
        gps_location_valid = true;
        } else {
        gps_location_valid = false;
        }
    }
}
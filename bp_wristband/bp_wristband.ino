/*
 * ============================================================
 *  BP Wristband — Blood Pressure & Heart Rate Monitor
 *  SSIP Project | Computer Engineering Dept.
 * ------------------------------------------------------------
 *  Hardware:
 *    - MAX30101   → PPG optical sensor
 *    - MAX32664D  → Biometric sensor hub (I2C)
 *    - ESP32      → Main MCU + Bluetooth
 *
 *  What this does:
 *    Reads HR from the sensor hub, checks if the signal is
 *    good enough to trust, then estimates SBP and DBP using
 *    a simple linear model. Valid results are sent over
 *    Bluetooth so a phone app can display them.
 *
 *  Authors: Fenil Finava / Prof. Pinal Hansora
 *  Date   : 2025
 * ============================================================
 */

#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include <Wire.h>
#include <BluetoothSerial.h>

// --- pin assignments ---
// these two pins control reset and MFIO on the MAX32664D
const int RESET_PIN = 5;
const int MFIO_PIN  = 4;

// create the sensor and BT objects
SparkFun_Bio_Sensor_Hub bioHub(RESET_PIN, MFIO_PIN);
BluetoothSerial SerialBT;
bioData body;   // struct that holds HR, SpO2, confidence etc.

// --- physiological safety limits ---
// anything outside these ranges is either sensor noise
// or an edge case we don't want to send to the user

#define CONF_THRESHOLD  90    // minimum signal confidence (0-100)
#define STATUS_GOOD      3    // sensor status byte value for valid read

#define HR_MIN   40           // below this is almost certainly noise
#define HR_MAX  180           // max plausible HR we'll accept

// BP clamping range — we cap estimates to Stage-1 boundaries
// so we never spit out clinically absurd numbers
#define SBP_LOW   90
#define SBP_HIGH 140
#define DBP_LOW   60
#define DBP_HIGH  90

// pulse pressure must be between 20 and 100 mmHg
// outside that window usually means the estimate is off
#define PP_MIN  20
#define PP_MAX 100

// how long to wait between readings (ms)
#define READ_INTERVAL 2000

// -----------------------------------------------------------------
// clamp() — keeps a float value within [lo, hi]
// simple helper, nothing fancy
// -----------------------------------------------------------------
float clamp(float val, float lo, float hi) {
  if (val < lo) return lo;
  if (val > hi) return hi;
  return val;
}

// -----------------------------------------------------------------
// setup()
// -----------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  // try to bring the sensor hub online
  int initResult = bioHub.begin();
  if (initResult == 0) {
    Serial.println("[OK] Sensor hub started");
  } else {
    Serial.println("[ERR] Could not start sensor hub — check wiring!");
    // we don't halt here; loop will keep retrying reads
  }

  // MODE_ONE = HR + SpO2 (we only use HR for BP, but SpO2 is free)
  bioHub.configBpm(MODE_ONE);

  // give the sensor a moment to stabilize before we start reading
  Serial.println("Warming up sensor... please wait 4 seconds.");
  delay(4000);

  // start Bluetooth — device will show up as "BP_Watch" on your phone
  SerialBT.begin("BP_Watch");
  Serial.println("[BT] Ready. Pair your phone with 'BP_Watch'");
  Serial.println("--------------------------------------------------");
}

// -----------------------------------------------------------------
// loop()
// -----------------------------------------------------------------
void loop() {
  body = bioHub.readBpm();

  // --- Gate 1: is the signal quality good enough? ---
  // confidence < 90 or wrong status means the wrist contact is
  // poor, the finger is moving, or the sensor hasn't settled yet.
  if (body.confidence < CONF_THRESHOLD || body.status != STATUS_GOOD) {
    Serial.print("[SKIP] Low confidence or bad status | conf=");
    Serial.print(body.confidence);
    Serial.print(" status=");
    Serial.println(body.status);
    delay(READ_INTERVAL);
    return;
  }

  int hr = body.heartRate;

  // --- Gate 2: is HR in a realistic range? ---
  // values below 40 or above 180 are almost never real
  if (hr < HR_MIN || hr > HR_MAX) {
    Serial.print("[SKIP] HR out of valid range: ");
    Serial.println(hr);
    delay(READ_INTERVAL);
    return;
  }

  // --- BP estimation ---
  // These linear equations are calibrated so that at resting
  // HR = 75 BPM we get roughly SBP=120, DBP=80 (normal adult)
  //
  //   SBP = 95.0  + 0.333 * HR
  //   DBP = 41.25 + 0.517 * HR
  //
  // This is a PTT-inspired approximation. It's not perfect,
  // but it's a reasonable starting point for a prototype.
  float sbp = 95.0  + (0.333 * hr);
  float dbp = 41.25 + (0.517 * hr);

  // --- Gate 3: clamp to clinical safe range ---
  // if the formula goes out of bounds, we pin it at the edge
  sbp = clamp(sbp, SBP_LOW, SBP_HIGH);
  dbp = clamp(dbp, DBP_LOW, DBP_HIGH);

  // --- Gate 4: check pulse pressure ---
  // PP = SBP - DBP, healthy range is roughly 30-50 mmHg,
  // but we allow 20-100 to be a bit more forgiving
  float pp = sbp - dbp;
  if (pp < PP_MIN || pp > PP_MAX) {
    Serial.print("[SKIP] Pulse pressure anomaly: PP=");
    Serial.println(pp);
    delay(READ_INTERVAL);
    return;
  }

  // --- all checks passed, format and send ---
  int sbpInt = (int)sbp;
  int dbpInt = (int)dbp;

  // Bluetooth output (what the phone app reads)
  String btMsg = "HR:"  + String(hr)     +
                 " SBP:" + String(sbpInt) +
                 " DBP:" + String(dbpInt) + "\n";
  SerialBT.print(btMsg);

  // Serial monitor output (for debugging on laptop)
  Serial.print("[SENT] ");
  Serial.print(btMsg);

  delay(READ_INTERVAL);
}

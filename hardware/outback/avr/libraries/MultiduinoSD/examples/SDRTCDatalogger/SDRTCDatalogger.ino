/*
  SDRTCDatalogger
  ---------------
  Logs timestamped sensor readings to the SD card every LOG_INTERVAL_MS.
  Each log line is CSV: timestamp, A0 raw ADC, A1 raw ADC.

  The log filename is built from the RTC date: "YYYYMMDDnnnn.CSV" where
  nnnn is a 4-digit sequence that increments until a free name is found,
  preventing collisions across reboots on the same day.

  Graceful degradation:
    - If the SD card is absent, readings are logged to Serial only.
    - If the RTC is absent, timestamps are replaced with a millisecond
      counter (mm:ss since boot).

  Hardware: Multiduino (ATmega328P)
    - micro SD : SPI, CS = D10
    - DS1307   : I2C, A4=SDA, A5=SCL
  Libraries: MultiduinoSD v2, MultiduinoRTC
*/

#include <MultiduinoSD.h>
#include <MultiduinoRTC.h>

static const uint32_t LOG_INTERVAL_MS = 1000;

static char    logFilename[16];
static SDFile  logFile;
static bool    sdOk  = false;
static bool    rtcOk = false;

// Build a unique log filename from today's date + a sequence number.
static void buildFilename(const DateTime& t) {
    for (uint16_t seq = 0; seq < 9999; seq++) {
        snprintf(logFilename, sizeof(logFilename),
                 "%04u%02u%02u%04u.CS",   // 8.3 format: YYYYMMDD####.CS
                 t.year, t.month, t.day, seq);
        if (!MultiduinoSD.exists(logFilename)) break;
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    // ---- RTC ----------------------------------------------------------------
    if (MultiduinoRTC.begin()) {
        rtcOk = true;
        if (!MultiduinoRTC.isRunning()) {
            Serial.println(F("RTC stopped — setting to compile time."));
            const char* ds = __DATE__; const char* ts = __TIME__;
            const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
            char mn[4] = {ds[0], ds[1], ds[2], 0};
            uint8_t mon = 1;
            for (uint8_t i = 0; i < 12; i++) {
                if (strncmp(mn, months + i * 3, 3) == 0) { mon = i + 1; break; }
            }
            MultiduinoRTC.adjust(DateTime(
                (uint16_t)atoi(ds + 7), mon, (uint8_t)atoi(ds + 4),
                (uint8_t)atoi(ts), (uint8_t)atoi(ts + 3), (uint8_t)atoi(ts + 6)));
        }
        Serial.println(F("RTC OK."));
    } else {
        Serial.println(F("WARNING: DS1307 not found — millis timestamps."));
    }

    // ---- SD -----------------------------------------------------------------
    if (MultiduinoSD.begin()) {
        sdOk = true;
        Serial.println(F("SD OK."));
        MultiduinoSD.printCardInfo(Serial);

        DateTime t = rtcOk ? MultiduinoRTC.now() : DateTime(2000, 1, 1);
        buildFilename(t);
        Serial.print(F("Log file: ")); Serial.println(logFilename);

        logFile = MultiduinoSD.open(logFilename, FILE_WRITE);
        if (logFile) {
            logFile.println("timestamp,a0_raw,a1_raw");
            logFile.flush();
        } else {
            Serial.println(F("ERROR: could not create log file."));
            sdOk = false;
        }
    } else {
        Serial.println(F("WARNING: SD not found — logging to Serial only."));
    }

    Serial.println();
    Serial.println(F("Logging started."));
    Serial.println(F("timestamp,a0_raw,a1_raw"));
}

void loop() {
    static uint32_t lastLog = 0;
    uint32_t now = millis();
    if (now - lastLog < LOG_INTERVAL_MS) return;
    lastLog = now;

    // ---- Gather data --------------------------------------------------------
    DateTime t = rtcOk ? MultiduinoRTC.now()
                       : DateTime(2000, 1, 1, 0, (uint8_t)(now / 60000 % 60),
                                  (uint8_t)(now / 1000 % 60));
    int a0 = analogRead(A0);
    int a1 = analogRead(A1);

    // ---- Format line --------------------------------------------------------
    char tsBuf[20]; t.toString(tsBuf);
    char line[48];
    snprintf(line, sizeof(line), "%s,%d,%d", tsBuf, a0, a1);

    // ---- Output -------------------------------------------------------------
    Serial.println(line);

    if (sdOk && logFile) {
        logFile.println(line);
        logFile.flush();
    }
}

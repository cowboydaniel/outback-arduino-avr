/*
 * SDRTCDatalogger — Multiduino example
 *
 * Logs timestamped sensor readings to the SD card every LOG_INTERVAL_MS.
 * Each log line is CSV: timestamp, A0 raw ADC, A1 raw ADC
 *
 * The log file is named DATExxxx.CSV where DATE is YYYYMMDD from the RTC
 * and xxxx is a 4-digit sequence number to avoid collisions across reboots.
 *
 * Hardware: Multiduino
 *   - micro SD : SPI, CS = D10
 *   - DS1307   : I2C, A4=SDA, A5=SCL
 * Libraries: MultiduinoSD, MultiduinoRTC
 */

#include <MultiduinoSD.h>
#include <MultiduinoRTC.h>

static const uint32_t LOG_INTERVAL_MS = 1000;  // log once per second

static char  logFilename[16];   // e.g. "20250217_001.CSV"
static File  logFile;
static bool  sdOk  = false;
static bool  rtcOk = false;

// Build a unique filename: DATExxxx.CSV, incrementing xxxx until a free name.
static void buildFilename(const DateTime& t) {
    char dateStr[9];
    snprintf(dateStr, sizeof(dateStr), "%04u%02u%02u", t.year, t.month, t.day);

    for (uint16_t seq = 0; seq < 9999; seq++) {
        snprintf(logFilename, sizeof(logFilename), "%s%04u.CSV", dateStr, seq);
        if (!MultiduinoSD.exists(logFilename)) break;
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // --- RTC ---
    if (MultiduinoRTC.begin()) {
        rtcOk = true;
        if (!MultiduinoRTC.isRunning()) {
            Serial.println(F("RTC stopped — setting to compile time."));
            // Quick compile-time parse (same technique as RTCSetAndRead example)
            const char* months[] = {
                "Jan","Feb","Mar","Apr","May","Jun",
                "Jul","Aug","Sep","Oct","Nov","Dec"
            };
            char monStr[4]; int d, y, hh, mm, ss;
            sscanf(__DATE__, "%s %d %d", monStr, &d, &y);
            sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
            uint8_t mon = 1;
            for (uint8_t i = 0; i < 12; i++) {
                if (strncmp(monStr, months[i], 3) == 0) { mon = i + 1; break; }
            }
            MultiduinoRTC.adjust(DateTime(y, mon, d, hh, mm, ss));
        }
        Serial.println(F("RTC OK."));
    } else {
        Serial.println(F("WARNING: DS1307 not found — timestamps will be 00:00:00."));
    }

    // --- SD ---
    if (MultiduinoSD.begin()) {
        sdOk = true;
        Serial.println(F("SD OK."));

        DateTime t = rtcOk ? MultiduinoRTC.now() : DateTime(2000, 1, 1);
        buildFilename(t);
        Serial.print(F("Log file: "));
        Serial.println(logFilename);

        logFile = MultiduinoSD.open(logFilename, FILE_WRITE);
        if (logFile) {
            logFile.println(F("timestamp,a0_raw,a1_raw"));
            logFile.flush();
        } else {
            Serial.println(F("ERROR: could not create log file."));
            sdOk = false;
        }
    } else {
        Serial.println(F("WARNING: SD card not found — logging to Serial only."));
    }

    Serial.println(F("\nLogging started.\n"));
    Serial.println(F("timestamp,a0_raw,a1_raw"));
}

void loop() {
    static uint32_t lastLog = 0;
    uint32_t now = millis();
    if (now - lastLog < LOG_INTERVAL_MS) return;
    lastLog = now;

    // --- Gather data ---
    DateTime t = rtcOk ? MultiduinoRTC.now() : DateTime(2000, 1, 1, 0, 0, (uint8_t)(now / 1000 % 60));
    int a0 = analogRead(A0);
    int a1 = analogRead(A1);

    // --- Format line ---
    char tsBuf[20];
    t.toString(tsBuf);

    char line[48];
    snprintf(line, sizeof(line), "%s,%d,%d", tsBuf, a0, a1);

    // --- Output ---
    Serial.println(line);

    if (sdOk && logFile) {
        logFile.println(line);
        logFile.flush();   // ensure data is written even if power is cut
    }
}

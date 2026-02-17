/*
  SDRTCEventLogger
  ================
  Cross-library example: MultiduinoRTC + MultiduinoSD

  Writes timestamped event records to a CSV file on the SD card.
  Events are triggered by Serial input (press any key) or automatically
  every LOG_INTERVAL_S seconds as a "heartbeat" event.

  CSV format:  YYYY-MM-DD,HH:MM:SS,EVENT_CODE,DESCRIPTION\r\n

  Hardware: Multiduino with DS1307 RTC + SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoRTC.h>
#include <MultiduinoSD.h>

#define LOG_FILE         "EVENTS.CSV"
#define LOG_INTERVAL_S   30    // heartbeat every 30 seconds

static uint32_t g_lastLog    = 0;
static uint32_t g_eventCount = 0;

static void writeEvent(uint8_t code, const char* desc) {
    DateTime now = MultiduinoRTC.now();
    SDFile f = MultiduinoSD.open(LOG_FILE, FILE_WRITE);
    if (!f) { Serial.println(F("SD open failed")); return; }

    // Date
    char buf[8];
    snprintf(buf, sizeof(buf), "%04u", now.year); f.print(buf); f.write('-');
    snprintf(buf, sizeof(buf), "%02u", now.month); f.print(buf); f.write('-');
    snprintf(buf, sizeof(buf), "%02u", now.day);   f.print(buf); f.write(',');
    // Time
    snprintf(buf, sizeof(buf), "%02u", now.hour);   f.print(buf); f.write(':');
    snprintf(buf, sizeof(buf), "%02u", now.minute); f.print(buf); f.write(':');
    snprintf(buf, sizeof(buf), "%02u", now.second); f.print(buf); f.write(',');
    // Code + description
    snprintf(buf, sizeof(buf), "%u", code); f.print(buf); f.write(',');
    f.println(desc);
    f.close();

    g_eventCount++;
    char ts[20]; now.toString(ts);
    Serial.print(F("Logged: ")); Serial.print(ts);
    Serial.print(F("  code=")); Serial.print(code);
    Serial.print(F("  ")); Serial.println(desc);
}

static void dumpCSV() {
    SDFile f = MultiduinoSD.open(LOG_FILE, FILE_READ);
    if (!f) { Serial.println(F("Cannot open log")); return; }
    Serial.println(F("\n--- EVENTS.CSV ---"));
    char line[64];
    while (f.available()) {
        f.readLine(line, sizeof(line));
        Serial.println(line);
    }
    Serial.print(F("--- ")); Serial.print(g_eventCount); Serial.println(F(" events ---\n"));
    f.close();
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("RTC not found!")); while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 8, 0, 0, 3));
    }
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!")); while (true) {}
    }

    // Write CSV header if file is new
    if (!MultiduinoSD.exists(LOG_FILE)) {
        SDFile f = MultiduinoSD.open(LOG_FILE,
                       SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
        if (f) { f.println(F("date,time,code,description")); f.close(); }
    }

    g_lastLog = MultiduinoRTC.now().toEpoch();
    writeEvent(1, "BOOT");

    Serial.println(F("SDRTCEventLogger ready."));
    Serial.print(F("Press any key to log an event.  Heartbeat every "));
    Serial.print(LOG_INTERVAL_S); Serial.println(F("s.\n"));
}

void loop() {
    DateTime now = MultiduinoRTC.now();
    uint32_t epoch = now.toEpoch();

    if (Serial.available()) {
        while (Serial.available()) Serial.read();
        writeEvent(2, "MANUAL");
    }

    if ((int32_t)(epoch - g_lastLog) >= LOG_INTERVAL_S) {
        g_lastLog = epoch;
        writeEvent(3, "HEARTBEAT");
    }

    // Dump after every 5 events
    if (g_eventCount > 0 && g_eventCount % 5 == 0) {
        dumpCSV();
    }
    delay(500);
}

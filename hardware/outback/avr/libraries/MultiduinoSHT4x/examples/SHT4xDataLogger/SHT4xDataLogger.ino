/*
 * SHT4xDataLogger
 *
 * Logs timestamped temperature and humidity readings to the SD card using
 * the DS3231 (MultiduinoRTCv2) for timestamps and the SHT4x for readings.
 *
 * Demonstrates using both 3.3V-rail sensors together.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoRTCv2.h>
#include <MultiduinoSHT4x.h>
#include <MultiduinoSD.h>

static const char* LOG_FILE = "envlog.csv";

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Enable 3.3V rail before initialising any 3.3V peripherals
    MultiduinoPower.begin();
    delay(5);

    if (!RTCv2.begin()) {
        Serial.println(F("DS3231 not found.")); while (1);
    }
    if (!SHT4x.begin()) {
        Serial.println(F("SHT4x not found.")); while (1);
    }
    if (!MultiduinoSD.begin(PIN_SD_DET)) {
        Serial.println(F("SD not found or not inserted.")); while (1);
    }

    // Write CSV header if file doesn't exist yet
    if (!MultiduinoSD.exists(LOG_FILE)) {
        SDFile f = MultiduinoSD.open(LOG_FILE, FILE_WRITE);
        if (f) { f.println("datetime,temp_c,humidity_pct"); f.close(); }
    }

    Serial.println(F("Logging every 30 seconds..."));
}

void loop() {
    static uint32_t lastLog = 0;
    if (millis() - lastLog < 30000UL) return;
    lastLog = millis();

    float tempC, humidity;
    if (!SHT4x.read(tempC, humidity)) return;

    char ts[20];
    RTCv2.now().toString(ts);

    // Log to SD
    SDFile f = MultiduinoSD.open(LOG_FILE, FILE_WRITE);
    if (f) {
        f.print(ts); f.print(',');
        f.print(tempC, 2); f.print(',');
        f.println(humidity, 2);
        f.close();
    }

    // Also print to Serial
    Serial.print(ts); Serial.print(F("  "));
    Serial.print(tempC, 2); Serial.print(F(" C  "));
    Serial.print(humidity, 2); Serial.println(F(" %"));
}

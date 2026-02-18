/*
  RTCEpochConvert
  ===============
  Demonstrates toEpoch() and fromEpoch() on the DateTime struct.

  Every 5 seconds the current time is read from the DS1307, converted to a
  Y2K-epoch (seconds since 2000-01-01 00:00:00), and then reconstructed back
  into a DateTime to verify the round-trip.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found – check wiring!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        Serial.println(F("RTC halted – setting to 2025-01-01 00:00:00"));
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 0, 0, 0, 3)); // Wednesday
    }
    Serial.println(F("RTCEpochConvert ready.\n"));
}

void loop() {
    DateTime now = MultiduinoRTC.now();

    uint32_t epoch = now.toEpoch();
    DateTime rebuilt = DateTime::fromEpoch(epoch);

    char buf[20];
    now.toString(buf);
    Serial.print(F("Now     : ")); Serial.println(buf);

    Serial.print(F("Epoch   : ")); Serial.println(epoch);

    rebuilt.toString(buf);
    Serial.print(F("Rebuilt : ")); Serial.println(buf);

    bool match = (now.year   == rebuilt.year   &&
                  now.month  == rebuilt.month  &&
                  now.day    == rebuilt.day    &&
                  now.hour   == rebuilt.hour   &&
                  now.minute == rebuilt.minute &&
                  now.second == rebuilt.second);
    Serial.println(match ? F("Round-trip: OK") : F("Round-trip: MISMATCH"));
    Serial.println();
    delay(5000);
}

/*
  RTCDateArithmetic
  =================
  Demonstrates addSeconds() and secondsSince() on the DateTime struct.

  On boot the current RTC time is captured as a reference.  Every 10 seconds:
    - addSeconds() advances a copy of that reference by a growing offset and
      prints the resulting future timestamp.
    - secondsSince() is called on that future DateTime (not on the live clock)
      to confirm it reports exactly the offset back – proving the two functions
      are inverses of each other.
    - The live wall-clock seconds elapsed since boot are shown separately.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

static DateTime g_ref;
static int32_t  g_addOffset = 0;

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        MultiduinoRTC.adjust(DateTime(2025, 6, 15, 12, 0, 0, 7));
    }
    g_ref = MultiduinoRTC.now();
    char buf[20]; g_ref.toString(buf);
    Serial.print(F("Reference: ")); Serial.println(buf);
    Serial.println();
}

void loop() {
    g_addOffset += 3661; // +1 hour, 1 minute, 1 second each iteration

    DateTime future = g_ref;
    future.addSeconds(g_addOffset);

    // secondsSince() on the computed future proves it round-trips correctly.
    int32_t roundTrip = future.secondsSince(g_ref);

    // Separately measure real wall-clock seconds using the live RTC reading.
    DateTime now = MultiduinoRTC.now();
    int32_t wallElapsed = now.secondsSince(g_ref);

    char buf[20];
    Serial.print(F("Offset +"));  Serial.print(g_addOffset); Serial.println(F(" s:"));
    future.toString(buf);
    Serial.print(F("  Future           : ")); Serial.println(buf);
    Serial.print(F("  future-ref (s)   : ")); Serial.println(roundTrip);
    Serial.print(F("  Wall elapsed (s) : ")); Serial.println(wallElapsed);
    Serial.println();
    delay(10000);
}

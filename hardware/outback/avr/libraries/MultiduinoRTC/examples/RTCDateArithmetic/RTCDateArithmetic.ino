/*
  RTCDateArithmetic
  =================
  Demonstrates addSeconds() and secondsSince() on the DateTime struct.

  On boot the current RTC time is captured as a reference.  Every 10 seconds:
    - addSeconds() advances a copy of that reference by a growing offset.
    - secondsSince() measures how far the current time is from the reference.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/

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

    DateTime now = MultiduinoRTC.now();
    int32_t elapsed = now.secondsSince(g_ref);

    char buf[20];
    Serial.print(F("Offset +"));  Serial.print(g_addOffset); Serial.println(F("s:"));
    future.toString(buf);
    Serial.print(F("  Future  : ")); Serial.println(buf);
    Serial.print(F("  Elapsed : ")); Serial.print(elapsed); Serial.println(F(" s since reference"));
    Serial.println();
    delay(10000);
}

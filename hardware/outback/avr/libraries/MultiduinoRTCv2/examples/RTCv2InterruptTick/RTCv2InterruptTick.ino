/*
 * RTCv2InterruptTick
 *
 * Uses Alarm 1 in "once per second" mode to fire a callback every second
 * via the DS3231 INT output on PE1 (requires JP4 closed).
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoRTCv2.h>

volatile bool tickFlag = false;

void onSecond() {
    tickFlag = true;
    RTCv2.clearAlarm1();  // must re-arm before next second
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();   // enable 3.3V rail first
    delay(5);

    if (!RTCv2.begin()) {
        Serial.println(F("DS3231 not found — check wiring and 3.3V rail."));
        while (1);
    }

    // Alarm 1 every second; callback on PE1 INT (JP4 must be closed)
    RTCv2.setAlarm1(DateTime(), ALARM1_EVERY_SECOND);
    RTCv2.onAlarm1(onSecond);

    Serial.println(F("RTCv2 — interrupt-driven 1-second tick"));
}

void loop() {
    if (tickFlag) {
        tickFlag = false;
        char buf[20];
        RTCv2.now().toString(buf);
        Serial.println(buf);
    }
}

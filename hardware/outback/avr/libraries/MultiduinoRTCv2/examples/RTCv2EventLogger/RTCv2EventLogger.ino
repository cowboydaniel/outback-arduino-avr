/*
 * RTCv2EventLogger
 *
 * Logs timestamped events to Serial.  Uses the DS3231 temperature reading
 * and demonstrates polling-mode alarm detection without an interrupt pin.
 *
 * Also shows forceConversion() for an on-demand temperature read.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoRTCv2.h>

static uint32_t lastLog = 0;

void logEvent(const char* msg) {
    char buf[20];
    RTCv2.now().toString(buf);
    Serial.print(buf);
    Serial.print(F("  "));
    Serial.println(msg);
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!RTCv2.begin()) {
        Serial.println(F("DS3231 not found."));
        while (1);
    }

    // Set to compile time if oscillator was stopped
    if (!RTCv2.isRunning()) {
        RTCv2.adjust(DateTime(2025, 1, 1, 0, 0, 0));
        logEvent("RTC time reset to default.");
    }

    // Alarm 2 fires every minute — polled in loop
    RTCv2.setAlarm2(DateTime(), ALARM2_EVERY_MINUTE);

    // Enable the DS3231 control register alarm flag — no PCINT needed here
    // (just poll alarm2Fired() in loop)

    logEvent("Logger started.");
}

void loop() {
    if (RTCv2.alarm2Fired()) {
        RTCv2.clearAlarm2();

        RTCv2.forceConversion();
        float t = RTCv2.temperature();

        char msg[32];
        int whole = (int)t;
        int frac  = (int)((t - (float)whole) * 100.0f);
        if (frac < 0) frac = -frac;
        snprintf(msg, sizeof(msg), "Temp: %d.%02d C", whole, frac);
        logEvent(msg);
    }
}

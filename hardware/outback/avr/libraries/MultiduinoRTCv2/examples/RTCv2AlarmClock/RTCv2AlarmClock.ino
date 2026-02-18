/*
 * RTCv2AlarmClock
 *
 * Sets Alarm 1 for a specific time and Alarm 2 for every minute.
 * Both use interrupt callbacks via PE1 (requires JP4 closed).
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoRTCv2.h>

volatile bool alarm1Flag = false;
volatile bool alarm2Flag = false;

void onAlarm1() {
    alarm1Flag = true;
    RTCv2.clearAlarm1();
}

void onAlarm2() {
    alarm2Flag = true;
    RTCv2.clearAlarm2();
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

    // Set the current time if needed
    // RTCv2.adjust(DateTime(2025, 6, 1, 8, 59, 55));

    // Alarm 1: fire at 09:00:00 every day
    RTCv2.setAlarm1(DateTime(2000, 1, 1, 9, 0, 0), ALARM1_MATCH_HOURS_MINUTES_SECONDS);
    RTCv2.onAlarm1(onAlarm1);

    // Alarm 2: fire at every full minute (:00 seconds)
    RTCv2.setAlarm2(DateTime(), ALARM2_EVERY_MINUTE);
    RTCv2.onAlarm2(onAlarm2);

    Serial.println(F("RTCv2 alarm clock running."));
    Serial.println(F("Alarm 1: 09:00:00 daily.  Alarm 2: every minute."));
}

void loop() {
    if (alarm1Flag) {
        alarm1Flag = false;
        Serial.println(F("*** ALARM 1 — 09:00:00 ***"));
    }
    if (alarm2Flag) {
        alarm2Flag = false;
        char buf[20];
        RTCv2.now().toString(buf);
        Serial.print(F("Minute tick: "));
        Serial.println(buf);
    }
}

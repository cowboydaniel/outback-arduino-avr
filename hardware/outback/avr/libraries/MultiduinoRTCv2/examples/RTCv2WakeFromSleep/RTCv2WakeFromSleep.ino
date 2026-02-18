/*
 * RTCv2WakeFromSleep
 *
 * Puts the MCU into power-down sleep mode and uses DS3231 Alarm 2 via
 * PCINT3 (PE1, JP4 closed) to wake it once per minute.
 * The 3.3V rail is left on during sleep so the RTC keeps ticking.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoRTCv2.h>
#include <avr/sleep.h>

volatile bool woke = false;

void wakeISR() {
    woke = true;
    RTCv2.clearAlarm2();
}

void goToSleep() {
    woke = false;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();   // 3.3V rail on — keep it on so RTC runs during sleep
    delay(5);

    if (!RTCv2.begin()) {
        Serial.println(F("DS3231 not found."));
        while (1);
    }

    // Wake once per minute (Alarm 2, every minute at :00 seconds)
    RTCv2.setAlarm2(DateTime(), ALARM2_EVERY_MINUTE);
    RTCv2.onAlarm2(wakeISR);

    Serial.println(F("RTCv2 wake-from-sleep demo — sleeping until :00 each minute."));
    Serial.flush();
}

void loop() {
    goToSleep();

    // Woke up
    char buf[20];
    RTCv2.now().toString(buf);
    Serial.print(F("Woke at: "));
    Serial.println(buf);
    Serial.print(F("RTC temp: "));
    Serial.print(RTCv2.temperature(), 2);
    Serial.println(F(" C"));
    Serial.flush();
}

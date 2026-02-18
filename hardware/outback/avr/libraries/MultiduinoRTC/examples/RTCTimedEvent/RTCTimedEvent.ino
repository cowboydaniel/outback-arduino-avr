/*
  RTCTimedEvent
  =============
  Shows how to trigger events at exact calendar times using the RTC.

  Two events are defined: a "morning alarm" at 07:00 and an "evening alarm"
  at 22:00.  The sketch compares the current time against each event every
  second.  When a match occurs the event fires (here it prints a message;
  in a real application you would actuate a relay, buzzer, etc.).

  A simple one-shot debounce ensures each event fires only once per day
  even if the MCU is slower than 1 s per iteration.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

struct TimedEvent {
    uint8_t hour;
    uint8_t minute;
    const char* label;
    bool    fired;   // true once the event has fired today
};

static TimedEvent EVENTS[] = {
    {  7, 0, "Morning alarm", false },
    { 22, 0, "Evening alarm", false },
};
static const uint8_t NUM_EVENTS = sizeof(EVENTS) / sizeof(EVENTS[0]);

static uint8_t g_lastDay = 0;   // Reset fired flags at midnight

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        // Set to just before morning alarm for quick demo
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 6, 59, 55, 3));
    }
    Serial.println(F("RTCTimedEvent ready.  Watching for alarms...\n"));
}

void loop() {
    DateTime now = MultiduinoRTC.now();

    // Reset fired flags at the start of a new day
    if (now.day != g_lastDay) {
        g_lastDay = now.day;
        for (uint8_t i = 0; i < NUM_EVENTS; i++) EVENTS[i].fired = false;
    }

    char buf[20]; now.toString(buf);
    Serial.println(buf);

    for (uint8_t i = 0; i < NUM_EVENTS; i++) {
        if (!EVENTS[i].fired &&
            now.hour   == EVENTS[i].hour &&
            now.minute == EVENTS[i].minute) {
            Serial.print(F("*** EVENT: ")); Serial.print(EVENTS[i].label);
            Serial.println(F(" ***"));
            EVENTS[i].fired = true;
        }
    }
    delay(1000);
}

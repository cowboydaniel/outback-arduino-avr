/*
  RTCWorldClock
  =============
  Uses addSeconds() to display the current time in several time zones.

  The RTC is assumed to store UTC.  A table of UTC offsets (in seconds) is
  applied using DateTime::addSeconds() to derive local times for each zone.
  The zone name, offset, and local time string are printed every 10 seconds.

  Hardware: Multiduino with DS1307 RTC (set to UTC).
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

struct Zone {
    const char* name;
    int32_t     offsetSec;  // UTC offset in seconds (negative = west)
};

static const Zone ZONES[] = {
    { "UTC+0  (London,  winter)", 0         },
    { "UTC+1  (Berlin,  winter)", 3600L     },
    { "UTC+3  (Moscow          )", 10800L   },
    { "UTC+5:30 (Mumbai        )", 19800L   },
    { "UTC+8  (Singapore       )", 28800L   },
    { "UTC+9  (Tokyo           )", 32400L   },
    { "UTC-5  (New York, winter)", -18000L  },
    { "UTC-8  (Los Angeles, win)", -28800L  },
};
static const uint8_t NUM_ZONES = sizeof(ZONES) / sizeof(ZONES[0]);

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        // Set RTC to a known UTC time for demonstration
        MultiduinoRTC.adjust(DateTime(2025, 3, 15, 12, 0, 0, 6));
    }
    Serial.println(F("RTCWorldClock – printing every 10 s\n"));
}

void loop() {
    DateTime utc = MultiduinoRTC.now();
    char buf[20]; utc.toString(buf);
    Serial.print(F("UTC base : ")); Serial.println(buf);
    Serial.println(F("---"));

    for (uint8_t i = 0; i < NUM_ZONES; i++) {
        DateTime local = utc;
        local.addSeconds(ZONES[i].offsetSec);
        local.toString(buf);
        Serial.print(ZONES[i].name); Serial.print(F("  ")); Serial.println(buf);
    }
    Serial.println();
    delay(10000);
}

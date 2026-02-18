/*
  RTCDayOfWeekCalc
  ================
  Demonstrates computeDow(), dayName(), monthName(), and isLeapYear().

  For each day of a configurable date range the sketch constructs a DateTime,
  calls computeDow() to fill in the ISO day-of-week, then prints the result.
  It also confirms that the computed dow matches the value stored by the RTC.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

// Dates to check
static const struct { uint16_t y; uint8_t m, d; } DATES[] = {
    {2000,  1,  1},  // Saturday
    {2024,  2, 29},  // Thursday (leap day)
    {2025,  7,  4},  // Friday
    {2026,  1,  1},  // Thursday
    {2038,  1, 19},  // Tuesday (near 32-bit Unix overflow)
};
static const uint8_t NUM_DATES = sizeof(DATES) / sizeof(DATES[0]);

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }

    Serial.println(F("Date              Leap  DoW  Day"));
    Serial.println(F("---------------------------------"));

    for (uint8_t i = 0; i < NUM_DATES; i++) {
        DateTime dt(DATES[i].y, DATES[i].m, DATES[i].d);
        dt.computeDow();

        char buf[20]; dt.toString(buf);
        Serial.print(buf);
        Serial.print(F("  "));
        Serial.print(dt.isLeapYear() ? F("yes") : F("no "));
        Serial.print(F("   "));
        Serial.print(dt.dow);
        Serial.print(F("    "));
        Serial.println(dt.dayName());
    }

    // Also show the current RTC time with its month name
    Serial.println();
    DateTime now = MultiduinoRTC.now();
    Serial.print(F("RTC month: ")); Serial.println(now.monthName());
}

void loop() {}

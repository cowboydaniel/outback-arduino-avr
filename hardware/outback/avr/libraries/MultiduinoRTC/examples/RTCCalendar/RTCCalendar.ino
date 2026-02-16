/*
  RTCCalendar
  -----------
  Reads the DS1307 RTC and prints a human-readable calendar display to
  Serial once per second, including:

    - Full date: "Wednesday, 14 January 2026"
    - Time:      "09:05:32"
    - ISO week number (1–53)
    - Day of year (1–366, leap-year aware)
    - Days remaining in the year
    - Whether the current year is a leap year

  The Multiduino DS1307 uses ISO 8601 day-of-week (1=Monday, 7=Sunday).

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>

static const char* const kDayNames[] = {
    "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

static const char* const kMonthNames[] = {
    "", "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const uint8_t kDaysInMonth[] = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

bool isLeapYear(uint16_t y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

// Day-of-year: 1 = Jan 1
uint16_t dayOfYear(const DateTime& dt) {
    uint16_t doy = dt.day;
    for (uint8_t m = 1; m < dt.month; m++) {
        doy += kDaysInMonth[m];
        if (m == 2 && isLeapYear(dt.year)) doy++;
    }
    return doy;
}

// ISO week number (1–53)
// Uses a simplified Doomsday-adjacent calculation.
uint8_t isoWeekNumber(const DateTime& dt) {
    uint16_t doy = dayOfYear(dt);
    // ISO week starts on Monday; dt.dow: 1=Mon, 7=Sun
    // Day of week correction so that Jan 4 is always in week 1
    int16_t w = (doy - dt.dow + 10) / 7;
    if (w < 1)  return 52;   // Last week of previous year (simplified)
    if (w > 52 && dt.month == 12 && dt.day >= 29) return 1; // First week of next year
    return (uint8_t)w;
}

uint32_t lastPrint = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    if (!MultiduinoRTC.isRunning()) {
        const char* ds = __DATE__;
        const char* ts = __TIME__;
        const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mn[4] = {ds[0], ds[1], ds[2], 0};
        uint8_t mon = 1;
        for (uint8_t i = 0; i < 12; i++) {
            if (strncmp(mn, months + i * 3, 3) == 0) { mon = i + 1; break; }
        }
        MultiduinoRTC.adjust(DateTime(
            (uint16_t)atoi(ds + 7), mon, (uint8_t)atoi(ds + 4),
            (uint8_t)atoi(ts), (uint8_t)atoi(ts + 3), (uint8_t)atoi(ts + 6)));
        Serial.println(F("RTC set to compile time."));
    }
}

void loop() {
    if (millis() - lastPrint < 1000) return;
    lastPrint = millis();

    DateTime dt = MultiduinoRTC.now();
    uint16_t doy = dayOfYear(dt);
    uint16_t totalDays = isLeapYear(dt.year) ? 366 : 365;

    // Day and date line
    Serial.println();
    Serial.print(kDayNames[dt.dow]);
    Serial.print(F(", "));
    Serial.print(dt.day);
    Serial.print(' ');
    Serial.print(kMonthNames[dt.month]);
    Serial.print(' ');
    Serial.println(dt.year);

    // Time line
    if (dt.hour < 10)   Serial.print('0');
    Serial.print(dt.hour);   Serial.print(':');
    if (dt.minute < 10) Serial.print('0');
    Serial.print(dt.minute); Serial.print(':');
    if (dt.second < 10) Serial.print('0');
    Serial.println(dt.second);

    // Calendar info
    Serial.print(F("ISO week     : ")); Serial.println(isoWeekNumber(dt));
    Serial.print(F("Day of year  : ")); Serial.print(doy);
    Serial.print('/'); Serial.println(totalDays);
    Serial.print(F("Days left    : ")); Serial.println(totalDays - doy);
    Serial.print(F("Leap year    : ")); Serial.println(isLeapYear(dt.year) ? F("Yes") : F("No"));
    Serial.println(F("-----------------------------"));
}

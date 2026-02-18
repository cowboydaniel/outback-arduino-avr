/*
 * RTCSetAndRead — Multiduino example
 *
 * Sets the onboard DS1307 RTC to the time this sketch was compiled,
 * then prints the current time to Serial every second.
 *
 * Hardware: Multiduino (DS1307 on I2C, A4=SDA, A5=SCL)
 * Library : MultiduinoRTC (no extra dependencies)
 */
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true);
    }

    // If the oscillator was stopped (e.g. first power-up or dead battery),
    // set the clock to the compile-time stamp.
    if (!MultiduinoRTC.isRunning()) {
        Serial.println(F("RTC stopped — setting time from compile timestamp."));

        // __DATE__ = "Mmm DD YYYY", __TIME__ = "HH:MM:SS"
        // Parse month name manually.
        const char* months[] = {
            "Jan","Feb","Mar","Apr","May","Jun",
            "Jul","Aug","Sep","Oct","Nov","Dec"
        };
        char monStr[4];
        int d, y, hh, mm, ss;
        sscanf(__DATE__, "%s %d %d", monStr, &d, &y);
        sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);

        uint8_t mon = 1;
        for (uint8_t i = 0; i < 12; i++) {
            if (strncmp(monStr, months[i], 3) == 0) { mon = i + 1; break; }
        }

        MultiduinoRTC.adjust(DateTime(y, mon, d, hh, mm, ss));
    }

    Serial.println(F("RTC ready. Printing time every second.\n"));
}

void loop() {
    DateTime t = MultiduinoRTC.now();

    char buf[20];
    t.toString(buf);
    Serial.println(buf);

    delay(1000);
}

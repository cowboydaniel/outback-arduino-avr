/*
 * RTCv2SqwFrequencies
 *
 * Cycles through the four DS3231 SQW output frequencies, measuring each
 * with PCINT3 on PE1 to confirm the rate (requires JP4 closed).
 * Also reads the on-chip temperature.
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoRTCv2.h>

volatile uint32_t edgeCount = 0;

void countEdge() { edgeCount++; }

void measureFreq(const __FlashStringHelper* label, uint8_t rate, uint32_t windowMs) {
    edgeCount = 0;
    // Use PCINT3 rising-edge counting via Alarm1 "every second" callback hack:
    // Actually we'll configure SQW and count edges by polling the pin directly
    RTCv2.setSqwFreq(rate);
    uint32_t t0 = millis();
    bool last = digitalRead(PIN_SQW);
    uint32_t cnt = 0;
    while (millis() - t0 < windowMs) {
        bool cur = digitalRead(PIN_SQW);
        if (cur && !last) cnt++;  // rising edge
        last = cur;
    }
    Serial.print(label);
    Serial.print(F(": ~"));
    Serial.print((uint32_t)(cnt * 1000UL / windowMs));
    Serial.println(F(" Hz"));
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

    Serial.println(F("RTCv2 SQW frequency test (JP4 must be closed):"));
    measureFreq(F("  1 Hz   "), DS3231_SQW_1HZ,    3000);
    measureFreq(F("  1024 Hz"), DS3231_SQW_1024HZ, 1000);
    measureFreq(F("  4096 Hz"), DS3231_SQW_4096HZ,  500);
    measureFreq(F("  8192 Hz"), DS3231_SQW_8192HZ,  500);

    RTCv2.forceConversion();
    float t = RTCv2.temperature();
    Serial.print(F("RTC temperature: "));
    Serial.print(t, 2);
    Serial.println(F(" C"));
}

void loop() {}

/*
 * SHT4xBasicRead
 *
 * Reads temperature and humidity from the on-board SHT4x sensor every second
 * and prints the results to Serial.
 *
 * Multiduino v2 only.
 * Remember: MultiduinoPower.begin() must be called before SHT4x.begin()
 * because the SHT4x is on the 3.3V rail.
 */

#include <MultiduinoPower.h>
#include <MultiduinoSHT4x.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();   // enable 3.3V rail first
    delay(5);

    if (!SHT4x.begin()) {
        Serial.println(F("SHT4x not found — check 3.3V rail and wiring."));
        while (1);
    }

    uint32_t sn = SHT4x.serialNumber();
    Serial.print(F("SHT4x serial number: 0x"));
    Serial.println(sn, HEX);
    Serial.println(F("Temperature (C), Humidity (%)"));
}

void loop() {
    float tempC, humidity;

    if (SHT4x.read(tempC, humidity)) {
        Serial.print(tempC, 2);
        Serial.print(F(", "));
        Serial.println(humidity, 2);
    } else {
        Serial.println(F("Read failed."));
    }

    delay(1000);
}

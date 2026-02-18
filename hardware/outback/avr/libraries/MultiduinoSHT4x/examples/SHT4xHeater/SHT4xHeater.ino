/*
 * SHT4xHeater
 *
 * Demonstrates the on-chip heater.  Taking a reading with the heater active
 * is useful for de-condensation or checking sensor response.
 *
 * The heater temporarily raises the sensor temperature; the humidity reading
 * after a heater cycle reflects the drier local environment.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoSHT4x.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!SHT4x.begin()) {
        Serial.println(F("SHT4x not found."));
        while (1);
    }

    // Normal reading first
    float tempC, humidity;
    if (SHT4x.read(tempC, humidity)) {
        Serial.print(F("Before heater: "));
        Serial.print(tempC, 2); Serial.print(F(" C, "));
        Serial.print(humidity, 2); Serial.println(F(" %"));
    }

    // Heater reading — 200mW for 1 second
    Serial.println(F("Activating 200mW heater for 1 second..."));
    if (SHT4x.heater(SHT4X_HEATER_200MW_1S, tempC, humidity)) {
        Serial.print(F("After heater:  "));
        Serial.print(tempC, 2); Serial.print(F(" C, "));
        Serial.print(humidity, 2); Serial.println(F(" %"));
    }

    // Allow sensor to recover
    delay(2000);
    if (SHT4x.read(tempC, humidity)) {
        Serial.print(F("After recovery: "));
        Serial.print(tempC, 2); Serial.print(F(" C, "));
        Serial.print(humidity, 2); Serial.println(F(" %"));
    }
}

void loop() {}

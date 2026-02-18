/*
 * EEPROMBasicReadWrite
 *
 * Write single bytes to the on-board I2C EEPROM and read them back.
 * Values survive power cycles — use MultiduinoFRAM for high-frequency writes.
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoEEPROM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!ExtEEPROM.begin()) {
        Serial.println(F("EEPROM not found — check 3.3V rail.")); while (1);
    }

    Serial.println(F("MultiduinoEEPROM — basic read/write"));

    // Write pattern to first 16 bytes
    for (uint8_t i = 0; i < 16; i++) ExtEEPROM.write(i, i * 10);

    // Read back
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t v = ExtEEPROM.read(i);
        Serial.print(F("addr ")); Serial.print(i);
        Serial.print(F(": ")); Serial.println(v);
    }
}

void loop() {}

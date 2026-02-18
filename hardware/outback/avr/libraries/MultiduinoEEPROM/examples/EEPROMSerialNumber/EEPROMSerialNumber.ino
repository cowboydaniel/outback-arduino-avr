/*
 * EEPROMSerialNumber
 *
 * Reads and prints the factory-programmed 64-bit unique serial number
 * from I2C address 0x58.  This ID is unique per device and read-only.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoEEPROM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!ExtEEPROM.begin()) {
        Serial.println(F("EEPROM not found.")); while (1);
    }

    Serial.print(F("Device serial number: "));
    ExtEEPROM.printSerialNumber(Serial);

    // Also available as raw bytes
    uint8_t sn[8];
    if (ExtEEPROM.readSerialNumber(sn)) {
        Serial.print(F("As uint64 MSB: 0x"));
        for (uint8_t i = 0; i < 8; i++) {
            if (sn[i] < 0x10) Serial.print('0');
            Serial.print(sn[i], HEX);
        }
        Serial.println();
    }
}

void loop() {}

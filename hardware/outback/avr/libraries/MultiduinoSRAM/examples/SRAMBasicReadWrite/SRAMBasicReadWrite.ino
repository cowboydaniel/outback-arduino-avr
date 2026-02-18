/*
 * SRAMBasicReadWrite
 *
 * Write single bytes to the 23AA02M SRAM and read them back,
 * confirming the values match over Serial.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoSRAM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    SRAM.begin();
    Serial.println(F("MultiduinoSRAM — basic read/write"));

    // Write a pattern across the first 256 bytes
    for (uint16_t i = 0; i < 256; i++) {
        SRAM.write(i, (uint8_t)i);
    }

    // Read back and verify
    uint16_t errors = 0;
    for (uint16_t i = 0; i < 256; i++) {
        uint8_t val = SRAM.read(i);
        if (val != (uint8_t)i) {
            Serial.print(F("MISMATCH at "));
            Serial.print(i);
            Serial.print(F(": expected "));
            Serial.print(i);
            Serial.print(F(", got "));
            Serial.println(val);
            errors++;
        }
    }

    if (errors == 0) {
        Serial.println(F("All 256 bytes verified OK."));
    } else {
        Serial.print(errors);
        Serial.println(F(" errors found."));
    }
}

void loop() {}

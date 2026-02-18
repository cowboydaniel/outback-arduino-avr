/*
 * FRAMBasicReadWrite
 *
 * Write single bytes to the on-board FRAM and read them back to confirm.
 * Demonstrates that values survive power cycles (unlike SRAM).
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoFRAM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!FRAM.begin()) {
        Serial.println(F("FRAM not found — check 3.3V rail.")); while (1);
    }

    Serial.println(F("MultiduinoFRAM — basic read/write"));

    // Write 0–255 pattern to the first 256 bytes
    for (uint16_t i = 0; i < 256; i++) FRAM.write(i, (uint8_t)i);

    // Verify
    uint16_t errors = 0;
    for (uint16_t i = 0; i < 256; i++) {
        uint8_t v = FRAM.read(i);
        if (v != (uint8_t)i) { errors++; }
    }

    if (errors == 0) {
        Serial.println(F("All 256 bytes verified OK."));
        Serial.println(F("These values will survive a power cycle."));
    } else {
        Serial.print(errors); Serial.println(F(" errors."));
    }
}

void loop() {}

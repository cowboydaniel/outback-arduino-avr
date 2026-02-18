/*
 * FRAMBootCounter
 *
 * Simple persistent boot counter stored in FRAM.
 * Unlike EEPROM, FRAM has no write endurance limit, so this is safe to
 * increment on every boot without concern for wear-out.
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoFRAM.h>

// Boot counter lives at FRAM byte 0 (4 bytes for a uint32_t)
FRAMSlot<uint32_t> bootCounter(0);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!FRAM.begin()) {
        Serial.println(F("FRAM not found.")); while (1);
    }

    uint32_t count = bootCounter.load();
    count++;
    bootCounter.save(count);

    Serial.print(F("This board has booted "));
    Serial.print(count);
    Serial.println(F(" time(s)."));
}

void loop() {}

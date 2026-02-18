/*
  NVRAMClearAndVerify
  -------------------
  Demonstrates NVRAM.clear() and NVRAM.fill(), then verifies the result
  by reading all 56 bytes back and checking each one.

  Also shows the CRC-8 helper and the effect of fill() versus clear():
    clear() zeros the entire NVRAM.
    fill()  writes any chosen byte value to the entire NVRAM.

  Typical use: run this sketch before a factory reset or when
  decommissioning a Multiduino board.

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

// Read all 56 bytes, count how many equal 'expected', print mismatches.
bool verifyAll(uint8_t expected) {
    bool ok = true;
    for (uint8_t i = 0; i < NVRAM_SIZE; i++) {
        uint8_t val = NVRAM.read(i);
        if (val != expected) {
            Serial.print(F("  MISMATCH at addr "));
            Serial.print(i);
            Serial.print(F(": expected 0x"));
            if (expected < 0x10) Serial.print('0');
            Serial.print(expected, HEX);
            Serial.print(F(", got 0x"));
            if (val < 0x10) Serial.print('0');
            Serial.println(val, HEX);
            ok = false;
        }
    }
    return ok;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM Clear and Verify ==="));
    Serial.println();

    // ------------------------------------------------------------------
    // 1. Fill with 0xFF and verify
    // ------------------------------------------------------------------
    Serial.println(F("Filling NVRAM with 0xFF..."));
    NVRAM.fill(0xFF);
    Serial.print(F("Verifying all bytes == 0xFF ... "));
    if (verifyAll(0xFF)) {
        Serial.println(F("PASS"));
    } else {
        Serial.println(F("FAIL (see mismatches above)"));
    }
    uint8_t crc = NVRAM.crc8(0, NVRAM_SIZE);
    Serial.print(F("CRC-8 over full NVRAM: 0x"));
    if (crc < 0x10) Serial.print('0');
    Serial.println(crc, HEX);
    Serial.println();

    // ------------------------------------------------------------------
    // 2. Clear (write 0x00 to all) and verify
    // ------------------------------------------------------------------
    Serial.println(F("Clearing NVRAM (writing 0x00 to all bytes)..."));
    NVRAM.clear();
    Serial.print(F("Verifying all bytes == 0x00 ... "));
    if (verifyAll(0x00)) {
        Serial.println(F("PASS"));
    } else {
        Serial.println(F("FAIL (see mismatches above)"));
    }
    crc = NVRAM.crc8(0, NVRAM_SIZE);
    Serial.print(F("CRC-8 over full NVRAM: 0x"));
    if (crc < 0x10) Serial.print('0');
    Serial.println(crc, HEX);
    Serial.println();

    // ------------------------------------------------------------------
    // 3. Show the hex dump of a cleared NVRAM
    // ------------------------------------------------------------------
    Serial.println(F("Hex dump after clear:"));
    NVRAM.dump(Serial);

    Serial.println();
    Serial.println(F("Done."));
}

void loop() {
    // Nothing – all work done in setup()
}

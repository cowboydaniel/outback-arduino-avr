/*
 * NVRAMReadWrite — Multiduino example
 *
 * Demonstrates reading and writing the DS1307's 56-byte battery-backed NVRAM.
 * NVRAM contents survive power cycles (provided the coin cell is installed).
 *
 * On each run:
 *   1. Reads a 4-byte boot counter stored in NVRAM[0..3].
 *   2. Increments it and writes it back.
 *   3. Dumps all 56 bytes in hex.
 *
 * Hardware: Multiduino (DS1307 on I2C, A4=SDA, A5=SCL)
 * Library : MultiduinoRTC
 */
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

static const uint8_t COUNTER_ADDR = 0;  // NVRAM byte address of the 32-bit counter

void setup() {
    Serial.begin(115200);
    while (!Serial);

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true);
    }

    // --- Read 32-bit boot counter from NVRAM[0..3] (big-endian) ---
    uint8_t raw[4];
    MultiduinoRTC.readNVRAM(COUNTER_ADDR, raw, 4);
    uint32_t count = ((uint32_t)raw[0] << 24) |
                     ((uint32_t)raw[1] << 16) |
                     ((uint32_t)raw[2] <<  8) |
                      (uint32_t)raw[3];

    count++;
    Serial.print(F("Boot count: "));
    Serial.println(count);

    // Write back
    raw[0] = (count >> 24) & 0xFF;
    raw[1] = (count >> 16) & 0xFF;
    raw[2] = (count >>  8) & 0xFF;
    raw[3] =  count        & 0xFF;
    MultiduinoRTC.writeNVRAM(COUNTER_ADDR, raw, 4);

    // --- Store a short string in NVRAM[4..] ---
    const char msg[] = "Multiduino";
    MultiduinoRTC.writeNVRAM(4, (const uint8_t*)msg, sizeof(msg));

    // --- Hex dump of all 56 NVRAM bytes ---
    Serial.println(F("\nNVRAM dump (56 bytes):"));
    Serial.println(F("     00 01 02 03 04 05 06 07"));
    for (uint8_t row = 0; row < 7; row++) {
        Serial.print(F("  "));
        if (row * 8 < 10) Serial.print('0');
        Serial.print(row * 8);
        Serial.print(F(": "));
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t val = MultiduinoRTC.readNVRAM(row * 8 + col);
            if (val < 0x10) Serial.print('0');
            Serial.print(val, HEX);
            Serial.print(' ');
        }
        Serial.println();
    }
}

void loop() {
    // Nothing to do after setup.
}

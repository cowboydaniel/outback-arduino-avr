/*
 * SRAMFillAndVerify
 *
 * Fills a region of SRAM with a known value then reads every byte back
 * to confirm integrity.  Useful as a first-power-on self-test.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoSRAM.h>

static const uint32_t TEST_ADDR = 0x00000UL;
static const uint32_t TEST_LEN  = 4096UL;   // 4 KB test region
static const uint8_t  FILL_VAL  = 0xA5;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    SRAM.begin();
    Serial.println(F("MultiduinoSRAM — fill and verify"));

    // Fill
    Serial.print(F("Filling "));
    Serial.print(TEST_LEN);
    Serial.print(F(" bytes at 0x"));
    Serial.print(TEST_ADDR, HEX);
    Serial.print(F(" with 0x"));
    Serial.println(FILL_VAL, HEX);
    SRAM.fill(TEST_ADDR, TEST_LEN, FILL_VAL);

    // Verify
    uint32_t errors = 0;
    static uint8_t buf[64];
    for (uint32_t off = 0; off < TEST_LEN; off += sizeof(buf)) {
        uint32_t n = TEST_LEN - off;
        if (n > sizeof(buf)) n = sizeof(buf);
        SRAM.readBlock(TEST_ADDR + off, buf, (uint32_t)n);
        for (uint32_t i = 0; i < n; i++) {
            if (buf[i] != FILL_VAL) errors++;
        }
    }

    if (errors == 0) {
        Serial.println(F("Verify PASS — all bytes correct."));
    } else {
        Serial.print(errors);
        Serial.println(F(" byte(s) incorrect — FAIL."));
    }
}

void loop() {}

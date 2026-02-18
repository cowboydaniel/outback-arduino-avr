/*
  NVRAMFillAndVerify
  ==================
  Demonstrates fill() and compare() by:
    1. Filling the whole NVRAM with 0xAA and verifying every byte.
    2. Filling a sub-range with 0x55 and verifying that range separately.
    3. Clearing NVRAM and confirming all zeros via compare().

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

static void report(const char* label, bool ok) {
    Serial.print(label); Serial.println(ok ? F(": OK") : F(": FAIL"));
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    Serial.println(F("=== NVRAMFillAndVerify ===\n"));

    // --- Step 1: fill all 56 bytes with 0xAA ---
    Serial.println(F("Step 1: fill all with 0xAA"));
    NVRAM.fill(0xAA);

    uint8_t ref1[NVRAM_SIZE];
    memset(ref1, 0xAA, NVRAM_SIZE);
    report("  compare(0, 56, 0xAA)", NVRAM.compare(0, ref1, NVRAM_SIZE));

    // --- Step 2: fill sub-range [8, 16) with 0x55 ---
    Serial.println(F("Step 2: fill [8..23] with 0x55"));
    NVRAM.fill(8, 16, 0x55);

    uint8_t ref2[16];
    memset(ref2, 0x55, 16);
    report("  compare(8, 16, 0x55)", NVRAM.compare(8, ref2, 16));

    // Bytes outside the sub-range should still be 0xAA
    uint8_t ref3[8];
    memset(ref3, 0xAA, 8);
    report("  compare(0,  8, 0xAA)", NVRAM.compare(0, ref3, 8));
    report("  compare(24, 8, 0xAA)", NVRAM.compare(24, ref3, 8));

    // --- Step 3: clear and confirm zeros ---
    Serial.println(F("Step 3: clear all"));
    NVRAM.clear();

    uint8_t ref4[NVRAM_SIZE];
    memset(ref4, 0x00, NVRAM_SIZE);
    report("  compare(0, 56, 0x00)", NVRAM.compare(0, ref4, NVRAM_SIZE));

    Serial.println(F("\nDone."));
    NVRAM.dump(Serial);
}

void loop() {}

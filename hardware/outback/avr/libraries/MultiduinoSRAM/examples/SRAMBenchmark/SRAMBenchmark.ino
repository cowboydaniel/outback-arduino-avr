/*
 * SRAMBenchmark
 *
 * Measures block read and write throughput on the 23AA02M SRAM.
 * Reports bytes per second and approximate time for a full 256KB pass.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoSRAM.h>

static uint8_t buf[256];

void setup() {
    Serial.begin(115200);
    while (!Serial);

    SRAM.begin();
    Serial.println(F("MultiduinoSRAM — benchmark"));

    // Fill buffer with a pattern
    for (uint16_t i = 0; i < sizeof(buf); i++) buf[i] = (uint8_t)i;

    // --- Write benchmark ---
    const uint32_t totalBytes = 16384UL;  // 16 KB pass (fast enough for Serial)
    uint32_t t0 = micros();
    for (uint32_t off = 0; off < totalBytes; off += sizeof(buf)) {
        SRAM.writeBlock(off, buf, sizeof(buf));
    }
    uint32_t writeUs = micros() - t0;

    // --- Read benchmark ---
    t0 = micros();
    for (uint32_t off = 0; off < totalBytes; off += sizeof(buf)) {
        SRAM.readBlock(off, buf, sizeof(buf));
    }
    uint32_t readUs = micros() - t0;

    float writeKBs = (float)totalBytes / (float)writeUs * 1000.0f;
    float readKBs  = (float)totalBytes / (float)readUs  * 1000.0f;

    Serial.print(F("Write: "));
    Serial.print(writeKBs, 1);
    Serial.println(F(" KB/s"));

    Serial.print(F("Read:  "));
    Serial.print(readKBs, 1);
    Serial.println(F(" KB/s"));

    Serial.print(F("Full 256KB write estimate: ~"));
    Serial.print((uint32_t)(SRAM_SIZE / writeKBs / 1000.0f * 1000UL));
    Serial.println(F(" ms"));
}

void loop() {}

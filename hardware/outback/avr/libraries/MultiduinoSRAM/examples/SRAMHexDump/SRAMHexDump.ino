/*
 * SRAMHexDump
 *
 * Writes a short string to SRAM, then prints a formatted hex + ASCII
 * dump of the first 64 bytes to Serial.
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

    // Write a recognisable string at address 0
    const char* msg = "Hello from Multiduino v2 SRAM!";
    SRAM.writeBlock(0, (const uint8_t*)msg, strlen(msg) + 1);

    // Dump first 64 bytes
    Serial.println(F("SRAM hex dump — first 64 bytes:"));
    SRAM.dump(0, 64, Serial);
}

void loop() {}

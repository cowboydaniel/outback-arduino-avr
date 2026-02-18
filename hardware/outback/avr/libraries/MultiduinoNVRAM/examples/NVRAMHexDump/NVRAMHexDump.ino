/*
  NVRAMHexDump
  ------------
  Fills NVRAM with a recognisable test pattern, then dumps the full
  56-byte NVRAM to Serial in a formatted hex + ASCII table using
  NVRAM.dump().

  The dump format shows:
    - Row address (decimal)
    - Eight hex bytes per row
    - Printable ASCII representation of each byte

  Run this sketch to verify your DS1307 NVRAM is working correctly,
  or as a debugging aid when developing other NVRAM-based sketches.

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM Hex Dump ==="));
    Serial.println();

    // ------------------------------------------------------------------
    // Fill NVRAM with a test pattern: 0x00, 0x01, … 0x37 (55)
    // plus some printable ASCII in the second half so the ASCII
    // column in the dump shows readable characters.
    // ------------------------------------------------------------------
    Serial.println(F("Writing test pattern to NVRAM..."));
    for (uint8_t i = 0; i < 28; i++) {
        NVRAM.write(i, i);                 // 0x00 – 0x1B in first half
    }
    // Second half: printable ASCII starting from 'A'
    for (uint8_t i = 28; i < 56; i++) {
        NVRAM.write(i, 'A' + (i - 28));   // 'A' 'B' 'C' … 'z' …
    }

    Serial.println(F("Done. Dumping NVRAM:"));
    Serial.println();

    // ------------------------------------------------------------------
    // Dump the whole NVRAM
    // ------------------------------------------------------------------
    NVRAM.dump(Serial);

    Serial.println();
    Serial.println(F("Dump complete."));
}

void loop() {
    // Nothing – all work done in setup()
}

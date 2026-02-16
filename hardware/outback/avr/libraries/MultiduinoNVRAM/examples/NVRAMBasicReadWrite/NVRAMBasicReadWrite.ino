/*
  NVRAMBasicReadWrite
  -------------------
  Demonstrates the simplest NVRAM operations: writing and reading back
  individual bytes and 16-bit integers using the MultiduinoNVRAM library.

  The DS1307 NVRAM is battery-backed, so values written here survive
  power-off, reset, and brownout events.

  NVRAM address space: 0 – 55 (56 bytes total).

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    // MultiduinoRTC must be initialised first – it owns the Wire bus.
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM Basic Read/Write ==="));
    Serial.println();

    // ------------------------------------------------------------------
    // 1. Write individual bytes
    // ------------------------------------------------------------------
    Serial.println(F("Writing single bytes to addresses 0–4..."));
    for (uint8_t i = 0; i < 5; i++) {
        NVRAM.write(i, 0xA0 + i);
    }

    // Read them back
    Serial.println(F("Reading back addresses 0–4:"));
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t val = NVRAM.read(i);
        Serial.print(F("  addr "));
        Serial.print(i);
        Serial.print(F(" = 0x"));
        if (val < 0x10) Serial.print('0');
        Serial.println(val, HEX);
    }
    Serial.println();

    // ------------------------------------------------------------------
    // 2. Write a 16-bit unsigned integer using put/get
    // ------------------------------------------------------------------
    uint16_t writeVal = 0x1234;
    Serial.print(F("Writing uint16_t 0x"));
    Serial.print(writeVal, HEX);
    Serial.println(F(" to addr 10 (2 bytes)..."));
    NVRAM.put(10, writeVal);

    uint16_t readVal = 0;
    NVRAM.get(10, readVal);
    Serial.print(F("  Read back: 0x"));
    Serial.println(readVal, HEX);
    Serial.println();

    // ------------------------------------------------------------------
    // 3. Write a 32-bit unsigned integer
    // ------------------------------------------------------------------
    uint32_t bigVal = 0xDEADBEEFUL;
    Serial.print(F("Writing uint32_t 0x"));
    Serial.print(bigVal, HEX);
    Serial.println(F(" to addr 20 (4 bytes)..."));
    NVRAM.put(20, bigVal);

    uint32_t bigRead = 0;
    NVRAM.get(20, bigRead);
    Serial.print(F("  Read back: 0x"));
    Serial.println(bigRead, HEX);
    Serial.println();

    // ------------------------------------------------------------------
    // 4. Block write / read
    // ------------------------------------------------------------------
    uint8_t pattern[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    Serial.println(F("Writing 8-byte block to addr 30..."));
    NVRAM.writeBlock(30, pattern, sizeof(pattern));

    uint8_t rxBuf[8] = {};
    NVRAM.readBlock(30, rxBuf, sizeof(rxBuf));
    Serial.print(F("  Read back:"));
    for (uint8_t i = 0; i < 8; i++) {
        Serial.print(F(" 0x"));
        if (rxBuf[i] < 0x10) Serial.print('0');
        Serial.print(rxBuf[i], HEX);
    }
    Serial.println();
    Serial.println();

    Serial.println(F("Done. Open Serial Monitor to see results."));
}

void loop() {
    // Nothing – all work done in setup()
}

/*
  NVRAMCRCRoundtrip
  =================
  Writes a data block to NVRAM, appends a CRC byte, then reads it back and
  verifies integrity.  Also demonstrates the layout-tag API.

  Sequence:
    1. Write version tag (layout) at addr 0.
    2. Write a payload block at addr 2.
    3. Store a CRC of the payload at the byte following the payload.
    4. Read everything back and verify.
    5. Corrupt one byte in NVRAM and show the CRC catches it.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

#define LAYOUT_ADDR  0   // 2 bytes: version + CRC
#define DATA_ADDR    2   // payload starts here
#define DATA_LEN    10   // 10 bytes of payload
#define CRC_ADDR    (DATA_ADDR + DATA_LEN)  // 1 byte CRC

static const uint8_t LAYOUT_VERSION = 0x03;

static const uint8_t PAYLOAD[DATA_LEN] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xAA, 0x55
};

static void report(const char* label, bool ok) {
    Serial.print(label); Serial.println(ok ? F(": OK") : F(": FAIL"));
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    Serial.println(F("=== NVRAMCRCRoundtrip ===\n"));

    // 1. Write layout tag
    NVRAM.writeLayout(LAYOUT_ADDR, LAYOUT_VERSION);
    report("Layout written", NVRAM.checkLayout(LAYOUT_ADDR, LAYOUT_VERSION));

    // 2+3. Write payload + CRC
    NVRAM.writeBlock(DATA_ADDR, PAYLOAD, DATA_LEN);
    uint8_t crc = NVRAM.crc8(DATA_ADDR, DATA_LEN);
    NVRAM.write(CRC_ADDR, crc);
    Serial.print(F("CRC stored: 0x"));
    if (crc < 0x10) Serial.print('0');
    Serial.println(crc, HEX);

    // 4. Verify readback
    uint8_t readback[DATA_LEN];
    NVRAM.readBlock(DATA_ADDR, readback, DATA_LEN);
    bool match = (memcmp(readback, PAYLOAD, DATA_LEN) == 0);
    report("Payload matches", match);

    uint8_t crcCheck = NVRAM.crc8(DATA_ADDR, DATA_LEN);
    uint8_t crcStored = NVRAM.read(CRC_ADDR);
    report("CRC valid      ", crcCheck == crcStored);

    // 5. Corrupt one byte and show CRC detects it
    Serial.println(F("\nCorrupting byte at DATA_ADDR+5..."));
    NVRAM.write(DATA_ADDR + 5, 0xFF);
    uint8_t crcAfter = NVRAM.crc8(DATA_ADDR, DATA_LEN);
    report("CRC mismatch detected", crcAfter != crcStored);

    Serial.println(F("\nFinal dump:"));
    NVRAM.dump(LAYOUT_ADDR, CRC_ADDR - LAYOUT_ADDR + 1, Serial);
}

void loop() {}

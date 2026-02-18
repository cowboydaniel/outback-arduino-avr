#pragma once
#include <Arduino.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// MultiduinoEEPROM — 32Kbit (4KB) I2C EEPROM with factory serial number
//
// On-board I2C addresses:
//   0x50 — data memory (4096 bytes, addresses 0x0000–0x0FFF)
//   0x58 — 64-bit factory unique serial number (read-only, 8 bytes)
//
// Page size: 32 bytes (writes that cross a page boundary are split).
// Write cycle time: 5 ms.  Reads are immediate.
// Endurance: ~1 million write cycles — use MultiduinoFRAM for frequent writes.
//
// The device is on the 3.3V rail.  Call MultiduinoPower.begin() before use.
//
// Global instance name: ExtEEPROM  (avoids conflict with AVR EEPROM.h)
//
// Multiduino v2 only.
// ---------------------------------------------------------------------------

#define EEPROM_ADDR        0x50
#define EEPROM_SERIAL_ADDR 0x58
#define EEPROM_SIZE        4096U
#define EEPROM_PAGE_SIZE   32
#define EEPROM_WRITE_DELAY_MS 5

class MultiduinoEEPROMClass {
public:
    // Call once in setup(). Returns false if the EEPROM does not respond.
    bool begin();

    // -----------------------------------------------------------------------
    // Single-byte access
    // -----------------------------------------------------------------------

    uint8_t read (uint16_t addr);
    void    write(uint16_t addr, uint8_t value);

    // -----------------------------------------------------------------------
    // Block access
    // -----------------------------------------------------------------------

    uint16_t readBlock (uint16_t addr, uint8_t* buf, uint16_t len);
    void     writeBlock(uint16_t addr, const uint8_t* buf, uint16_t len);

    // Write up to EEPROM_PAGE_SIZE bytes aligned to a page boundary.
    // Caller is responsible for alignment; misaligned page writes will wrap.
    void writePage(uint16_t addr, const uint8_t* buf, uint8_t len);

    // -----------------------------------------------------------------------
    // Typed (template) access
    // -----------------------------------------------------------------------
    template<typename T>
    void put(uint16_t addr, const T& value) {
        writeBlock(addr,
            reinterpret_cast<const uint8_t*>(&value),
            (uint16_t)sizeof(T));
    }

    template<typename T>
    void get(uint16_t addr, T& value) {
        readBlock(addr,
            reinterpret_cast<uint8_t*>(&value),
            (uint16_t)sizeof(T));
    }

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    void fill(uint16_t addr, uint16_t len, uint8_t value);
    void clear();   // set all 4096 bytes to 0xFF (EEPROM erased state)
    uint16_t size() const { return EEPROM_SIZE; }

    // -----------------------------------------------------------------------
    // Factory serial number (8 bytes, read-only from address 0x58)
    // -----------------------------------------------------------------------

    // Read 8-byte serial number into buf (must be at least 8 bytes).
    // Returns true on success.
    bool readSerialNumber(uint8_t* buf);

    // Print the serial number as a colon-separated hex string to out.
    // E.g.:  "A1:B2:C3:D4:E5:F6:07:08"
    void printSerialNumber(Print& out);

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------
    void dump(uint16_t addr, uint16_t len, Print& out);

private:
    void     sendAddr(uint16_t addr);
    void     waitWrite();
};

extern MultiduinoEEPROMClass ExtEEPROM;

// ---------------------------------------------------------------------------
// EEPROMSlot<T>
// ---------------------------------------------------------------------------
template<typename T>
class EEPROMSlot {
public:
    explicit EEPROMSlot(uint16_t addr) : _addr(addr) {}

    void save(const T& value) { ExtEEPROM.put(_addr, value); }
    void load(T& value)       { ExtEEPROM.get(_addr, value); }
    T    load()               { T v; ExtEEPROM.get(_addr, v); return v; }

    uint16_t addr() const { return _addr; }
    uint16_t size() const { return (uint16_t)sizeof(T); }

private:
    uint16_t _addr;
};

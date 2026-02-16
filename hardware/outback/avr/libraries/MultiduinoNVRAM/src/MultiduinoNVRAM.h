#pragma once
#include <Arduino.h>
#include <MultiduinoRTC.h>

// ---------------------------------------------------------------------------
// MultiduinoNVRAM
//
// High-level API for the 56-byte battery-backed NVRAM inside the DS1307 RTC.
// The DS1307 retains NVRAM contents while the coin cell is present, surviving
// power-off, reset, and brownout events.
//
// Address space: 0 – 55 (DS1307_NVRAM_SIZE - 1)
//
// Depends on MultiduinoRTC (which owns the Wire bus and the DS1307 driver).
// Always call MultiduinoRTC.begin() before using this library.
// ---------------------------------------------------------------------------

#define NVRAM_SIZE       56   // Total NVRAM bytes in the DS1307
#define NVRAM_CRC_POLY   0x07 // CRC-8 (CCITT) polynomial

class MultiduinoNVRAMClass {
public:
    // -----------------------------------------------------------------------
    // Single-byte access
    // -----------------------------------------------------------------------

    // Write a single byte to NVRAM at addr (0–55).
    void write(uint8_t addr, uint8_t value);

    // Read a single byte from NVRAM at addr (0–55).
    uint8_t read(uint8_t addr);

    // -----------------------------------------------------------------------
    // Typed (template) access  –  put/get any POD type
    //
    // Usage:
    //   uint32_t counter;
    //   NVRAM.put(0, counter);    // writes sizeof(counter) bytes at addr 0
    //   NVRAM.get(0, counter);    // reads  sizeof(counter) bytes from addr 0
    //
    // addr + sizeof(T) must not exceed NVRAM_SIZE (56).
    // -----------------------------------------------------------------------

    template<typename T>
    void put(uint8_t addr, const T& value) {
        MultiduinoRTC.writeNVRAM(addr,
            reinterpret_cast<const uint8_t*>(&value),
            (uint8_t)sizeof(T));
    }

    template<typename T>
    void get(uint8_t addr, T& value) {
        MultiduinoRTC.readNVRAM(addr,
            reinterpret_cast<uint8_t*>(&value),
            (uint8_t)sizeof(T));
    }

    // -----------------------------------------------------------------------
    // String helpers
    // -----------------------------------------------------------------------

    // Write a null-terminated C string starting at addr.
    // Writes at most (NVRAM_SIZE - addr) bytes including the terminator.
    // The string is always null-terminated in NVRAM.
    void putString(uint8_t addr, const char* str);

    // Read a null-terminated C string from addr into buf (capacity maxLen).
    // buf is always null-terminated on return.
    void getString(uint8_t addr, char* buf, uint8_t maxLen);

    // -----------------------------------------------------------------------
    // Block access
    // -----------------------------------------------------------------------

    // Write len bytes from buf starting at addr.
    void writeBlock(uint8_t addr, const uint8_t* buf, uint8_t len);

    // Read len bytes into buf starting at addr.
    uint8_t readBlock(uint8_t addr, uint8_t* buf, uint8_t len);

    // -----------------------------------------------------------------------
    // Maintenance
    // -----------------------------------------------------------------------

    // Zero all 56 NVRAM bytes.
    void clear();

    // Fill all 56 NVRAM bytes with the given fill value.
    void fill(uint8_t value);

    // -----------------------------------------------------------------------
    // Integrity helpers
    // -----------------------------------------------------------------------

    // Compute CRC-8 (CCITT, poly 0x07) over len bytes starting at addr.
    // Useful for detecting NVRAM corruption across power cycles.
    uint8_t crc8(uint8_t addr, uint8_t len);

    // Write a one-byte "magic marker" to addr and its CRC to addr+1.
    // Use checkMagic() on next boot to verify NVRAM has not been corrupted
    // or cleared since the marker was written.
    void writeMagic(uint8_t addr, uint8_t magic);

    // Returns true if the byte at addr equals magic AND addr+1 contains the
    // expected CRC of that magic byte.  Returns false after a clear(), a
    // battery swap, or if the marker was never written.
    bool checkMagic(uint8_t addr, uint8_t magic);

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    // Print a formatted hex + ASCII dump of all 56 NVRAM bytes to 'out'.
    void dump(Print& out);
};

extern MultiduinoNVRAMClass NVRAM;

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
    void putString(uint8_t addr, const char* str);

    // Read a null-terminated C string from addr into buf (capacity maxLen).
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

    // Fill all 56 NVRAM bytes with value.
    void fill(uint8_t value);

    // Fill a sub-range [addr, addr+len) with value.
    // Silently clamps to NVRAM_SIZE.
    void fill(uint8_t addr, uint8_t len, uint8_t value);

    // -----------------------------------------------------------------------
    // Compare
    // -----------------------------------------------------------------------

    // Returns true when the len NVRAM bytes starting at addr exactly match buf.
    bool compare(uint8_t addr, const uint8_t* buf, uint8_t len);

    // -----------------------------------------------------------------------
    // Layout / version tagging
    // -----------------------------------------------------------------------
    //
    // Write a 1-byte version number to addr and a CRC of it to addr+1.
    // checkLayout() returns true only when both bytes still match.
    // Useful for detecting a first-boot (un-initialised NVRAM) or an
    // application upgrade that changed the NVRAM map.
    //
    // Consumes 2 bytes at addr.

    void    writeLayout(uint8_t addr, uint8_t version);
    uint8_t readLayout (uint8_t addr);
    bool    checkLayout(uint8_t addr, uint8_t version);

    // -----------------------------------------------------------------------
    // Integrity helpers
    // -----------------------------------------------------------------------

    // Compute CRC-8 (CCITT, poly 0x07) over len bytes starting at addr.
    uint8_t crc8(uint8_t addr, uint8_t len);

    // Write a one-byte "magic marker" to addr and its CRC to addr+1.
    void writeMagic(uint8_t addr, uint8_t magic);

    // Returns true if addr == magic and addr+1 contains the expected CRC.
    bool checkMagic(uint8_t addr, uint8_t magic);

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    // Print a formatted hex + ASCII dump of all 56 NVRAM bytes to 'out'.
    void dump(Print& out);

    // Print a formatted hex + ASCII dump of a sub-range [addr, addr+len).
    void dump(uint8_t addr, uint8_t len, Print& out);
};

extern MultiduinoNVRAMClass NVRAM;

// ---------------------------------------------------------------------------
// NVRAMSlot<T>  –  typed named slot with automatic address management
//
// Usage:
//   NVRAMSlot<uint32_t> counter(8);  // lives at NVRAM byte 8 (4 bytes)
//   counter.save(42);
//   uint32_t v = counter.load();
// ---------------------------------------------------------------------------
template<typename T>
class NVRAMSlot {
public:
    explicit NVRAMSlot(uint8_t addr) : _addr(addr) {}

    void save(const T& value) { NVRAM.put(_addr, value); }
    T    load()               { T v; NVRAM.get(_addr, v); return v; }
    void load(T& value)       { NVRAM.get(_addr, value); }

    uint8_t addr()  const { return _addr; }
    uint8_t size()  const { return (uint8_t)sizeof(T); }

private:
    uint8_t _addr;
};

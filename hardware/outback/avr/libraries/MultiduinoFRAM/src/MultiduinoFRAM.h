#pragma once
#include <Arduino.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// MultiduinoFRAM — 64Kbit (8KB) ferroelectric RAM driver for Multiduino v2
//
// On-board device: I2C address 0x57.
// Capacity: 8192 bytes (addresses 0x0000–0x1FFF).
//
// FRAM writes are non-volatile and instant — no write delay, no wear limit
// (rated 100 trillion write cycles).  Ideal for high-frequency logging or
// configuration storage.  Use MultiduinoEEPROM if you need longer-term
// archival storage with lower write frequency.
//
// The device is on the 3.3V rail.  Call MultiduinoPower.begin() before use.
//
// The AVR Wire library buffers up to 32 bytes per transaction (including the
// 2-byte address prefix), leaving 30 bytes of payload per write call.
// Larger block writes are automatically split.
//
// Multiduino v2 only.
// ---------------------------------------------------------------------------

#define FRAM_ADDR  0x57
#define FRAM_SIZE  8192U

class MultiduinoFRAMClass {
public:
    // Call once in setup(). Returns false if the FRAM does not respond.
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

    // -----------------------------------------------------------------------
    // Typed (template) access
    //
    //   FRAM.put(0, myStruct);
    //   FRAM.get(0, myStruct);
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
    void clear();   // zero all 8192 bytes
    uint16_t size() const { return FRAM_SIZE; }

    // Formatted hex + ASCII dump (16 bytes/row) to out.
    void dump(uint16_t addr, uint16_t len, Print& out);
};

extern MultiduinoFRAMClass FRAM;

// ---------------------------------------------------------------------------
// FRAMSlot<T> — typed named slot at a fixed address
// ---------------------------------------------------------------------------
template<typename T>
class FRAMSlot {
public:
    explicit FRAMSlot(uint16_t addr) : _addr(addr) {}

    void save(const T& value) { FRAM.put(_addr, value); }
    void load(T& value)       { FRAM.get(_addr, value); }
    T    load()               { T v; FRAM.get(_addr, v); return v; }

    uint16_t addr() const { return _addr; }
    uint16_t size() const { return (uint16_t)sizeof(T); }

private:
    uint16_t _addr;
};

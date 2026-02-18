#pragma once
#include <Arduino.h>
#include <SPI.h>

// ---------------------------------------------------------------------------
// MultiduinoSRAM — Microchip 23AA02M 2Mbit (256KB) SPI SRAM driver
//
// On-board connection (Multiduino v2, verified from KiCad netlist):
//   CS  — D9  / PB1  (PIN_SPI_SS_SRAM, net "CS")
//   SCK — D13 / PB5
//   SI  — D11 / PB3  (MOSI)
//   SO  — D12 / PB4  (MISO)
//
// The 23AA02M supports three operating modes selected via the Mode Register:
//   Byte mode       (0x00) — each command accesses exactly one byte
//   Page mode       (0x80) — burst within a 32-byte page
//   Sequential mode (0x40) — address auto-increments across the whole array
//
// This driver uses Sequential mode for all block transfers (read/write).
// Single-byte operations use Byte mode.
//
// Multiduino v2 only.  The SRAM chip is not present on the original Multiduino.
// ---------------------------------------------------------------------------

// 23AA02M commands
#define SRAM_CMD_READ   0x03
#define SRAM_CMD_WRITE  0x02
#define SRAM_CMD_RDMR   0x05  // read mode register
#define SRAM_CMD_WRMR   0x01  // write mode register

// Mode register values
#define SRAM_MODE_BYTE  0x00
#define SRAM_MODE_PAGE  0x80
#define SRAM_MODE_SEQ   0x40

// CS pin (D9, fixed by hardware)
#define SRAM_CS_PIN     9

// Capacity in bytes (2Mbit = 256KB)
#define SRAM_SIZE       262144UL

class MultiduinoSRAMClass {
public:
    // Call once in setup(). Configures the CS pin and SPI bus.
    void begin();

    // -----------------------------------------------------------------------
    // Single-byte access
    // -----------------------------------------------------------------------

    // Read one byte from addr (0 – SRAM_SIZE-1).
    uint8_t read(uint32_t addr);

    // Write one byte to addr.
    void write(uint32_t addr, uint8_t data);

    // -----------------------------------------------------------------------
    // Block access
    // -----------------------------------------------------------------------

    // Read len bytes starting at addr into buf.
    void readBlock(uint32_t addr, uint8_t* buf, uint32_t len);

    // Write len bytes from buf starting at addr.
    void writeBlock(uint32_t addr, const uint8_t* buf, uint32_t len);

    // -----------------------------------------------------------------------
    // Typed (template) access — put/get any POD type
    //
    //   SRAM.put(0, myStruct);   // writes sizeof(myStruct) bytes at addr 0
    //   SRAM.get(0, myStruct);   // reads  sizeof(myStruct) bytes from addr 0
    // -----------------------------------------------------------------------
    template<typename T>
    void put(uint32_t addr, const T& value) {
        writeBlock(addr,
            reinterpret_cast<const uint8_t*>(&value),
            (uint32_t)sizeof(T));
    }

    template<typename T>
    void get(uint32_t addr, T& value) {
        readBlock(addr,
            reinterpret_cast<uint8_t*>(&value),
            (uint32_t)sizeof(T));
    }

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    // Fill len bytes starting at addr with value.
    void fill(uint32_t addr, uint32_t len, uint8_t value);

    // Zero the entire SRAM array (262144 bytes). Takes ~300 ms at 8 MHz SPI.
    void clear();

    // Return SRAM capacity in bytes (always SRAM_SIZE).
    uint32_t size() const { return SRAM_SIZE; }

    // Print a formatted hex + ASCII dump of len bytes starting at addr to out.
    // Matches the output style of MultiduinoNVRAM::dump().
    void dump(uint32_t addr, uint32_t len, Print& out);

private:
    void     selectMode(uint8_t mode);
    void     csLow()  { digitalWrite(SRAM_CS_PIN, LOW);  }
    void     csHigh() { digitalWrite(SRAM_CS_PIN, HIGH); }
    void     sendAddr(uint32_t addr);

    static const SPISettings _spiSettings;
};

extern MultiduinoSRAMClass SRAM;

// ---------------------------------------------------------------------------
// SRAMSlot<T>  —  typed named slot at a fixed address
//
// Usage:
//   SRAMSlot<Config> cfg(0);   // 'Config' struct lives at SRAM byte 0
//   cfg.save(myConfig);
//   cfg.load(myConfig);
// ---------------------------------------------------------------------------
template<typename T>
class SRAMSlot {
public:
    explicit SRAMSlot(uint32_t addr) : _addr(addr) {}

    void save(const T& value) { SRAM.put(_addr, value); }
    void load(T& value)       { SRAM.get(_addr, value); }
    T    load()               { T v; SRAM.get(_addr, v); return v; }

    uint32_t addr() const { return _addr; }
    uint32_t size() const { return (uint32_t)sizeof(T); }

private:
    uint32_t _addr;
};

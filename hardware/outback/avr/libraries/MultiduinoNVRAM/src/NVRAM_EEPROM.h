#pragma once
/*
  NVRAM_EEPROM.h  –  EEPROM-compatible shim for MultiduinoNVRAM
  ==============================================================
  Include this header instead of <EEPROM.h> to redirect common EEPROM calls
  to the DS1307 battery-backed NVRAM.

  Only the subset of the Arduino EEPROM API that maps cleanly to the 56-byte
  NVRAM address space is provided:

    EEPROM.read(addr)             → NVRAM.read(addr)
    EEPROM.write(addr, val)       → NVRAM.write(addr, val)
    EEPROM.get(addr, obj)         → NVRAM.get(addr, obj)
    EEPROM.put(addr, obj)         → NVRAM.put(addr, obj)
    EEPROM.length()               → NVRAM_SIZE (56)

  Limitation: EEPROM.update() is mapped to write() because NVRAM has no
  individual-cell write-cycle limit concern (writes are to battery-backed SRAM).

  Usage:
    #include <MultiduinoNVRAM.h>
    #include "NVRAM_EEPROM.h"      // replaces #include <EEPROM.h>
    // ... rest of code unchanged
*/

#include <MultiduinoNVRAM.h>

class NVRAMEEPROMClass {
public:
    uint8_t read(uint8_t addr)                    { return NVRAM.read(addr); }
    void    write(uint8_t addr, uint8_t val)       { NVRAM.write(addr, val); }
    void    update(uint8_t addr, uint8_t val)      { NVRAM.write(addr, val); }
    uint8_t length() const                         { return NVRAM_SIZE; }

    template<typename T>
    T& get(uint8_t addr, T& obj) {
        NVRAM.get(addr, obj);
        return obj;
    }

    template<typename T>
    const T& put(uint8_t addr, const T& obj) {
        NVRAM.put(addr, obj);
        return obj;
    }

    // Array-subscript read: EEPROM[addr]
    uint8_t operator[](uint8_t addr) const { return NVRAM.read(addr); }
};

// Shadow the global EEPROM object with this shim instance.
static NVRAMEEPROMClass EEPROM;

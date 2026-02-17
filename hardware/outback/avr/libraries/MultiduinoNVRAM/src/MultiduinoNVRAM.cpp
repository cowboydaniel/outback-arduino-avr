#include "MultiduinoNVRAM.h"

// ---------------------------------------------------------------------------
// Single-byte access
// ---------------------------------------------------------------------------

void MultiduinoNVRAMClass::write(uint8_t addr, uint8_t value) {
    MultiduinoRTC.writeNVRAM(addr, value);
}

uint8_t MultiduinoNVRAMClass::read(uint8_t addr) {
    return MultiduinoRTC.readNVRAM(addr);
}

// ---------------------------------------------------------------------------
// String helpers
// ---------------------------------------------------------------------------

void MultiduinoNVRAMClass::putString(uint8_t addr, const char* str) {
    if (addr >= NVRAM_SIZE) return;
    uint8_t maxLen = NVRAM_SIZE - addr;
    uint8_t i = 0;
    while (i < maxLen - 1 && str[i] != '\0') {
        MultiduinoRTC.writeNVRAM(addr + i, (uint8_t)str[i]);
        i++;
    }
    MultiduinoRTC.writeNVRAM(addr + i, 0x00);
}

void MultiduinoNVRAMClass::getString(uint8_t addr, char* buf, uint8_t maxLen) {
    if (addr >= NVRAM_SIZE || maxLen == 0) {
        if (buf && maxLen > 0) buf[0] = '\0';
        return;
    }
    uint8_t i = 0;
    while (i < maxLen - 1 && addr + i < NVRAM_SIZE) {
        uint8_t c = MultiduinoRTC.readNVRAM(addr + i);
        if (c == 0x00) break;
        buf[i++] = (char)c;
    }
    buf[i] = '\0';
}

// ---------------------------------------------------------------------------
// Block access
// ---------------------------------------------------------------------------

void MultiduinoNVRAMClass::writeBlock(uint8_t addr, const uint8_t* buf, uint8_t len) {
    MultiduinoRTC.writeNVRAM(addr, buf, len);
}

uint8_t MultiduinoNVRAMClass::readBlock(uint8_t addr, uint8_t* buf, uint8_t len) {
    return MultiduinoRTC.readNVRAM(addr, buf, len);
}

// ---------------------------------------------------------------------------
// Maintenance
// ---------------------------------------------------------------------------

void MultiduinoNVRAMClass::clear() {
    MultiduinoRTC.clearNVRAM();
}

void MultiduinoNVRAMClass::fill(uint8_t value) {
    for (uint8_t i = 0; i < NVRAM_SIZE; i++) {
        MultiduinoRTC.writeNVRAM(i, value);
    }
}

void MultiduinoNVRAMClass::fill(uint8_t addr, uint8_t len, uint8_t value) {
    if (addr >= NVRAM_SIZE) return;
    if ((uint16_t)addr + len > NVRAM_SIZE) len = NVRAM_SIZE - addr;
    for (uint8_t i = 0; i < len; i++) {
        MultiduinoRTC.writeNVRAM(addr + i, value);
    }
}

// ---------------------------------------------------------------------------
// Compare
// ---------------------------------------------------------------------------

bool MultiduinoNVRAMClass::compare(uint8_t addr, const uint8_t* buf, uint8_t len) {
    if (addr >= NVRAM_SIZE) return false;
    if ((uint16_t)addr + len > NVRAM_SIZE) return false;
    for (uint8_t i = 0; i < len; i++) {
        if (MultiduinoRTC.readNVRAM(addr + i) != buf[i]) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Layout / version tagging
// ---------------------------------------------------------------------------

static uint8_t _layoutCrc(uint8_t version) {
    uint8_t crc = 0x00;
    crc ^= version;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (crc & 0x80) crc = (crc << 1) ^ NVRAM_CRC_POLY;
        else            crc <<= 1;
    }
    return crc;
}

void MultiduinoNVRAMClass::writeLayout(uint8_t addr, uint8_t version) {
    if (addr + 1 >= NVRAM_SIZE) return;
    MultiduinoRTC.writeNVRAM(addr,     version);
    MultiduinoRTC.writeNVRAM(addr + 1, _layoutCrc(version));
}

uint8_t MultiduinoNVRAMClass::readLayout(uint8_t addr) {
    if (addr >= NVRAM_SIZE) return 0xFF;
    return MultiduinoRTC.readNVRAM(addr);
}

bool MultiduinoNVRAMClass::checkLayout(uint8_t addr, uint8_t version) {
    if (addr + 1 >= NVRAM_SIZE) return false;
    if (MultiduinoRTC.readNVRAM(addr) != version) return false;
    return MultiduinoRTC.readNVRAM(addr + 1) == _layoutCrc(version);
}

// ---------------------------------------------------------------------------
// Integrity helpers
// ---------------------------------------------------------------------------

uint8_t MultiduinoNVRAMClass::crc8(uint8_t addr, uint8_t len) {
    if (addr >= NVRAM_SIZE) return 0xFF;
    if ((uint16_t)addr + len > NVRAM_SIZE) len = NVRAM_SIZE - addr;

    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = MultiduinoRTC.readNVRAM(addr + i);
        crc ^= byte;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) crc = (crc << 1) ^ NVRAM_CRC_POLY;
            else            crc <<= 1;
        }
    }
    return crc;
}

void MultiduinoNVRAMClass::writeMagic(uint8_t addr, uint8_t magic) {
    if (addr + 1 >= NVRAM_SIZE) return;
    MultiduinoRTC.writeNVRAM(addr, magic);
    uint8_t crc = 0x00;
    crc ^= magic;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (crc & 0x80) crc = (crc << 1) ^ NVRAM_CRC_POLY;
        else            crc <<= 1;
    }
    MultiduinoRTC.writeNVRAM(addr + 1, crc);
}

bool MultiduinoNVRAMClass::checkMagic(uint8_t addr, uint8_t magic) {
    if (addr + 1 >= NVRAM_SIZE) return false;
    uint8_t stored = MultiduinoRTC.readNVRAM(addr);
    if (stored != magic) return false;
    uint8_t crc = 0x00;
    crc ^= magic;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (crc & 0x80) crc = (crc << 1) ^ NVRAM_CRC_POLY;
        else            crc <<= 1;
    }
    return MultiduinoRTC.readNVRAM(addr + 1) == crc;
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

void MultiduinoNVRAMClass::dump(Print& out) {
    dump(0, NVRAM_SIZE, out);
}

void MultiduinoNVRAMClass::dump(uint8_t addr, uint8_t len, Print& out) {
    if (addr >= NVRAM_SIZE) return;
    if ((uint16_t)addr + len > NVRAM_SIZE) len = NVRAM_SIZE - addr;

    uint8_t buf[NVRAM_SIZE];
    MultiduinoRTC.readNVRAM(addr, buf, len);

    out.print(F("NVRAM dump [")); out.print(addr);
    out.print(F("–")); out.print(addr + len - 1); out.println(F("]:"));
    out.println(F("Addr  00 01 02 03 04 05 06 07  ASCII"));
    out.println(F("----  -----------------------  --------"));

    for (uint8_t row = 0; row < len; row += 8) {
        uint8_t absRow = addr + row;
        if (absRow < 10) out.print('0');
        out.print(absRow, DEC);
        out.print(F("    "));

        for (uint8_t col = 0; col < 8; col++) {
            uint8_t idx = row + col;
            if (idx < len) {
                if (buf[idx] < 0x10) out.print('0');
                out.print(buf[idx], HEX);
            } else {
                out.print(F("  "));
            }
            out.print(' ');
        }

        out.print(F(" "));
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t idx = row + col;
            if (idx < len) {
                char c = (char)buf[idx];
                out.print((c >= 0x20 && c < 0x7F) ? c : '.');
            }
        }
        out.println();
    }
}

// Global instance
MultiduinoNVRAMClass NVRAM;

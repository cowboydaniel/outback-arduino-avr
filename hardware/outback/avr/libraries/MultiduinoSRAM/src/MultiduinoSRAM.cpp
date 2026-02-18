#include "MultiduinoSRAM.h"

// 23AA02M max SPI clock: 20 MHz.  Use 8 MHz to stay within avr-gcc SPI limits
// for boards running at 16 MHz (F_CPU/2 = 8 MHz).
const SPISettings MultiduinoSRAMClass::_spiSettings(8000000, MSBFIRST, SPI_MODE0);

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void MultiduinoSRAMClass::sendAddr(uint32_t addr) {
    // 23AA02M uses a 24-bit address field (3 bytes, MSB first).
    // Only the lower 18 bits are significant for the 256KB device.
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >>  8));
    SPI.transfer((uint8_t)(addr      ));
}

void MultiduinoSRAMClass::selectMode(uint8_t mode) {
    csLow();
    SPI.transfer(SRAM_CMD_WRMR);
    SPI.transfer(mode);
    csHigh();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void MultiduinoSRAMClass::begin() {
    pinMode(SRAM_CS_PIN, OUTPUT);
    csHigh();
    SPI.begin();
    // Default to sequential mode so block transfers need no per-byte overhead.
    SPI.beginTransaction(_spiSettings);
    selectMode(SRAM_MODE_SEQ);
    SPI.endTransaction();
}

uint8_t MultiduinoSRAMClass::read(uint32_t addr) {
    SPI.beginTransaction(_spiSettings);
    csLow();
    SPI.transfer(SRAM_CMD_READ);
    sendAddr(addr);
    uint8_t data = SPI.transfer(0x00);
    csHigh();
    SPI.endTransaction();
    return data;
}

void MultiduinoSRAMClass::write(uint32_t addr, uint8_t data) {
    SPI.beginTransaction(_spiSettings);
    csLow();
    SPI.transfer(SRAM_CMD_WRITE);
    sendAddr(addr);
    SPI.transfer(data);
    csHigh();
    SPI.endTransaction();
}

void MultiduinoSRAMClass::readBlock(uint32_t addr, uint8_t* buf, uint32_t len) {
    if (!len) return;
    SPI.beginTransaction(_spiSettings);
    csLow();
    SPI.transfer(SRAM_CMD_READ);
    sendAddr(addr);
    for (uint32_t i = 0; i < len; i++) buf[i] = SPI.transfer(0x00);
    csHigh();
    SPI.endTransaction();
}

void MultiduinoSRAMClass::writeBlock(uint32_t addr, const uint8_t* buf, uint32_t len) {
    if (!len) return;
    SPI.beginTransaction(_spiSettings);
    csLow();
    SPI.transfer(SRAM_CMD_WRITE);
    sendAddr(addr);
    for (uint32_t i = 0; i < len; i++) SPI.transfer(buf[i]);
    csHigh();
    SPI.endTransaction();
}

void MultiduinoSRAMClass::fill(uint32_t addr, uint32_t len, uint8_t value) {
    if (!len) return;
    SPI.beginTransaction(_spiSettings);
    csLow();
    SPI.transfer(SRAM_CMD_WRITE);
    sendAddr(addr);
    for (uint32_t i = 0; i < len; i++) SPI.transfer(value);
    csHigh();
    SPI.endTransaction();
}

void MultiduinoSRAMClass::clear() {
    fill(0, SRAM_SIZE, 0x00);
}

void MultiduinoSRAMClass::dump(uint32_t addr, uint32_t len, Print& out) {
    const uint8_t cols = 16;
    uint8_t row[cols];

    for (uint32_t offset = 0; offset < len; offset += cols) {
        uint8_t n = (uint8_t)((len - offset) < cols ? (len - offset) : cols);
        readBlock(addr + offset, row, n);

        // Address
        char buf[12];
        snprintf(buf, sizeof(buf), "%06lX  ", (unsigned long)(addr + offset));
        out.print(buf);

        // Hex bytes
        for (uint8_t i = 0; i < cols; i++) {
            if (i < n) {
                snprintf(buf, sizeof(buf), "%02X ", row[i]);
                out.print(buf);
            } else {
                out.print(F("   "));
            }
            if (i == 7) out.print(' ');
        }
        out.print(F(" |"));

        // ASCII
        for (uint8_t i = 0; i < n; i++) {
            out.print((char)(row[i] >= 0x20 && row[i] < 0x7F ? row[i] : '.'));
        }
        out.println('|');
    }
}

// Global instance
MultiduinoSRAMClass SRAM;

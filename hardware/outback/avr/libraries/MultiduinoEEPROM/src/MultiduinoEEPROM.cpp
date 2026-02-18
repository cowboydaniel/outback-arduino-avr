#include "MultiduinoEEPROM.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void MultiduinoEEPROMClass::sendAddr(uint16_t addr) {
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr     ));
}

void MultiduinoEEPROMClass::waitWrite() {
    // Poll ACK — the device NAKs while an internal write cycle is in progress.
    // Fall back to a fixed delay if the device doesn't respond quickly.
    uint8_t retries = 0;
    do {
        Wire.beginTransmission(EEPROM_ADDR);
        if (Wire.endTransmission() == 0) return;
        delay(1);
    } while (++retries < EEPROM_WRITE_DELAY_MS + 2);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool MultiduinoEEPROMClass::begin() {
    Wire.begin();
    Wire.beginTransmission(EEPROM_ADDR);
    sendAddr(0x0000);
    return Wire.endTransmission() == 0;
}

uint8_t MultiduinoEEPROMClass::read(uint16_t addr) {
    Wire.beginTransmission(EEPROM_ADDR);
    sendAddr(addr);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)EEPROM_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void MultiduinoEEPROMClass::write(uint16_t addr, uint8_t value) {
    Wire.beginTransmission(EEPROM_ADDR);
    sendAddr(addr);
    Wire.write(value);
    Wire.endTransmission();
    waitWrite();
}

uint16_t MultiduinoEEPROMClass::readBlock(uint16_t addr, uint8_t* buf, uint16_t len) {
    // Wire can transfer up to 30 bytes of payload reliably (32 - 2 addr bytes)
    const uint8_t chunkSize = 30;
    uint16_t received = 0;
    while (received < len) {
        uint8_t chunk = (uint8_t)((len - received) > chunkSize ? chunkSize : (len - received));
        Wire.beginTransmission(EEPROM_ADDR);
        sendAddr((uint16_t)(addr + received));
        Wire.endTransmission();
        uint8_t got = Wire.requestFrom((uint8_t)EEPROM_ADDR, chunk);
        for (uint8_t i = 0; i < got; i++) buf[received + i] = Wire.read();
        received += got;
        if (got < chunk) break;
    }
    return received;
}

void MultiduinoEEPROMClass::writePage(uint16_t addr, const uint8_t* buf, uint8_t len) {
    if (len > EEPROM_PAGE_SIZE) len = EEPROM_PAGE_SIZE;
    Wire.beginTransmission(EEPROM_ADDR);
    sendAddr(addr);
    for (uint8_t i = 0; i < len; i++) Wire.write(buf[i]);
    Wire.endTransmission();
    waitWrite();
}

void MultiduinoEEPROMClass::writeBlock(uint16_t addr, const uint8_t* buf, uint16_t len) {
    uint16_t written = 0;
    while (written < len) {
        // Calculate remaining space in the current page
        uint16_t pageEnd  = ((addr + written) / EEPROM_PAGE_SIZE + 1) * EEPROM_PAGE_SIZE;
        uint16_t pageRoom = pageEnd - (addr + written);
        uint8_t  chunk    = (uint8_t)((len - written) < pageRoom ? (len - written) : pageRoom);
        if (chunk > EEPROM_PAGE_SIZE) chunk = EEPROM_PAGE_SIZE;

        writePage((uint16_t)(addr + written), buf + written, chunk);
        written += chunk;
    }
}

void MultiduinoEEPROMClass::fill(uint16_t addr, uint16_t len, uint8_t value) {
    uint8_t page[EEPROM_PAGE_SIZE];
    memset(page, value, EEPROM_PAGE_SIZE);
    uint16_t done = 0;
    while (done < len) {
        uint16_t pageEnd  = ((addr + done) / EEPROM_PAGE_SIZE + 1) * EEPROM_PAGE_SIZE;
        uint16_t pageRoom = pageEnd - (addr + done);
        uint8_t  chunk    = (uint8_t)((len - done) < pageRoom ? (len - done) : pageRoom);

        Wire.beginTransmission(EEPROM_ADDR);
        sendAddr((uint16_t)(addr + done));
        for (uint8_t i = 0; i < chunk; i++) Wire.write(value);
        Wire.endTransmission();
        waitWrite();
        done += chunk;
    }
}

void MultiduinoEEPROMClass::clear() {
    fill(0, EEPROM_SIZE, 0xFF);
}

// ---------------------------------------------------------------------------
// Serial number (8 bytes at I2C address 0x58)
// ---------------------------------------------------------------------------

bool MultiduinoEEPROMClass::readSerialNumber(uint8_t* buf) {
    // The unique serial number lives at I2C address 0x58, register 0x0000.
    Wire.beginTransmission(EEPROM_SERIAL_ADDR);
    Wire.write(0x00);  // address MSB
    Wire.write(0x00);  // address LSB
    if (Wire.endTransmission() != 0) return false;
    uint8_t got = Wire.requestFrom((uint8_t)EEPROM_SERIAL_ADDR, (uint8_t)8);
    for (uint8_t i = 0; i < got; i++) buf[i] = Wire.read();
    return got == 8;
}

void MultiduinoEEPROMClass::printSerialNumber(Print& out) {
    uint8_t sn[8];
    if (!readSerialNumber(sn)) {
        out.println(F("(read failed)"));
        return;
    }
    char buf[4];
    for (uint8_t i = 0; i < 8; i++) {
        snprintf(buf, sizeof(buf), "%02X", sn[i]);
        out.print(buf);
        if (i < 7) out.print(':');
    }
    out.println();
}

// ---------------------------------------------------------------------------
// Dump
// ---------------------------------------------------------------------------

void MultiduinoEEPROMClass::dump(uint16_t addr, uint16_t len, Print& out) {
    const uint8_t cols = 16;
    uint8_t row[16];
    for (uint16_t offset = 0; offset < len; offset += cols) {
        uint8_t n = (uint8_t)((len - offset) < cols ? (len - offset) : cols);
        readBlock((uint16_t)(addr + offset), row, n);

        char buf[10];
        snprintf(buf, sizeof(buf), "%04X  ", (unsigned)(addr + offset));
        out.print(buf);

        for (uint8_t i = 0; i < cols; i++) {
            if (i < n) { snprintf(buf, sizeof(buf), "%02X ", row[i]); out.print(buf); }
            else        out.print(F("   "));
            if (i == 7) out.print(' ');
        }
        out.print(F(" |"));
        for (uint8_t i = 0; i < n; i++)
            out.print((char)(row[i] >= 0x20 && row[i] < 0x7F ? row[i] : '.'));
        out.println('|');
    }
}

// Global instance
MultiduinoEEPROMClass ExtEEPROM;

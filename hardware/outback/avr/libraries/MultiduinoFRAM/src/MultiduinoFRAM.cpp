#include "MultiduinoFRAM.h"

// The AVR Wire buffer is 32 bytes.  2 bytes are consumed by the address prefix,
// leaving 30 bytes of payload per transaction.
static const uint8_t FRAM_MAX_WRITE = 30;
static const uint8_t FRAM_MAX_READ  = 30;  // keep reads small too for reliability

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static void sendAddr(uint16_t addr) {
    Wire.write((uint8_t)(addr >> 8));
    Wire.write((uint8_t)(addr     ));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool MultiduinoFRAMClass::begin() {
    Wire.begin();
    Wire.beginTransmission(FRAM_ADDR);
    sendAddr(0x0000);
    return Wire.endTransmission() == 0;
}

uint8_t MultiduinoFRAMClass::read(uint16_t addr) {
    Wire.beginTransmission(FRAM_ADDR);
    sendAddr(addr);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)FRAM_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void MultiduinoFRAMClass::write(uint16_t addr, uint8_t value) {
    Wire.beginTransmission(FRAM_ADDR);
    sendAddr(addr);
    Wire.write(value);
    Wire.endTransmission();
}

uint16_t MultiduinoFRAMClass::readBlock(uint16_t addr, uint8_t* buf, uint16_t len) {
    uint16_t received = 0;
    while (received < len) {
        uint8_t chunk = (uint8_t)((len - received) > FRAM_MAX_READ ? FRAM_MAX_READ : (len - received));
        Wire.beginTransmission(FRAM_ADDR);
        sendAddr((uint16_t)(addr + received));
        Wire.endTransmission();
        uint8_t got = Wire.requestFrom((uint8_t)FRAM_ADDR, chunk);
        for (uint8_t i = 0; i < got; i++) buf[received + i] = Wire.read();
        received += got;
        if (got < chunk) break;  // device stopped responding
    }
    return received;
}

void MultiduinoFRAMClass::writeBlock(uint16_t addr, const uint8_t* buf, uint16_t len) {
    uint16_t written = 0;
    while (written < len) {
        uint8_t chunk = (uint8_t)((len - written) > FRAM_MAX_WRITE ? FRAM_MAX_WRITE : (len - written));
        Wire.beginTransmission(FRAM_ADDR);
        sendAddr((uint16_t)(addr + written));
        for (uint8_t i = 0; i < chunk; i++) Wire.write(buf[written + i]);
        Wire.endTransmission();
        written += chunk;
        // No write delay — FRAM writes are instant
    }
}

void MultiduinoFRAMClass::fill(uint16_t addr, uint16_t len, uint8_t value) {
    uint16_t done = 0;
    while (done < len) {
        uint8_t chunk = (uint8_t)((len - done) > FRAM_MAX_WRITE ? FRAM_MAX_WRITE : (len - done));
        Wire.beginTransmission(FRAM_ADDR);
        sendAddr((uint16_t)(addr + done));
        for (uint8_t i = 0; i < chunk; i++) Wire.write(value);
        Wire.endTransmission();
        done += chunk;
    }
}

void MultiduinoFRAMClass::clear() {
    fill(0, FRAM_SIZE, 0x00);
}

void MultiduinoFRAMClass::dump(uint16_t addr, uint16_t len, Print& out) {
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
MultiduinoFRAMClass FRAM;

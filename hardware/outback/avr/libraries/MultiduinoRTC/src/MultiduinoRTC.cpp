#include "MultiduinoRTC.h"

// ---------------------------------------------------------------------------
// Low-level register I/O
// ---------------------------------------------------------------------------

uint8_t MultiduinoRTCClass::readReg(uint8_t reg) {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS1307_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void MultiduinoRTCClass::writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool MultiduinoRTCClass::begin() {
    Wire.begin();
    // Probe the DS1307: try to read the seconds register.
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_SEC);
    return Wire.endTransmission() == 0;
}

bool MultiduinoRTCClass::isRunning() {
    // CH bit is bit 7 of the seconds register; 0 = oscillator running.
    return !(readReg(DS1307_REG_SEC) & 0x80);
}

void MultiduinoRTCClass::start() {
    uint8_t sec = readReg(DS1307_REG_SEC);
    writeReg(DS1307_REG_SEC, sec & 0x7F); // clear CH
}

void MultiduinoRTCClass::stop() {
    uint8_t sec = readReg(DS1307_REG_SEC);
    writeReg(DS1307_REG_SEC, sec | 0x80); // set CH
}

DateTime MultiduinoRTCClass::now() {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_SEC);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS1307_ADDR, (uint8_t)7);

    DateTime dt;
    if (Wire.available() >= 7) {
        dt.second = bcdToDec(Wire.read() & 0x7F); // mask off CH bit
        dt.minute = bcdToDec(Wire.read());
        dt.hour   = bcdToDec(Wire.read() & 0x3F); // mask off 12/24 bit
        dt.dow    = Wire.read();                   // 1–7
        dt.day    = bcdToDec(Wire.read());
        dt.month  = bcdToDec(Wire.read());
        dt.year   = 2000U + bcdToDec(Wire.read());
    }
    return dt;
}

void MultiduinoRTCClass::adjust(const DateTime& dt) {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_SEC);
    Wire.write(decToBcd(dt.second) & 0x7F); // CH=0, start oscillator
    Wire.write(decToBcd(dt.minute));
    Wire.write(decToBcd(dt.hour));           // 24-hour mode (bit 6 = 0)
    Wire.write(dt.dow);
    Wire.write(decToBcd(dt.day));
    Wire.write(decToBcd(dt.month));
    Wire.write(decToBcd((uint8_t)(dt.year - 2000)));
    Wire.endTransmission();
}

// ---------------------------------------------------------------------------
// NVRAM
// ---------------------------------------------------------------------------

uint8_t MultiduinoRTCClass::readNVRAM(uint8_t addr) {
    if (addr >= DS1307_NVRAM_SIZE) return 0xFF;
    return readReg(DS1307_REG_NVRAM + addr);
}

void MultiduinoRTCClass::writeNVRAM(uint8_t addr, uint8_t value) {
    if (addr >= DS1307_NVRAM_SIZE) return;
    writeReg(DS1307_REG_NVRAM + addr, value);
}

uint8_t MultiduinoRTCClass::readNVRAM(uint8_t addr, uint8_t* buf, uint8_t len) {
    if (addr >= DS1307_NVRAM_SIZE) return 0;
    if (addr + len > DS1307_NVRAM_SIZE) len = DS1307_NVRAM_SIZE - addr;

    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_NVRAM + addr);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS1307_ADDR, len);

    uint8_t i = 0;
    while (Wire.available() && i < len) buf[i++] = Wire.read();
    return i;
}

void MultiduinoRTCClass::writeNVRAM(uint8_t addr, const uint8_t* buf, uint8_t len) {
    if (addr >= DS1307_NVRAM_SIZE) return;
    if (addr + len > DS1307_NVRAM_SIZE) len = DS1307_NVRAM_SIZE - addr;

    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_NVRAM + addr);
    for (uint8_t i = 0; i < len; i++) Wire.write(buf[i]);
    Wire.endTransmission();
}

void MultiduinoRTCClass::clearNVRAM() {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_NVRAM);
    for (uint8_t i = 0; i < DS1307_NVRAM_SIZE; i++) Wire.write(0x00);
    Wire.endTransmission();
}

// Global instance
MultiduinoRTCClass MultiduinoRTC;

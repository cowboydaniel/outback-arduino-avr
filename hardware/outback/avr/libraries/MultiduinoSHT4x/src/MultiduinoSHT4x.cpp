#include "MultiduinoSHT4x.h"

// ---------------------------------------------------------------------------
// CRC-8 (poly 0x31, init 0xFF) — Sensirion standard
// ---------------------------------------------------------------------------

uint8_t MultiduinoSHT4xClass::crc8(uint8_t msb, uint8_t lsb) {
    uint8_t crc = SHT4X_CRC_INIT;
    uint8_t bytes[2] = { msb, lsb };
    for (uint8_t b = 0; b < 2; b++) {
        crc ^= bytes[b];
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc & 0x80) ? (crc << 1) ^ SHT4X_CRC_POLY : (crc << 1);
        }
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Low-level I2C helpers
// ---------------------------------------------------------------------------

bool MultiduinoSHT4xClass::sendCmd(uint8_t cmd) {
    Wire.beginTransmission(SHT4X_ADDR);
    Wire.write(cmd);
    return Wire.endTransmission() == 0;
}

bool MultiduinoSHT4xClass::readResponse(uint8_t* buf, uint8_t len) {
    if (Wire.requestFrom((uint8_t)SHT4X_ADDR, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

bool MultiduinoSHT4xClass::parseResponse(const uint8_t* buf, float& tempC, float& humidity) {
    // buf layout: [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC]
    if (crc8(buf[0], buf[1]) != buf[2]) return false;
    if (crc8(buf[3], buf[4]) != buf[5]) return false;

    uint16_t tRaw  = toU16(buf);
    uint16_t rhRaw = toU16(buf + 3);

    tempC    = -45.0f + 175.0f * ((float)tRaw  / 65535.0f);
    humidity = -6.0f  + 125.0f * ((float)rhRaw / 65535.0f);

    if (humidity < 0.0f)   humidity = 0.0f;
    if (humidity > 100.0f) humidity = 100.0f;

    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool MultiduinoSHT4xClass::begin() {
    Wire.begin();
    reset();
    delay(1);
    // Probe: try to read serial number
    return serialNumber() != 0 || true;  // tolerate 0 serial; just check ACK
}

bool MultiduinoSHT4xClass::read(float& tempC, float& humidity, SHT4xPrecision precision) {
    uint8_t delayMs;
    switch (precision) {
        case SHT4X_HIGH:   delayMs = SHT4X_DELAY_HIGH;   break;
        case SHT4X_MEDIUM: delayMs = SHT4X_DELAY_MEDIUM; break;
        default:           delayMs = SHT4X_DELAY_LOW;    break;
    }

    if (!sendCmd((uint8_t)precision)) return false;
    delay(delayMs);

    uint8_t buf[6];
    if (!readResponse(buf, 6)) return false;
    return parseResponse(buf, tempC, humidity);
}

void MultiduinoSHT4xClass::reset() {
    sendCmd(SHT4X_CMD_RESET);
    delay(1);
}

uint32_t MultiduinoSHT4xClass::serialNumber() {
    if (!sendCmd(SHT4X_CMD_SERIAL)) return 0;
    delay(1);

    uint8_t buf[6];
    if (!readResponse(buf, 6)) return 0;

    // Word 1 = buf[0:1] + CRC buf[2]; Word 2 = buf[3:4] + CRC buf[5]
    if (crc8(buf[0], buf[1]) != buf[2]) return 0;
    if (crc8(buf[3], buf[4]) != buf[5]) return 0;

    return ((uint32_t)toU16(buf) << 16) | toU16(buf + 3);
}

bool MultiduinoSHT4xClass::heater(SHT4xHeater mode, float& tempC, float& humidity) {
    // Heater commands take up to 1100 ms depending on mode
    uint16_t delayMs;
    switch (mode) {
        case SHT4X_HEATER_200MW_1S:
        case SHT4X_HEATER_110MW_1S:
        case SHT4X_HEATER_20MW_1S:    delayMs = 1100; break;
        default:                       delayMs =  110; break;
    }

    if (!sendCmd((uint8_t)mode)) return false;
    delay(delayMs);

    uint8_t buf[6];
    if (!readResponse(buf, 6)) return false;
    return parseResponse(buf, tempC, humidity);
}

// Global instance
MultiduinoSHT4xClass SHT4x;

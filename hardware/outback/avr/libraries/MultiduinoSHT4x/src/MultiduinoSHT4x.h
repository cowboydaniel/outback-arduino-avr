#pragma once
#include <Arduino.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// MultiduinoSHT4x — Sensirion SHT4x humidity and temperature sensor driver
//
// On-board I2C address: 0x44 (fixed, single device on bus).
// The SHT4x sits on the 3.3V domain — ensure MultiduinoPower.enable3V3() has
// been called (and at least 1 ms allowed to settle) before using this library.
//
// Multiduino v2 only.
// ---------------------------------------------------------------------------

#define SHT4X_ADDR  0x44

// Measurement commands
#define SHT4X_CMD_HIGH     0xFD  // high repeatability (~8.3 ms)
#define SHT4X_CMD_MEDIUM   0xF6  // medium repeatability (~4.5 ms)
#define SHT4X_CMD_LOW      0xE0  // lowest repeatability (~1.7 ms)

// Heater commands (activate heater then immediately measure)
#define SHT4X_CMD_HEAT_200MW_1S    0x39
#define SHT4X_CMD_HEAT_200MW_100MS 0x32
#define SHT4X_CMD_HEAT_110MW_1S    0x2F
#define SHT4X_CMD_HEAT_110MW_100MS 0x24
#define SHT4X_CMD_HEAT_20MW_1S     0x1E
#define SHT4X_CMD_HEAT_20MW_100MS  0x15

// Soft reset
#define SHT4X_CMD_RESET    0x94

// Read serial number
#define SHT4X_CMD_SERIAL   0x89

// Measurement durations (ms) — wait at least this long after sending command
#define SHT4X_DELAY_HIGH    10
#define SHT4X_DELAY_MEDIUM   5
#define SHT4X_DELAY_LOW      2

// CRC-8 polynomial for SHT4x (x^8 + x^5 + x^4 + 1 = 0x31, init 0xFF)
#define SHT4X_CRC_POLY  0x31
#define SHT4X_CRC_INIT  0xFF

enum SHT4xPrecision : uint8_t {
    SHT4X_HIGH   = SHT4X_CMD_HIGH,
    SHT4X_MEDIUM = SHT4X_CMD_MEDIUM,
    SHT4X_LOW    = SHT4X_CMD_LOW,
};

enum SHT4xHeater : uint8_t {
    SHT4X_HEATER_200MW_1S    = SHT4X_CMD_HEAT_200MW_1S,
    SHT4X_HEATER_200MW_100MS = SHT4X_CMD_HEAT_200MW_100MS,
    SHT4X_HEATER_110MW_1S    = SHT4X_CMD_HEAT_110MW_1S,
    SHT4X_HEATER_110MW_100MS = SHT4X_CMD_HEAT_110MW_100MS,
    SHT4X_HEATER_20MW_1S     = SHT4X_CMD_HEAT_20MW_1S,
    SHT4X_HEATER_20MW_100MS  = SHT4X_CMD_HEAT_20MW_100MS,
};

class MultiduinoSHT4xClass {
public:
    // Call once in setup(). Returns false if the SHT4x does not respond.
    bool begin();

    // Take a measurement.
    // tempC    — temperature in degrees Celsius
    // humidity — relative humidity in percent (0.0–100.0)
    // Returns true on success; false if the CRC fails or device doesn't respond.
    // precision: SHT4X_HIGH (default), SHT4X_MEDIUM, or SHT4X_LOW.
    bool read(float& tempC, float& humidity, SHT4xPrecision precision = SHT4X_HIGH);

    // Perform a soft reset (takes ~1 ms).
    void reset();

    // Read the 32-bit factory serial number.  Returns 0 on failure.
    uint32_t serialNumber();

    // Activate the on-chip heater, then take a measurement.
    // The heater is useful for de-condensation or sensor self-test.
    // mode: one of the SHT4xHeater constants.
    // tempC and humidity are filled with the post-heater reading.
    bool heater(SHT4xHeater mode, float& tempC, float& humidity);

private:
    bool    sendCmd(uint8_t cmd);
    bool    readResponse(uint8_t* buf, uint8_t len);
    uint8_t crc8(uint8_t msb, uint8_t lsb);
    bool    parseResponse(const uint8_t* buf, float& tempC, float& humidity);

    static uint16_t toU16(const uint8_t* b) { return ((uint16_t)b[0] << 8) | b[1]; }
};

extern MultiduinoSHT4xClass SHT4x;

#pragma once
#include <Arduino.h>
#include <Wire.h>

// DS1307 I2C address (7-bit, fixed)
#define DS1307_ADDR     0x68

// DS1307 register addresses
#define DS1307_REG_SEC  0x00  // Seconds + Clock Halt (CH) bit
#define DS1307_REG_MIN  0x01  // Minutes
#define DS1307_REG_HOUR 0x02  // Hours (+ 12/24 select)
#define DS1307_REG_DOW  0x03  // Day of week (1–7)
#define DS1307_REG_DATE 0x04  // Day of month
#define DS1307_REG_MON  0x05  // Month
#define DS1307_REG_YEAR 0x06  // Year (00–99, relative to 2000)
#define DS1307_REG_CTRL 0x07  // Control (SQW)
#define DS1307_REG_NVRAM 0x08 // First NVRAM byte (0x08–0x3F)
#define DS1307_NVRAM_SIZE 56  // Bytes of battery-backed NVRAM

// SQW output frequency constants (DS1307 control register values)
#define SQW_OFF      0x00  // SQW pin held low
#define SQW_1HZ      0x10  // 1 Hz square wave
#define SQW_4096HZ   0x11  // 4096 Hz square wave
#define SQW_8192HZ   0x12  // 8192 Hz square wave
#define SQW_32768HZ  0x13  // 32768 Hz square wave

// ---------------------------------------------------------------------------
// DateTime
// ---------------------------------------------------------------------------
struct DateTime {
    uint16_t year;   // Full year, e.g. 2025
    uint8_t  month;  // 1–12
    uint8_t  day;    // 1–31
    uint8_t  hour;   // 0–23 (always 24-hour on this board)
    uint8_t  minute; // 0–59
    uint8_t  second; // 0–59
    uint8_t  dow;    // Day of week: 1 = Monday … 7 = Sunday (ISO 8601)

    DateTime() : year(2000), month(1), day(1), hour(0), minute(0), second(0), dow(1) {}

    DateTime(uint16_t year, uint8_t month, uint8_t day,
             uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0,
             uint8_t dow = 1)
        : year(year), month(month), day(day),
          hour(hour), minute(minute), second(second), dow(dow) {}

    // Convenience: print as "YYYY-MM-DD HH:MM:SS" into a caller-supplied buffer
    // buf must be at least 20 bytes.
    void toString(char* buf) const {
        snprintf(buf, 20, "%04u-%02u-%02u %02u:%02u:%02u",
                 year, month, day, hour, minute, second);
    }

    // Returns true if all fields are within valid ranges.
    bool isValid() const;

    // Returns true if this->year is a leap year.
    bool isLeapYear() const;

    // Compute and store dow from year/month/day (Tomohiko Sakamoto algorithm).
    void computeDow();

    // Convert to seconds since 2000-01-01 00:00:00 (epoch base = Y2K).
    uint32_t toEpoch() const;

    // Construct a DateTime from Y2K-epoch seconds.
    static DateTime fromEpoch(uint32_t epoch);

    // Add (or subtract) seconds in-place.  Adjusts all fields correctly.
    void addSeconds(int32_t s);

    // Return signed seconds between this and an earlier DateTime.
    // Result is positive when this > earlier.
    int32_t secondsSince(const DateTime& earlier) const;

    // Fill h12 with the 12-hour clock value (1–12) and set pm = true for PM.
    void toAMPM(uint8_t& h12, bool& pm) const;

    // Flash-string day name ("Monday" … "Sunday").  dow must be 1–7.
    const __FlashStringHelper* dayName() const;

    // Flash-string month name ("January" … "December").  month must be 1–12.
    const __FlashStringHelper* monthName() const;
};

// ---------------------------------------------------------------------------
// MultiduinoRTC
// ---------------------------------------------------------------------------
class MultiduinoRTCClass {
public:
    // Call once in setup(). Returns false if the DS1307 does not respond.
    bool begin();

    // Returns true if the oscillator is running (CH bit = 0).
    bool isRunning();

    // Start the oscillator if it was stopped (clears the CH bit).
    void start();

    // Stop the oscillator (sets the CH bit). Time registers retain their values.
    void stop();

    // Read the current date/time from the DS1307.
    DateTime now();

    // Write date/time to the DS1307 and start the oscillator.
    void adjust(const DateTime& dt);

    // Configure the SQW/OUT pin.  freq must be one of the SQW_* constants.
    // SQW_OFF drives the pin low; SQW_1HZ/4096HZ/8192HZ/32768HZ output a
    // square wave at the corresponding frequency.
    void setSqwFreq(uint8_t freq);

    // --------------- NVRAM (56 bytes, addresses 0–55) -----------------------

    // Read a single byte from NVRAM. addr must be 0–55.
    uint8_t readNVRAM(uint8_t addr);

    // Write a single byte to NVRAM. addr must be 0–55.
    void writeNVRAM(uint8_t addr, uint8_t value);

    // Read len bytes starting at addr into buf.
    // addr + len must not exceed 56.
    uint8_t readNVRAM(uint8_t addr, uint8_t* buf, uint8_t len);

    // Write len bytes from buf starting at addr.
    // addr + len must not exceed 56.
    void writeNVRAM(uint8_t addr, const uint8_t* buf, uint8_t len);

    // Zero all 56 NVRAM bytes.
    void clearNVRAM();

private:
    static uint8_t bcdToDec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
    static uint8_t decToBcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

    uint8_t readReg(uint8_t reg);
    void    writeReg(uint8_t reg, uint8_t value);
};

extern MultiduinoRTCClass MultiduinoRTC;

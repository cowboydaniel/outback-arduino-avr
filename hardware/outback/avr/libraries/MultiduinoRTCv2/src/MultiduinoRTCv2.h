#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <MultiduinoRTC.h>  // re-uses the DateTime struct

// ---------------------------------------------------------------------------
// MultiduinoRTCv2 — Maxim DS3231 RTC driver for Multiduino v2
//
// The Multiduino v2 uses a DS3231 (temperature-compensated, ±2 ppm) instead
// of the DS1307 fitted on the original Multiduino.  Key differences:
//
//   • Two hardware alarms (Alarm 1: second precision, Alarm 2: minute precision)
//   • INT/SQW output is connected to PE1 (D21) via JP4 (ATmega328PB PCINT25)
//   • 32KHz crystal output is available on PE0 (D20) via JP3
//   • On-chip temperature sensor (0.25 °C resolution)
//   • No general-purpose NVRAM — use MultiduinoFRAM for non-volatile storage
//
// I2C address is the same as the DS1307: 0x68.
//
// JP4 must be closed (A–B) for alarm interrupts to reach the MCU.
// The 3.3V rail (MultiduinoPower) must be enabled before calling begin().
//
// Multiduino v2 only.
// ---------------------------------------------------------------------------

#define DS3231_ADDR   0x68

// DS3231 register map
#define DS3231_REG_SEC       0x00
#define DS3231_REG_MIN       0x01
#define DS3231_REG_HOUR      0x02
#define DS3231_REG_DOW       0x03
#define DS3231_REG_DATE      0x04
#define DS3231_REG_MON       0x05  // bit 7 = century
#define DS3231_REG_YEAR      0x06
#define DS3231_REG_ALM1_SEC  0x07  // Alarm 1 seconds  (bit 7 = A1M1)
#define DS3231_REG_ALM1_MIN  0x08  // Alarm 1 minutes  (bit 7 = A1M2)
#define DS3231_REG_ALM1_HOUR 0x09  // Alarm 1 hours    (bit 7 = A1M3)
#define DS3231_REG_ALM1_DAY  0x0A  // Alarm 1 day/date (bit 7 = A1M4, bit 6 = DY/DT)
#define DS3231_REG_ALM2_MIN  0x0B  // Alarm 2 minutes  (bit 7 = A2M2)
#define DS3231_REG_ALM2_HOUR 0x0C  // Alarm 2 hours    (bit 7 = A2M3)
#define DS3231_REG_ALM2_DAY  0x0D  // Alarm 2 day/date (bit 7 = A2M4, bit 6 = DY/DT)
#define DS3231_REG_CTRL      0x0E
#define DS3231_REG_STATUS    0x0F
#define DS3231_REG_AGING     0x10
#define DS3231_REG_TEMP_MSB  0x11
#define DS3231_REG_TEMP_LSB  0x12

// Control register bits (0x0E)
#define DS3231_CTRL_EOSC    0x80  // enable oscillator (active LOW — 0 = running)
#define DS3231_CTRL_BBSQW   0x40  // battery-backed SQW
#define DS3231_CTRL_CONV    0x20  // convert temperature
#define DS3231_CTRL_RS2     0x10  // rate select bit 2
#define DS3231_CTRL_RS1     0x08  // rate select bit 1
#define DS3231_CTRL_INTCN   0x04  // 1 = INT/alarm output, 0 = SQW output
#define DS3231_CTRL_A2IE    0x02  // alarm 2 interrupt enable
#define DS3231_CTRL_A1IE    0x01  // alarm 1 interrupt enable

// Status register bits (0x0F)
#define DS3231_STAT_OSF     0x80  // oscillator stop flag (sticky, write 0 to clear)
#define DS3231_STAT_EN32KHZ 0x08  // 32KHz output enable
#define DS3231_STAT_BSY     0x04  // busy (temperature conversion in progress)
#define DS3231_STAT_A2F     0x02  // alarm 2 fired flag (write 0 to clear)
#define DS3231_STAT_A1F     0x01  // alarm 1 fired flag (write 0 to clear)

// SQW output rates (RS2:RS1 field in control register)
#define DS3231_SQW_1HZ      0x00
#define DS3231_SQW_1024HZ   0x08
#define DS3231_SQW_4096HZ   0x10
#define DS3231_SQW_8192HZ   0x18

// SQW/INT pin — PE1 / D21 / PCINT25 (via JP4)
#define PIN_SQW   21

// 32KHz output pin — PE0 / D20 (via JP3, hardware oscillator output)
#define PIN_32KHZ 20

// ---------------------------------------------------------------------------
// ATmega328PB PCINT3 — PE0–PE3 = PCINT24–PCINT27, in PCIE3 / PCMSK3
// ---------------------------------------------------------------------------
#ifndef PCMSK3
#define PCMSK3 (*(volatile uint8_t*)0x73)
#endif

// ---------------------------------------------------------------------------
// Alarm mode enumerations
// ---------------------------------------------------------------------------

// Alarm 1 modes (A1M4:A1M1 mask bits, combined with optional DY/DT flag)
enum Alarm1Mode : uint8_t {
    ALARM1_EVERY_SECOND              = 0x0F,  // once per second
    ALARM1_MATCH_SECONDS             = 0x0E,  // when seconds match
    ALARM1_MATCH_MINUTES_SECONDS     = 0x0C,  // when minutes & seconds match
    ALARM1_MATCH_HOURS_MINUTES_SECONDS = 0x08,// when hours, minutes & seconds match
    ALARM1_MATCH_DATE                = 0x00,  // when date, hh, mm, ss match
    ALARM1_MATCH_DAY                 = 0x10,  // when day-of-week, hh, mm, ss match
};

// Alarm 2 modes (A2M4:A2M2 mask bits)
enum Alarm2Mode : uint8_t {
    ALARM2_EVERY_MINUTE              = 0x07,  // once per minute (at 00 seconds)
    ALARM2_MATCH_MINUTES             = 0x06,  // when minutes match
    ALARM2_MATCH_HOURS_MINUTES       = 0x04,  // when hours & minutes match
    ALARM2_MATCH_DATE                = 0x00,  // when date, hh, mm match
    ALARM2_MATCH_DAY                 = 0x08,  // when day-of-week, hh, mm match
};

// ---------------------------------------------------------------------------
// MultiduinoRTCv2Class
// ---------------------------------------------------------------------------
class MultiduinoRTCv2Class {
public:
    // -----------------------------------------------------------------------
    // Initialisation
    // -----------------------------------------------------------------------

    // Call once in setup().  Starts the I2C bus and verifies the DS3231 responds.
    // Clears the oscillator stop flag if set (first power-on after battery install).
    // Returns false if the DS3231 does not respond.
    bool begin();

    // -----------------------------------------------------------------------
    // Time
    // -----------------------------------------------------------------------

    // Read the current date/time.  Re-uses the DateTime struct from MultiduinoRTC.
    DateTime now();

    // Set the date/time.  Clears the oscillator stop flag automatically.
    void adjust(const DateTime& dt);

    // Returns true if the oscillator is running (OSF flag = 0).
    bool isRunning();

    // Clear the oscillator stop flag (set by power loss or first battery install).
    void clearOscillatorStopFlag();

    // -----------------------------------------------------------------------
    // Temperature
    // -----------------------------------------------------------------------

    // Read the on-chip temperature in degrees Celsius (0.25 °C resolution).
    // The DS3231 auto-converts temperature every 64 seconds; call
    // forceConversion() to request an immediate reading.
    float temperature();

    // Trigger an immediate temperature conversion.  Completes in <125 ms.
    // temperature() will block until the conversion is done.
    void forceConversion();

    // -----------------------------------------------------------------------
    // Hardware alarms
    // -----------------------------------------------------------------------

    // Set Alarm 1.  dt supplies the seconds/minutes/hours/day/date fields.
    // mode controls which fields are compared (see Alarm1Mode enum).
    void setAlarm1(const DateTime& dt, Alarm1Mode mode = ALARM1_MATCH_HOURS_MINUTES_SECONDS);

    // Set Alarm 2.  dt supplies the minutes/hours/day/date fields.
    // mode controls which fields are compared (see Alarm2Mode enum).
    void setAlarm2(const DateTime& dt, Alarm2Mode mode = ALARM2_MATCH_HOURS_MINUTES);

    // Returns true if Alarm 1 has fired (A1F flag set).  Does not clear the flag.
    bool alarm1Fired();

    // Returns true if Alarm 2 has fired (A2F flag set).  Does not clear the flag.
    bool alarm2Fired();

    // Clear the Alarm 1 fired flag (must be cleared before the next alarm fires).
    void clearAlarm1();

    // Clear the Alarm 2 fired flag.
    void clearAlarm2();

    // -----------------------------------------------------------------------
    // Alarm interrupts  (PE1 / PCINT25, requires JP4 closed)
    // -----------------------------------------------------------------------

    // Attach an interrupt callback for Alarm 1 and/or Alarm 2.
    // Configures the DS3231 INT/SQW pin as an alarm output and enables the
    // ATmega328PB PCINT3 interrupt on PE1.
    // The callback fires on the falling edge of the INT pin (active LOW).
    // Always clear the alarm flag inside the callback (or in loop) to re-arm.
    void onAlarm1(void (*callback)());
    void onAlarm2(void (*callback)());

    // Detach all alarm interrupts.  INT/SQW pin reverts to SQW output mode.
    void detachAlarmInterrupt();

    // -----------------------------------------------------------------------
    // SQW output  (PE1 via JP4 — mutually exclusive with alarm interrupts)
    // -----------------------------------------------------------------------

    // Configure the INT/SQW pin as a square-wave output.
    // rate: DS3231_SQW_1HZ | DS3231_SQW_1024HZ | DS3231_SQW_4096HZ | DS3231_SQW_8192HZ
    void setSqwFreq(uint8_t rate);

    // -----------------------------------------------------------------------
    // 32KHz output  (PE0 via JP3)
    // -----------------------------------------------------------------------

    // Enable the 32KHz output on PE0 (EN32KHZ bit in status register).
    void enable32KHz();

    // Disable the 32KHz output.
    void disable32KHz();

    // Returns true if the 32KHz output is enabled.
    bool is32KHzEnabled();

    // -----------------------------------------------------------------------
    // Internal — called from PCINT3_vect
    // -----------------------------------------------------------------------
    void _handlePcint3();

private:
    uint8_t  readReg (uint8_t reg);
    void     writeReg(uint8_t reg, uint8_t value);
    void     modReg  (uint8_t reg, uint8_t clrMask, uint8_t setMask);

    static uint8_t bcdToDec(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
    static uint8_t decToBcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }

    void (*_cb1)() = nullptr;
    void (*_cb2)() = nullptr;
};

extern MultiduinoRTCv2Class RTCv2;

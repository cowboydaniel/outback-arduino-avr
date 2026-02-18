#include "MultiduinoRTCv2.h"

// ---------------------------------------------------------------------------
// ISR — PCINT3 (covers PE0–PE3, i.e. PCINT24–PCINT27 on ATmega328PB)
// ---------------------------------------------------------------------------
ISR(PCINT3_vect) {
    RTCv2._handlePcint3();
}

// ---------------------------------------------------------------------------
// Low-level register access
// ---------------------------------------------------------------------------

uint8_t MultiduinoRTCv2Class::readReg(uint8_t reg) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS3231_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void MultiduinoRTCv2Class::writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void MultiduinoRTCv2Class::modReg(uint8_t reg, uint8_t clrMask, uint8_t setMask) {
    uint8_t v = readReg(reg);
    v &= ~clrMask;
    v |=  setMask;
    writeReg(reg, v);
}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

bool MultiduinoRTCv2Class::begin() {
    Wire.begin();

    // Probe DS3231 at 0x68
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(DS3231_REG_SEC);
    if (Wire.endTransmission() != 0) return false;

    // Clear the oscillator stop flag so the first now() is valid.
    if (readReg(DS3231_REG_STATUS) & DS3231_STAT_OSF) {
        clearOscillatorStopFlag();
    }

    // Configure PE1 (SQW/INT) as input with pullup (DS3231 INT is open-drain).
    // Configure PE0 (32KHz) as input (hardware oscillator output).
    pinMode(PIN_SQW, INPUT_PULLUP);
    pinMode(PIN_32KHZ, INPUT);

    return true;
}

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------

DateTime MultiduinoRTCv2Class::now() {
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(DS3231_REG_SEC);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS3231_ADDR, (uint8_t)7);

    DateTime dt;
    if (Wire.available() >= 7) {
        dt.second = bcdToDec(Wire.read() & 0x7F);
        dt.minute = bcdToDec(Wire.read() & 0x7F);
        dt.hour   = bcdToDec(Wire.read() & 0x3F);  // ignore 12/24 bit; always 24h
        dt.dow    = Wire.read() & 0x07;
        dt.day    = bcdToDec(Wire.read() & 0x3F);
        dt.month  = bcdToDec(Wire.read() & 0x1F);  // ignore century bit
        dt.year   = 2000U + bcdToDec(Wire.read());
    }
    return dt;
}

void MultiduinoRTCv2Class::adjust(const DateTime& dt) {
    DateTime tmp = dt;
    tmp.computeDow();  // ensure dow is consistent with date

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(DS3231_REG_SEC);
    Wire.write(decToBcd(tmp.second));
    Wire.write(decToBcd(tmp.minute));
    Wire.write(decToBcd(tmp.hour));   // 24-hour mode (bit 6 = 0)
    Wire.write(tmp.dow);
    Wire.write(decToBcd(tmp.day));
    Wire.write(decToBcd(tmp.month));  // century bit left clear (post-2000)
    Wire.write(decToBcd((uint8_t)(tmp.year - 2000)));
    Wire.endTransmission();

    // Setting the time implicitly restarts the oscillator — clear the stop flag.
    clearOscillatorStopFlag();
}

bool MultiduinoRTCv2Class::isRunning() {
    return !(readReg(DS3231_REG_STATUS) & DS3231_STAT_OSF);
}

void MultiduinoRTCv2Class::clearOscillatorStopFlag() {
    modReg(DS3231_REG_STATUS, DS3231_STAT_OSF, 0x00);
}

// ---------------------------------------------------------------------------
// Temperature
// ---------------------------------------------------------------------------

float MultiduinoRTCv2Class::temperature() {
    // Wait if a forced conversion is in progress
    while (readReg(DS3231_REG_CTRL) & DS3231_CTRL_CONV);

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(DS3231_REG_TEMP_MSB);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS3231_ADDR, (uint8_t)2);

    if (Wire.available() < 2) return -999.0f;

    int8_t  msb = (int8_t)Wire.read();
    uint8_t lsb = Wire.read();

    // Upper 2 bits of LSB = fractional part in 0.25°C steps
    float frac = ((lsb >> 6) & 0x03) * 0.25f;
    return (float)msb + (msb < 0 ? -frac : frac);
}

void MultiduinoRTCv2Class::forceConversion() {
    modReg(DS3231_REG_CTRL, 0x00, DS3231_CTRL_CONV);
    while (readReg(DS3231_REG_CTRL) & DS3231_CTRL_CONV);
}

// ---------------------------------------------------------------------------
// Alarms
// ---------------------------------------------------------------------------

void MultiduinoRTCv2Class::setAlarm1(const DateTime& dt, Alarm1Mode mode) {
    bool matchDay = (mode & 0x10);
    uint8_t m = mode & 0x0F;  // actual A1M4:A1M1 bits

    // A1M1 = bit 7 of alarm seconds register, etc.
    uint8_t a1m1 = (m & 0x01) ? 0x80 : 0x00;
    uint8_t a1m2 = (m & 0x02) ? 0x80 : 0x00;
    uint8_t a1m3 = (m & 0x04) ? 0x80 : 0x00;
    uint8_t a1m4 = (m & 0x08) ? 0x80 : 0x00;

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(DS3231_REG_ALM1_SEC);
    Wire.write(decToBcd(dt.second) | a1m1);
    Wire.write(decToBcd(dt.minute) | a1m2);
    Wire.write(decToBcd(dt.hour)   | a1m3);
    if (matchDay) {
        Wire.write(decToBcd(dt.dow) | a1m4 | 0x40);  // DY/DT=1 → day of week
    } else {
        Wire.write(decToBcd(dt.day) | a1m4);           // DY/DT=0 → date
    }
    Wire.endTransmission();

    clearAlarm1();
}

void MultiduinoRTCv2Class::setAlarm2(const DateTime& dt, Alarm2Mode mode) {
    bool matchDay = (mode & 0x08);
    uint8_t m = mode & 0x07;

    uint8_t a2m2 = (m & 0x01) ? 0x80 : 0x00;
    uint8_t a2m3 = (m & 0x02) ? 0x80 : 0x00;
    uint8_t a2m4 = (m & 0x04) ? 0x80 : 0x00;

    Wire.beginTransmission(DS3231_ADDR);
    Wire.write(DS3231_REG_ALM2_MIN);
    Wire.write(decToBcd(dt.minute) | a2m2);
    Wire.write(decToBcd(dt.hour)   | a2m3);
    if (matchDay) {
        Wire.write(decToBcd(dt.dow) | a2m4 | 0x40);
    } else {
        Wire.write(decToBcd(dt.day) | a2m4);
    }
    Wire.endTransmission();

    clearAlarm2();
}

bool MultiduinoRTCv2Class::alarm1Fired() {
    return (readReg(DS3231_REG_STATUS) & DS3231_STAT_A1F);
}

bool MultiduinoRTCv2Class::alarm2Fired() {
    return (readReg(DS3231_REG_STATUS) & DS3231_STAT_A2F);
}

void MultiduinoRTCv2Class::clearAlarm1() {
    modReg(DS3231_REG_STATUS, DS3231_STAT_A1F, 0x00);
}

void MultiduinoRTCv2Class::clearAlarm2() {
    modReg(DS3231_REG_STATUS, DS3231_STAT_A2F, 0x00);
}

// ---------------------------------------------------------------------------
// Alarm interrupts
// ---------------------------------------------------------------------------

void MultiduinoRTCv2Class::onAlarm1(void (*callback)()) {
    _cb1 = callback;
    // INTCN=1 (alarm mode), A1IE=1
    modReg(DS3231_REG_CTRL, DS3231_CTRL_RS1 | DS3231_CTRL_RS2,
                            DS3231_CTRL_INTCN | DS3231_CTRL_A1IE);
    // Enable PCINT3 on PE1 (PCINT25 = bit 1 of PCMSK3)
    PCMSK3 |= (1 << PCINT25);
    PCICR  |= (1 << PCIE3);
}

void MultiduinoRTCv2Class::onAlarm2(void (*callback)()) {
    _cb2 = callback;
    modReg(DS3231_REG_CTRL, DS3231_CTRL_RS1 | DS3231_CTRL_RS2,
                            DS3231_CTRL_INTCN | DS3231_CTRL_A2IE);
    PCMSK3 |= (1 << PCINT25);
    PCICR  |= (1 << PCIE3);
}

void MultiduinoRTCv2Class::detachAlarmInterrupt() {
    PCMSK3 &= ~(1 << PCINT25);
    if (PCMSK3 == 0) PCICR &= ~(1 << PCIE3);
    _cb1 = nullptr;
    _cb2 = nullptr;
    // Revert to SQW output mode, no alarm interrupts
    modReg(DS3231_REG_CTRL, DS3231_CTRL_INTCN | DS3231_CTRL_A1IE | DS3231_CTRL_A2IE, 0x00);
}

// ---------------------------------------------------------------------------
// SQW output
// ---------------------------------------------------------------------------

void MultiduinoRTCv2Class::setSqwFreq(uint8_t rate) {
    // Clear INTCN (SQW mode) and RS bits, then set new RS
    modReg(DS3231_REG_CTRL,
           DS3231_CTRL_INTCN | DS3231_CTRL_RS2 | DS3231_CTRL_RS1,
           rate & (DS3231_CTRL_RS2 | DS3231_CTRL_RS1));
}

// ---------------------------------------------------------------------------
// 32KHz output
// ---------------------------------------------------------------------------

void MultiduinoRTCv2Class::enable32KHz() {
    modReg(DS3231_REG_STATUS, 0x00, DS3231_STAT_EN32KHZ);
}

void MultiduinoRTCv2Class::disable32KHz() {
    modReg(DS3231_REG_STATUS, DS3231_STAT_EN32KHZ, 0x00);
}

bool MultiduinoRTCv2Class::is32KHzEnabled() {
    return (readReg(DS3231_REG_STATUS) & DS3231_STAT_EN32KHZ);
}

// ---------------------------------------------------------------------------
// PCINT3 ISR dispatch — DS3231 INT is active LOW (open-drain)
// ---------------------------------------------------------------------------

void MultiduinoRTCv2Class::_handlePcint3() {
    // Only react to PE1 (PCINT25) going LOW (alarm fired)
    bool intLow = !(PINE & (1 << PE1));
    if (!intLow) return;

    if (_cb1 && alarm1Fired()) _cb1();
    if (_cb2 && alarm2Fired()) _cb2();
}

// Global instance
MultiduinoRTCv2Class RTCv2;

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
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_SEC);
    return Wire.endTransmission() == 0;
}

bool MultiduinoRTCClass::isRunning() {
    return !(readReg(DS1307_REG_SEC) & 0x80);
}

void MultiduinoRTCClass::start() {
    uint8_t sec = readReg(DS1307_REG_SEC);
    writeReg(DS1307_REG_SEC, sec & 0x7F);
}

void MultiduinoRTCClass::stop() {
    uint8_t sec = readReg(DS1307_REG_SEC);
    writeReg(DS1307_REG_SEC, sec | 0x80);
}

DateTime MultiduinoRTCClass::now() {
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_SEC);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)DS1307_ADDR, (uint8_t)7);

    DateTime dt;
    if (Wire.available() >= 7) {
        dt.second = bcdToDec(Wire.read() & 0x7F);
        dt.minute = bcdToDec(Wire.read());
        dt.hour   = bcdToDec(Wire.read() & 0x3F);
        dt.dow    = Wire.read();
        dt.day    = bcdToDec(Wire.read());
        dt.month  = bcdToDec(Wire.read());
        dt.year   = 2000U + bcdToDec(Wire.read());
    }
    return dt;
}

void MultiduinoRTCClass::adjust(const DateTime& dt) {
    // Always recompute dow from the calendar date so callers that omit it
    // (or pass the default dow=1) still get the correct DS1307 register value.
    DateTime tmp = dt;
    tmp.computeDow();
    Wire.beginTransmission(DS1307_ADDR);
    Wire.write(DS1307_REG_SEC);
    Wire.write(decToBcd(tmp.second) & 0x7F);
    Wire.write(decToBcd(tmp.minute));
    Wire.write(decToBcd(tmp.hour));
    Wire.write(tmp.dow);
    Wire.write(decToBcd(tmp.day));
    Wire.write(decToBcd(tmp.month));
    Wire.write(decToBcd((uint8_t)(tmp.year - 2000)));
    Wire.endTransmission();
}

void MultiduinoRTCClass::setSqwFreq(uint8_t freq) {
    // DS1307 control register bits:
    //   bit 7: OUT  (output level when SQW disabled)
    //   bit 4: SQWE (square-wave enable)
    //   bits 1-0: RS1, RS0 (rate select)
    // SQW_OFF  = 0x00 -> SQWE=0, OUT=0
    // SQW_1HZ  = 0x10 -> SQWE=1, RS=00
    // SQW_4096HZ = 0x11 -> SQWE=1, RS=01
    // SQW_8192HZ = 0x12 -> SQWE=1, RS=10
    // SQW_32768HZ= 0x13 -> SQWE=1, RS=11
    writeReg(DS1307_REG_CTRL, freq);
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

    // AVR Wire buffer is 32 bytes; 1 byte is consumed by the register address,
    // leaving 31 bytes of payload per transaction.
    const uint8_t maxDataPerTx = 31;
    uint8_t offset = 0;

    while (offset < len) {
        uint8_t chunk = len - offset;
        if (chunk > maxDataPerTx) chunk = maxDataPerTx;

        Wire.beginTransmission(DS1307_ADDR);
        Wire.write((uint8_t)(DS1307_REG_NVRAM + addr + offset));
        for (uint8_t i = 0; i < chunk; i++) Wire.write(buf[offset + i]);
        Wire.endTransmission();

        offset += chunk;
    }
}

void MultiduinoRTCClass::clearNVRAM() {
    // AVR Wire buffer is 32 bytes; 1 byte is consumed by the register address,
    // leaving 31 bytes of payload per transaction.
    const uint8_t maxDataPerTx = 31;
    uint8_t offset = 0;

    while (offset < DS1307_NVRAM_SIZE) {
        uint8_t chunk = DS1307_NVRAM_SIZE - offset;
        if (chunk > maxDataPerTx) chunk = maxDataPerTx;

        Wire.beginTransmission(DS1307_ADDR);
        Wire.write((uint8_t)(DS1307_REG_NVRAM + offset));
        for (uint8_t i = 0; i < chunk; i++) Wire.write((uint8_t)0x00);
        Wire.endTransmission();

        offset += chunk;
    }
}

// ---------------------------------------------------------------------------
// DateTime helpers
// ---------------------------------------------------------------------------

// Days in each month (non-leap); index 1-based, index 0 unused.
static const uint8_t _daysInMonth[] PROGMEM = {
    0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

bool DateTime::isLeapYear() const {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

bool DateTime::isValid() const {
    if (month < 1 || month > 12) return false;
    if (day < 1) return false;
    uint8_t dim = pgm_read_byte(&_daysInMonth[month]);
    if (month == 2 && isLeapYear()) dim = 29;
    if (day > dim) return false;
    if (hour > 23 || minute > 59 || second > 59) return false;
    if (dow < 1 || dow > 7) return false;
    return true;
}

void DateTime::computeDow() {
    // Tomohiko Sakamoto algorithm (returns 0=Sun ... 6=Sat), remapped to ISO (1=Mon ... 7=Sun)
    static const uint8_t t[] PROGMEM = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t y = year;
    uint8_t m = month, d = day;
    if (m < 3) y--;
    uint8_t wd = (uint8_t)((y + y/4 - y/100 + y/400 + pgm_read_byte(&t[m-1]) + d) % 7);
    // wd: 0=Sun,1=Mon,...,6=Sat  →  ISO: Mon=1,...,Sun=7
    dow = (wd == 0) ? 7 : wd;
}

uint32_t DateTime::toEpoch() const {
    // Count days from 2000-01-01 to this date, then add time.
    uint32_t days = 0;
    for (uint16_t y = 2000; y < year; y++) {
        bool ly = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
        days += ly ? 366 : 365;
    }
    for (uint8_t m = 1; m < month; m++) {
        uint8_t dim = pgm_read_byte(&_daysInMonth[m]);
        if (m == 2 && isLeapYear()) dim = 29;
        days += dim;
    }
    days += (day - 1);
    return days * 86400UL + (uint32_t)hour * 3600UL + (uint32_t)minute * 60UL + second;
}

DateTime DateTime::fromEpoch(uint32_t epoch) {
    DateTime dt;
    dt.second = epoch % 60; epoch /= 60;
    dt.minute = epoch % 60; epoch /= 60;
    dt.hour   = epoch % 24; epoch /= 24;
    // epoch is now days since 2000-01-01
    uint16_t y = 2000;
    while (true) {
        bool ly = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
        uint16_t diy = ly ? 366 : 365;
        if (epoch < diy) break;
        epoch -= diy; y++;
    }
    dt.year = y;
    bool ly = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
    uint8_t m = 1;
    while (m <= 12) {
        uint8_t dim = pgm_read_byte(&_daysInMonth[m]);
        if (m == 2 && ly) dim = 29;
        if (epoch < dim) break;
        epoch -= dim; m++;
    }
    dt.month = m;
    dt.day   = (uint8_t)(epoch + 1);
    dt.computeDow();
    return dt;
}

void DateTime::addSeconds(int32_t s) {
    int32_t ep = (int32_t)toEpoch() + s;
    if (ep < 0) ep = 0;
    *this = fromEpoch((uint32_t)ep);
}

int32_t DateTime::secondsSince(const DateTime& earlier) const {
    return (int32_t)(toEpoch() - earlier.toEpoch());
}

void DateTime::toAMPM(uint8_t& h12, bool& pm) const {
    pm = (hour >= 12);
    h12 = hour % 12;
    if (h12 == 0) h12 = 12;
}

const __FlashStringHelper* DateTime::dayName() const {
    switch (dow) {
        case 1: return F("Monday");
        case 2: return F("Tuesday");
        case 3: return F("Wednesday");
        case 4: return F("Thursday");
        case 5: return F("Friday");
        case 6: return F("Saturday");
        default: return F("Sunday");
    }
}

const __FlashStringHelper* DateTime::monthName() const {
    switch (month) {
        case  1: return F("January");
        case  2: return F("February");
        case  3: return F("March");
        case  4: return F("April");
        case  5: return F("May");
        case  6: return F("June");
        case  7: return F("July");
        case  8: return F("August");
        case  9: return F("September");
        case 10: return F("October");
        case 11: return F("November");
        default: return F("December");
    }
}

// Global instance
MultiduinoRTCClass MultiduinoRTC;

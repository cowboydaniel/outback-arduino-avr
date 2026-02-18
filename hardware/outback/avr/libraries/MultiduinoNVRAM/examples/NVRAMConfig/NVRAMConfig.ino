/*
  NVRAMConfig
  -----------
  A practical pattern for storing versioned application settings in NVRAM.
  Uses a header block with a magic marker, config version, and CRC-8 to
  safely detect stale or corrupt data across firmware upgrades and power cycles.

  Layout (20 bytes used out of 56):
    Addr 0    : magic marker (0xCF)
    Addr 1    : CRC of magic byte (written by writeMagic)
    Addr 2    : config version number
    Addr 3    : CRC-8 over the Settings struct (bytes 4–19)
    Addr 4–19 : Settings struct (16 bytes)

  Upgrade strategy: if the stored version < SETTINGS_VERSION, default values
  are written (a real application could migrate field by field instead).

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

#define MAGIC_BYTE        0xCF
#define ADDR_MAGIC        0     // 2 bytes (magic + CRC via writeMagic)
#define ADDR_VERSION      2     // 1 byte
#define ADDR_DATA_CRC     3     // 1 byte  – CRC of Settings struct
#define ADDR_DATA         4     // 16 bytes – Settings struct

#define SETTINGS_VERSION  2     // Increment when Settings layout changes

struct Settings {
    uint16_t reportIntervalSec;  // How often to send a report
    uint8_t  txPower;            // Transmit power level 0–10
    uint8_t  retries;            // Max retry count
    float    calibOffset;        // Sensor calibration offset
    uint8_t  flags;              // Bit-field of feature flags
    uint8_t  pad[7];             // Padding to reach 16 bytes
    // Total: 2+1+1+4+1+7 = 16 bytes
};

static const Settings kDefaults = {
    30,     // reportIntervalSec
    7,      // txPower
    3,      // retries
    0.0f,   // calibOffset
    0x05,   // flags: bits 0 and 2 set
    {}      // pad
};

void printSettings(const Settings& s) {
    Serial.print(F("  reportIntervalSec : ")); Serial.println(s.reportIntervalSec);
    Serial.print(F("  txPower           : ")); Serial.println(s.txPower);
    Serial.print(F("  retries           : ")); Serial.println(s.retries);
    Serial.print(F("  calibOffset       : ")); Serial.println(s.calibOffset, 4);
    Serial.print(F("  flags             : 0x"));
    if (s.flags < 0x10) Serial.print('0');
    Serial.println(s.flags, HEX);
}

void writeSettings(const Settings& s) {
    NVRAM.put(ADDR_DATA, s);
    NVRAM.write(ADDR_DATA_CRC, NVRAM.crc8(ADDR_DATA, sizeof(Settings)));
}

bool loadSettings(Settings& s) {
    if (!NVRAM.checkMagic(ADDR_MAGIC, MAGIC_BYTE)) return false;
    uint8_t storedVer = NVRAM.read(ADDR_VERSION);
    if (storedVer != SETTINGS_VERSION) return false;
    uint8_t storedCrc = NVRAM.read(ADDR_DATA_CRC);
    if (storedCrc != NVRAM.crc8(ADDR_DATA, sizeof(Settings))) return false;
    NVRAM.get(ADDR_DATA, s);
    return true;
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM Versioned Config ==="));
    Serial.println();

    Settings s;

    if (loadSettings(s)) {
        Serial.println(F("Loaded settings from NVRAM:"));
        printSettings(s);

        // Example: update one field and save
        s.retries++;
        Serial.println();
        Serial.print(F("Bumping retries to "));
        Serial.println(s.retries);
        writeSettings(s);
    } else {
        Serial.println(F("NVRAM invalid or version mismatch – applying defaults:"));
        s = kDefaults;
        printSettings(s);

        // Write magic, version, and settings
        NVRAM.writeMagic(ADDR_MAGIC, MAGIC_BYTE);
        NVRAM.write(ADDR_VERSION, SETTINGS_VERSION);
        writeSettings(s);
    }

    Serial.println();
    Serial.println(F("Settings saved. Reset to reload."));
}

void loop() {
    // Nothing – all work done in setup()
}

/*
  NVRAMStructStorage
  ------------------
  Demonstrates storing and retrieving an entire C struct in NVRAM using
  NVRAM.put() and NVRAM.get().  Also shows how to use a CRC-8 checksum
  to detect corruption across power cycles.

  The struct (AppConfig, 14 bytes) lives at NVRAM address 0.
  A CRC byte is stored immediately after the struct at addr 14.

  On first boot (or after corruption) defaults are loaded and written.
  On subsequent boots the stored config is loaded and validated.

  NVRAM layout (16 bytes used out of 56):
    Addr 0  – 13  : AppConfig struct  (14 bytes)
    Addr 14        : CRC-8 over bytes 0–13
    Addr 15        : (free)

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

// Application configuration structure (must be a POD type)
struct AppConfig {
    uint16_t sampleIntervalMs;  // ADC sample interval in milliseconds
    uint8_t  ledBrightness;     // PWM duty cycle 0–255
    uint8_t  logLevel;          // 0=off, 1=error, 2=info, 3=debug
    uint32_t runTimeSeconds;    // Cumulative run time accumulator
    uint8_t  wifiChannel;       // 1–13
    char     deviceName[5];     // Short name, 4 chars + null terminator
    // Total: 2 + 1 + 1 + 4 + 1 + 5 = 14 bytes
};

#define ADDR_CONFIG  0
#define ADDR_CRC    14  // sizeof(AppConfig) == 14

// Default configuration applied when NVRAM is blank
static const AppConfig kDefaultConfig = {
    1000,       // sampleIntervalMs
    128,        // ledBrightness
    2,          // logLevel = info
    0,          // runTimeSeconds
    6,          // wifiChannel
    "MDuo"      // deviceName
};

void printConfig(const AppConfig& cfg) {
    Serial.print(F("  sampleIntervalMs : ")); Serial.println(cfg.sampleIntervalMs);
    Serial.print(F("  ledBrightness    : ")); Serial.println(cfg.ledBrightness);
    Serial.print(F("  logLevel         : ")); Serial.println(cfg.logLevel);
    Serial.print(F("  runTimeSeconds   : ")); Serial.println(cfg.runTimeSeconds);
    Serial.print(F("  wifiChannel      : ")); Serial.println(cfg.wifiChannel);
    Serial.print(F("  deviceName       : ")); Serial.println(cfg.deviceName);
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM Struct Storage ==="));
    Serial.println();

    AppConfig cfg;
    uint8_t storedCrc = NVRAM.read(ADDR_CRC);
    uint8_t computedCrc = NVRAM.crc8(ADDR_CONFIG, sizeof(AppConfig));

    if (storedCrc == computedCrc) {
        // Config appears valid – load it
        NVRAM.get(ADDR_CONFIG, cfg);
        Serial.println(F("Loaded config from NVRAM:"));
        printConfig(cfg);

        // Simulate accumulating run time (increment each boot for demo)
        cfg.runTimeSeconds += 60;
        Serial.println();
        Serial.print(F("Incrementing runTimeSeconds to "));
        Serial.println(cfg.runTimeSeconds);
    } else {
        // NVRAM blank or corrupt – use defaults
        Serial.println(F("NVRAM invalid – writing default config:"));
        cfg = kDefaultConfig;
        printConfig(cfg);
    }

    // Write config + updated CRC back to NVRAM
    NVRAM.put(ADDR_CONFIG, cfg);
    NVRAM.write(ADDR_CRC, NVRAM.crc8(ADDR_CONFIG, sizeof(AppConfig)));

    Serial.println();
    Serial.println(F("Config saved. Reset to load it back."));
}

void loop() {
    // Nothing – all work done in setup()
}

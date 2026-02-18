/*
 * EEPROMConfigStore
 *
 * Stores a device configuration struct in EEPROM using EEPROMSlot<T>.
 * Values persist across power cycles.  A magic byte detects uninitialised
 * EEPROM (first boot or after a clear).
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoEEPROM.h>

struct Config {
    uint8_t  magic;          // 0xAB = initialised
    char     deviceName[16]; // null-terminated
    uint16_t sampleInterval; // seconds between readings
    uint8_t  flags;
};

EEPROMSlot<Config> configSlot(0);

void loadDefaults(Config& cfg) {
    cfg.magic          = 0xAB;
    strncpy(cfg.deviceName, "Multiduino-v2", sizeof(cfg.deviceName));
    cfg.sampleInterval = 60;
    cfg.flags          = 0x01;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!ExtEEPROM.begin()) {
        Serial.println(F("EEPROM not found.")); while (1);
    }

    Config cfg = configSlot.load();

    if (cfg.magic != 0xAB) {
        Serial.println(F("First boot — writing defaults."));
        loadDefaults(cfg);
        configSlot.save(cfg);
    } else {
        Serial.println(F("Config loaded from EEPROM."));
    }

    Serial.print(F("Device name:     ")); Serial.println(cfg.deviceName);
    Serial.print(F("Sample interval: ")); Serial.print(cfg.sampleInterval); Serial.println(F(" s"));
    Serial.print(F("Flags:           0x")); Serial.println(cfg.flags, HEX);
}

void loop() {}

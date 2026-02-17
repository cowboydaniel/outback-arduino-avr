/*
  NVRAMDualBank
  =============
  Demonstrates a dual-bank (ping-pong) NVRAM pattern.

  Two equal-sized "banks" hold an application configuration struct.  On each
  save the inactive bank is written first; only if the write verifies
  (via CRC) is the active-bank pointer swapped.  This ensures the last
  good configuration survives a power failure mid-write.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

// ---------- Configuration struct (keep <= 26 bytes so two banks fit in 56) --
struct Config {
    uint16_t bootCount;
    uint8_t  brightness;  // 0–255
    char     name[12];    // null-terminated, max 11 chars
};

static const uint8_t BANK_SIZE   = sizeof(Config) + 1; // +1 for CRC
static const uint8_t BANK_A_ADDR = 0;
static const uint8_t BANK_B_ADDR = BANK_SIZE;
static const uint8_t META_ADDR   = 2 * BANK_SIZE;      // 1-byte active bank index (0=A, 1=B)

static bool loadConfig(Config& cfg) {
    uint8_t active = NVRAM.read(META_ADDR);
    if (active > 1) return false;  // uninitialised
    uint8_t base = (active == 0) ? BANK_A_ADDR : BANK_B_ADDR;
    uint8_t storedCrc = NVRAM.read(base + sizeof(Config));
    uint8_t calcCrc   = NVRAM.crc8(base, sizeof(Config));
    if (storedCrc != calcCrc) return false;
    NVRAM.get(base, cfg);
    return true;
}

static void saveConfig(const Config& cfg) {
    uint8_t active  = NVRAM.read(META_ADDR);
    if (active > 1) active = 0;
    uint8_t next    = active ^ 1;
    uint8_t base    = (next == 0) ? BANK_A_ADDR : BANK_B_ADDR;

    NVRAM.put(base, cfg);
    uint8_t crc = NVRAM.crc8(base, sizeof(Config));
    NVRAM.write(base + sizeof(Config), crc);

    // Verify before promoting
    uint8_t stored = NVRAM.read(base + sizeof(Config));
    if (stored == crc) {
        NVRAM.write(META_ADDR, next);
        Serial.print(F("Saved to bank ")); Serial.println(next == 0 ? 'A' : 'B');
    } else {
        Serial.println(F("Write-verify failed – old bank retained"));
    }
}

static void printConfig(const Config& cfg) {
    Serial.print(F("  bootCount : ")); Serial.println(cfg.bootCount);
    Serial.print(F("  brightness: ")); Serial.println(cfg.brightness);
    Serial.print(F("  name      : ")); Serial.println(cfg.name);
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }

    Serial.println(F("=== NVRAMDualBank ===\n"));

    Config cfg;
    if (!loadConfig(cfg)) {
        Serial.println(F("No valid config – using defaults"));
        cfg.bootCount  = 0;
        cfg.brightness = 128;
        strncpy(cfg.name, "Multiduino", sizeof(cfg.name) - 1);
        cfg.name[sizeof(cfg.name) - 1] = '\0';
    }

    cfg.bootCount++;
    Serial.print(F("Boot #")); Serial.println(cfg.bootCount);
    printConfig(cfg);
    saveConfig(cfg);

    // Modify and save again to show bank alternation
    cfg.brightness = (cfg.brightness < 200) ? cfg.brightness + 10 : 10;
    Serial.println(F("\nUpdating brightness:"));
    printConfig(cfg);
    saveConfig(cfg);

    Serial.println(F("\nFinal NVRAM state:"));
    NVRAM.dump(0, META_ADDR + 1, Serial);
}

void loop() {}

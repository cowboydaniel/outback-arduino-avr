/*
  SDNVRAMSyncConfig
  =================
  Cross-library example: MultiduinoNVRAM + MultiduinoSD

  Maintains a configuration struct in two places:
    - NVRAM (fast, instant, 56-byte battery-backed SRAM) for runtime access.
    - SD card file "CONFIG.TXT" as a human-readable backup/restore source.

  On boot:
    1. If NVRAM layout tag is valid, load config from NVRAM.
    2. Otherwise try to load from SD "CONFIG.TXT" and save to NVRAM.
    3. If neither exists, use defaults and write both.

  Hardware: Multiduino with DS1307 RTC + SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>
#include <MultiduinoSD.h>

#define LAYOUT_ADDR     0   // 2 bytes: version + CRC
#define CONFIG_ADDR     2   // config struct starts here
#define LAYOUT_VER   0x02   // bump when struct layout changes
#define CONFIG_FILE  "CONFIG.TXT"

struct AppConfig {
    uint16_t baudRate;        // Serial baud rate (stored / 100 to fit in 2 bytes)
    uint8_t  ledBrightness;   // 0–255
    uint8_t  samplePeriodSec; // data-sampling interval
    char     deviceName[12];  // null-terminated, max 11 chars
};

static void configToText(const AppConfig& c, char* buf, uint8_t len) {
    snprintf(buf, len,
        "baud=%u\nbright=%u\nperiod=%u\nname=%s\n",
        (unsigned)(c.baudRate * 100),
        c.ledBrightness,
        c.samplePeriodSec,
        c.deviceName);
}

static void printConfig(const AppConfig& c) {
    Serial.print(F("  baud      : ")); Serial.println(c.baudRate * 100);
    Serial.print(F("  brightness: ")); Serial.println(c.ledBrightness);
    Serial.print(F("  period    : ")); Serial.println(c.samplePeriodSec);
    Serial.print(F("  name      : ")); Serial.println(c.deviceName);
}

static bool loadFromNVRAM(AppConfig& c) {
    if (!NVRAM.checkLayout(LAYOUT_ADDR, LAYOUT_VER)) return false;
    NVRAM.get(CONFIG_ADDR, c);
    return true;
}

static void saveToNVRAM(const AppConfig& c) {
    NVRAM.writeLayout(LAYOUT_ADDR, LAYOUT_VER);
    NVRAM.put(CONFIG_ADDR, c);
}

static bool loadFromSD(AppConfig& c) {
    if (!MultiduinoSD.exists(CONFIG_FILE)) return false;
    SDFile f = MultiduinoSD.open(CONFIG_FILE, FILE_READ);
    if (!f) return false;

    // Simple key=value parser
    char line[32];
    c = AppConfig{};  // zero-init
    while (f.available()) {
        f.readLine(line, sizeof(line));
        if (strncmp(line, "baud=", 5) == 0)   c.baudRate        = (uint16_t)(atol(line + 5) / 100);
        if (strncmp(line, "bright=", 7) == 0)  c.ledBrightness   = (uint8_t)atoi(line + 7);
        if (strncmp(line, "period=", 7) == 0)  c.samplePeriodSec = (uint8_t)atoi(line + 7);
        if (strncmp(line, "name=", 5) == 0) {
            strncpy(c.deviceName, line + 5, sizeof(c.deviceName) - 1);
            c.deviceName[sizeof(c.deviceName) - 1] = '\0';
        }
    }
    f.close();
    return c.baudRate > 0;
}

static void saveToSD(const AppConfig& c) {
    if (MultiduinoSD.exists(CONFIG_FILE)) MultiduinoSD.remove(CONFIG_FILE);
    SDFile f = MultiduinoSD.open(CONFIG_FILE,
                   SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
    if (!f) { Serial.println(F("SD write failed")); return; }
    char buf[80]; configToText(c, buf, sizeof(buf));
    f.print(buf);
    f.close();
    Serial.println(F("Config saved to SD."));
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("RTC not found!")); while (true) {}
    }
    bool sdOk = MultiduinoSD.begin();

    AppConfig cfg;
    bool loaded = false;

    Serial.println(F("=== SDNVRAMSyncConfig ===\n"));

    if (loadFromNVRAM(cfg)) {
        Serial.println(F("Loaded from NVRAM."));
        loaded = true;
    } else if (sdOk && loadFromSD(cfg)) {
        Serial.println(F("Loaded from SD, saving to NVRAM."));
        saveToNVRAM(cfg);
        loaded = true;
    }

    if (!loaded) {
        Serial.println(F("Using defaults."));
        cfg.baudRate        = 1152;   // 115200 / 100
        cfg.ledBrightness   = 128;
        cfg.samplePeriodSec = 10;
        strncpy(cfg.deviceName, "Multiduino", sizeof(cfg.deviceName) - 1);
        cfg.deviceName[sizeof(cfg.deviceName) - 1] = '\0';
        saveToNVRAM(cfg);
        if (sdOk) saveToSD(cfg);
    }

    Serial.println(F("Active configuration:"));
    printConfig(cfg);

    // Modify and re-sync to show update flow
    cfg.samplePeriodSec++;
    Serial.println(F("\nIncremented samplePeriod, re-syncing:"));
    saveToNVRAM(cfg);
    if (sdOk) saveToSD(cfg);
    printConfig(cfg);
}

void loop() {}

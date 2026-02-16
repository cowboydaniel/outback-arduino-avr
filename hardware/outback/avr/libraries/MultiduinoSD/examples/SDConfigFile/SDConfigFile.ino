/*
  SDConfigFile
  ------------
  Demonstrates a simple key=value configuration file on the SD card.

  On first boot the file CONFIG.TXT is created with default settings.
  On subsequent boots the file is parsed and the values loaded.
  The sketch also shows how to update a single setting and rewrite the file.

  CONFIG.TXT format (one key=value per line, # comments ignored):
    # Multiduino configuration
    sampleRate=10
    txPower=7
    deviceName=MDuo01
    logEnabled=1

  The parser handles:
    - Lines starting with '#' or whitespace are skipped.
    - Key and value are separated by '='.
    - Values are read as C strings (max KEY_MAX / VAL_MAX chars).
    - Unknown keys are ignored.

  Hardware: Multiduino (ATmega328P), SD card on SPI (CS = D10).
*/

#include <MultiduinoSD.h>

#define CONFIG_FILE  "CONFIG.TXT"
#define KEY_MAX      16
#define VAL_MAX      24
#define LINE_MAX     (KEY_MAX + 1 + VAL_MAX + 2)  // key=value\r\n

// ---- Configuration struct -----------------------------------------------
struct Config {
    uint16_t sampleRate;   // samples per minute
    uint8_t  txPower;      // 0–10
    char     deviceName[12];
    bool     logEnabled;
};

static const Config kDefaults = { 10, 7, "MDuo01", true };

// ---- INI parser helpers --------------------------------------------------

// Read one text line from SDFile f into buf (maxLen includes null).
// Returns number of chars read (0 = EOF).
static uint8_t readLine(SDFile& f, char* buf, uint8_t maxLen) {
    uint8_t i = 0;
    while (i < maxLen - 1 && f.position() < f.size()) {
        int c = f.read();
        if (c < 0) break;
        if (c == '\r') continue;  // Skip CR in CRLF
        if (c == '\n') break;
        buf[i++] = (char)c;
    }
    buf[i] = '\0';
    return i;
}

// Split "key=value" into key (out_k) and value (out_v) buffers.
// Returns false if '=' not found or either buffer would overflow.
static bool splitKV(const char* line, char* out_k, char* out_v) {
    const char* eq = strchr(line, '=');
    if (!eq) return false;
    uint8_t klen = (uint8_t)(eq - line);
    if (klen == 0 || klen >= KEY_MAX) return false;
    uint8_t vlen = strlen(eq + 1);
    if (vlen >= VAL_MAX) return false;
    memcpy(out_k, line, klen); out_k[klen] = '\0';
    strcpy(out_v, eq + 1);
    return true;
}

// ---- Load config from SD -------------------------------------------------
static bool loadConfig(Config& cfg) {
    SDFile f = MultiduinoSD.open(CONFIG_FILE, FILE_READ);
    if (!f) return false;

    cfg = kDefaults;  // Populate defaults first
    char line[LINE_MAX], key[KEY_MAX], val[VAL_MAX];

    while (f.position() < f.size()) {
        if (!readLine(f, line, LINE_MAX)) break;
        if (line[0] == '#' || line[0] == '\0') continue;  // Comment / blank
        if (!splitKV(line, key, val)) continue;

        if      (strcmp(key, "sampleRate")  == 0) cfg.sampleRate  = (uint16_t)atoi(val);
        else if (strcmp(key, "txPower")     == 0) cfg.txPower     = (uint8_t)atoi(val);
        else if (strcmp(key, "deviceName")  == 0) { strncpy(cfg.deviceName, val, 11); cfg.deviceName[11] = '\0'; }
        else if (strcmp(key, "logEnabled")  == 0) cfg.logEnabled  = (atoi(val) != 0);
    }
    f.close();
    return true;
}

// ---- Write config to SD --------------------------------------------------
static bool writeConfig(const Config& cfg) {
    SDFile f = MultiduinoSD.open(CONFIG_FILE, SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
    if (!f) return false;
    f.println("# Multiduino configuration");
    f.print("sampleRate=");  f.println((const char*)String(cfg.sampleRate).c_str());
    f.print("txPower=");     f.println((const char*)String(cfg.txPower).c_str());
    f.print("deviceName=");  f.println(cfg.deviceName);
    f.print("logEnabled=");  f.println(cfg.logEnabled ? "1" : "0");
    f.flush(); f.close();
    return true;
}

static void printConfig(const Config& cfg) {
    Serial.print(F("  sampleRate  : ")); Serial.println(cfg.sampleRate);
    Serial.print(F("  txPower     : ")); Serial.println(cfg.txPower);
    Serial.print(F("  deviceName  : ")); Serial.println(cfg.deviceName);
    Serial.print(F("  logEnabled  : ")); Serial.println(cfg.logEnabled ? F("yes") : F("no"));
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println(F("=== SD Config File ==="));
    Serial.println();

    if (!MultiduinoSD.begin()) {
        Serial.println(F("ERROR: SD init failed."));
        while (true) {}
    }

    Config cfg;

    if (!MultiduinoSD.exists(CONFIG_FILE)) {
        Serial.println(F("CONFIG.TXT not found – creating with defaults:"));
        cfg = kDefaults;
        printConfig(cfg);
        if (writeConfig(cfg)) {
            Serial.println(F("Written."));
        } else {
            Serial.println(F("ERROR writing config."));
        }
    } else {
        Serial.println(F("Loading CONFIG.TXT:"));
        if (loadConfig(cfg)) {
            printConfig(cfg);
        } else {
            Serial.println(F("ERROR reading config – using defaults."));
            cfg = kDefaults;
        }

        // Demonstrate updating a setting and saving
        cfg.sampleRate += 5;
        Serial.println();
        Serial.print(F("Incrementing sampleRate to ")); Serial.println(cfg.sampleRate);
        writeConfig(cfg);
        Serial.println(F("Config updated."));
    }

    Serial.println();
    Serial.println(F("Reset to reload. Delete CONFIG.TXT to recreate defaults."));
}

void loop() {
    // Nothing – all work done in setup()
}

/*
  NVRAMStringStorage
  ------------------
  Shows how to store and retrieve null-terminated C strings in NVRAM using
  NVRAM.putString() and NVRAM.getString().

  Three independent strings are stored at different addresses and read back.
  Because NVRAM is battery-backed, the strings persist across reboots.

  NVRAM address map (48 bytes used out of 56):
    Addr  0 – 15  : device name (16 bytes, including null)
    Addr 16 – 31  : firmware version string (16 bytes)
    Addr 32 – 47  : last error message (16 bytes)

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

#define ADDR_DEVICE_NAME    0
#define ADDR_FW_VERSION    16
#define ADDR_LAST_ERROR    32
#define STRING_FIELD_LEN   16

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM String Storage ==="));
    Serial.println();

    // ------------------------------------------------------------------
    // Write three strings to NVRAM
    // ------------------------------------------------------------------
    const char* deviceName  = "Multiduino-01";
    const char* fwVersion   = "v1.0.0-release";
    const char* lastError   = "E03:SD_INIT_FAIL";

    Serial.println(F("Writing strings to NVRAM..."));
    NVRAM.putString(ADDR_DEVICE_NAME, deviceName);
    NVRAM.putString(ADDR_FW_VERSION,  fwVersion);
    NVRAM.putString(ADDR_LAST_ERROR,  lastError);

    // ------------------------------------------------------------------
    // Read strings back
    // ------------------------------------------------------------------
    char buf[STRING_FIELD_LEN];

    Serial.println(F("Reading back:"));

    NVRAM.getString(ADDR_DEVICE_NAME, buf, STRING_FIELD_LEN);
    Serial.print(F("  Device name    : ")); Serial.println(buf);

    NVRAM.getString(ADDR_FW_VERSION, buf, STRING_FIELD_LEN);
    Serial.print(F("  Firmware ver   : ")); Serial.println(buf);

    NVRAM.getString(ADDR_LAST_ERROR, buf, STRING_FIELD_LEN);
    Serial.print(F("  Last error     : ")); Serial.println(buf);

    Serial.println();

    // ------------------------------------------------------------------
    // Overwrite one string to show update behaviour
    // ------------------------------------------------------------------
    Serial.println(F("Clearing last error..."));
    NVRAM.putString(ADDR_LAST_ERROR, "OK");

    NVRAM.getString(ADDR_LAST_ERROR, buf, STRING_FIELD_LEN);
    Serial.print(F("  Last error now : ")); Serial.println(buf);

    Serial.println();
    Serial.println(F("Reset the board – the strings are retained in NVRAM."));
}

void loop() {
    // Nothing – all work done in setup()
}

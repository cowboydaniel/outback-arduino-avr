/*
  NVRAMStringStorage
  ------------------
  Shows how to store and retrieve null-terminated C strings in NVRAM using
  NVRAM.putString() and NVRAM.getString().

  On the first boot the sketch writes three default strings and marks NVRAM
  as initialised with a magic byte.  On every subsequent boot it reads the
  stored strings back without overwriting them, so you can actually observe
  that the values (including any mid-run update to "last error") survive a
  reset.

  NVRAM address map (49 bytes used out of 56):
    Addr  0 – 15  : device name (16 bytes, including null)
    Addr 16 – 31  : firmware version string (16 bytes)
    Addr 32 – 47  : last error message (16 bytes)
    Addr 48       : magic byte – 0xA5 means NVRAM has been initialised

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

#define ADDR_DEVICE_NAME    0
#define ADDR_FW_VERSION    16
#define ADDR_LAST_ERROR    32
#define ADDR_MAGIC         48
#define STRING_FIELD_LEN   16
#define MAGIC_VALUE        0xA5

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
    // First-boot detection via magic byte
    // ------------------------------------------------------------------
    if (NVRAM.read(ADDR_MAGIC) != MAGIC_VALUE) {
        // NVRAM has never been initialised – write defaults now.
        Serial.println(F("First boot: writing default strings to NVRAM..."));

        NVRAM.putString(ADDR_DEVICE_NAME, "Multiduino-01");
        NVRAM.putString(ADDR_FW_VERSION,  "v1.0.0-release");
        NVRAM.putString(ADDR_LAST_ERROR,  "E03:SD_INIT_FAIL");
        NVRAM.write(ADDR_MAGIC, MAGIC_VALUE);

        Serial.println(F("Done."));
        Serial.println();
    } else {
        Serial.println(F("NVRAM already initialised – reading stored values."));
        Serial.println();
    }

    // ------------------------------------------------------------------
    // Read strings back (works identically on first and subsequent boots)
    // ------------------------------------------------------------------
    char buf[STRING_FIELD_LEN];

    Serial.println(F("Stored strings:"));

    NVRAM.getString(ADDR_DEVICE_NAME, buf, STRING_FIELD_LEN);
    Serial.print(F("  Device name    : ")); Serial.println(buf);

    NVRAM.getString(ADDR_FW_VERSION, buf, STRING_FIELD_LEN);
    Serial.print(F("  Firmware ver   : ")); Serial.println(buf);

    NVRAM.getString(ADDR_LAST_ERROR, buf, STRING_FIELD_LEN);
    Serial.print(F("  Last error     : ")); Serial.println(buf);

    Serial.println();

    // ------------------------------------------------------------------
    // Update one field to show that mid-run changes persist across resets
    // ------------------------------------------------------------------
    Serial.println(F("Clearing last error (writing \"OK\")..."));
    NVRAM.putString(ADDR_LAST_ERROR, "OK");

    NVRAM.getString(ADDR_LAST_ERROR, buf, STRING_FIELD_LEN);
    Serial.print(F("  Last error now : ")); Serial.println(buf);

    Serial.println();
    Serial.println(F("Reset the board – on the next boot 'Last error' will"));
    Serial.println(F("show \"OK\", proving the update survived the reset."));
}

void loop() {
    // Nothing – all work done in setup()
}

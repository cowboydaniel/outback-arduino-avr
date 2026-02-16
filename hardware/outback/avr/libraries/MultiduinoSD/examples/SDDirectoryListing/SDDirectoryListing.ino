/*
  SDDirectoryListing
  ------------------
  Lists the complete directory tree of the SD card to Serial, showing
  file sizes alongside names.  Then iterates the root directory manually
  using openNextFile() to demonstrate the SDFile iteration API.

  Two listing methods are shown:
    1. MultiduinoSD.ls() – recursive, formatted, uses depth-indent.
    2. Manual iteration with dir.openNextFile() – gives full programmatic
       access to each SDFile (name, size, isDirectory).

  Hardware: Multiduino (ATmega328P), SD card on SPI (CS = D10).
*/

#include <MultiduinoSD.h>

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println(F("=== SD Directory Listing ==="));
    Serial.println();

    if (!MultiduinoSD.begin()) {
        Serial.println(F("ERROR: SD init failed."));
        while (true) {}
    }
    MultiduinoSD.printCardInfo(Serial);
    Serial.println();

    // ------------------------------------------------------------------
    // 1. Full recursive tree via ls()
    // ------------------------------------------------------------------
    Serial.println(F("--- Full directory tree ---"));
    MultiduinoSD.ls("/", Serial);
    Serial.println();

    // ------------------------------------------------------------------
    // 2. Manual iteration of root directory
    // ------------------------------------------------------------------
    Serial.println(F("--- Root directory (manual iteration) ---"));
    SDFile root = MultiduinoSD.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println(F("Cannot open root directory."));
    } else {
        uint16_t fileCount = 0;
        uint16_t dirCount  = 0;
        uint32_t totalSize = 0;

        while (true) {
            SDFile entry = root.openNextFile();
            if (!entry) break;

            if (entry.isDirectory()) {
                Serial.print(F("[DIR]  "));
                Serial.println(entry.name());
                dirCount++;
            } else {
                uint32_t sz = entry.size();
                Serial.print(F("[FILE] "));
                Serial.print(entry.name());
                Serial.print(F("  "));
                Serial.print(sz);
                Serial.println(F(" B"));
                totalSize += sz;
                fileCount++;
            }
            entry.close();
        }
        root.close();

        Serial.println();
        Serial.print(F("Files: ")); Serial.println(fileCount);
        Serial.print(F("Dirs : ")); Serial.println(dirCount);
        Serial.print(F("Total: ")); Serial.print(totalSize); Serial.println(F(" B"));
    }

    Serial.println();
    Serial.println(F("Done."));
}

void loop() {
    // Nothing – all work done in setup()
}

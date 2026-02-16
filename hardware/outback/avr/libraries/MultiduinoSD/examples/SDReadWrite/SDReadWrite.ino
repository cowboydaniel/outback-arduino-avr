/*
  SDReadWrite
  -----------
  Basic SD card read/write example using the custom MultiduinoSD library.

  Demonstrates:
    - Initialising the SD card with MultiduinoSD.begin()
    - Printing card type, FAT type, and capacity
    - Writing text to a file (append mode creates if missing)
    - Reading the file back and printing to Serial
    - Listing the root directory

  "LOG.TXT" is created on first run and appended on each subsequent reset.
  Open Serial Monitor at 115200 baud.

  Hardware: Multiduino (ATmega328P), SD card on SPI (CS = D10).
  Library : MultiduinoSD v2 – custom FAT16/FAT32, no Arduino SD dependency.
*/

#include <MultiduinoSD.h>

static const char FILENAME[] = "LOG.TXT";

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println(F("=== SD Read/Write ==="));
    Serial.println();

    // ---- Initialise SD ---------------------------------------------------
    Serial.print(F("Initialising SD card... "));
    if (!MultiduinoSD.begin()) {
        Serial.println(F("FAILED."));
        Serial.println(F("  Check card is inserted and formatted FAT16/FAT32."));
        while (true) {}
    }
    Serial.println(F("OK"));
    MultiduinoSD.printCardInfo(Serial);
    Serial.println();

    // ---- Append a line ---------------------------------------------------
    SDFile f = MultiduinoSD.open(FILENAME, FILE_WRITE);
    if (f) {
        f.println("Hello from Multiduino!");
        f.flush();
        f.close();
        Serial.print(F("Appended a line to ")); Serial.println(FILENAME);
    } else {
        Serial.print(F("ERROR: could not open ")); Serial.println(FILENAME);
    }

    // ---- Read the whole file back ----------------------------------------
    Serial.println();
    Serial.println(F("--- File contents ---"));
    f = MultiduinoSD.open(FILENAME, FILE_READ);
    if (f) {
        while (f.position() < f.size()) {
            int c = f.read();
            if (c < 0) break;
            Serial.write((char)c);
        }
        f.close();
    } else {
        Serial.println(F("ERROR: could not open file for reading."));
    }
    Serial.println(F("--- End of file ---"));
    Serial.println();

    // ---- Directory listing -----------------------------------------------
    Serial.println(F("Root directory:"));
    MultiduinoSD.ls("/", Serial);
    Serial.println();

    Serial.println(F("Done."));
}

void loop() {
    // Nothing – all work done in setup()
}

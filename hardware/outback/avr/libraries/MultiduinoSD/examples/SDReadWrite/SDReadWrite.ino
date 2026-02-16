/*
 * SDReadWrite — Multiduino example
 *
 * Creates a file on the SD card, appends a line to it on every boot,
 * then reads back the entire file and prints it to Serial.
 *
 * Hardware: Multiduino (micro SD on SPI, CS = D10)
 * Library : MultiduinoSD (depends on Arduino SD library)
 */

#include <MultiduinoSD.h>

static const char FILENAME[] = "log.txt";

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println(F("Initialising SD card..."));
    if (!MultiduinoSD.begin()) {
        Serial.println(F("ERROR: SD card not found or init failed."));
        Serial.println(F("  • Check card is seated correctly."));
        Serial.println(F("  • Check card is formatted FAT16 or FAT32."));
        while (true);
    }
    Serial.println(F("SD ready.\n"));
    MultiduinoSD.printCardInfo(Serial);
    Serial.println();

    // --- Append a line ---
    File f = MultiduinoSD.open(FILENAME, FILE_WRITE);
    if (f) {
        f.println(F("Hello from Multiduino!"));
        f.close();
        Serial.print(F("Appended a line to "));
        Serial.println(FILENAME);
    } else {
        Serial.print(F("ERROR: could not open "));
        Serial.println(FILENAME);
    }

    // --- Read the whole file back ---
    Serial.println(F("\n--- File contents ---"));
    f = MultiduinoSD.open(FILENAME, FILE_READ);
    if (f) {
        while (f.available()) {
            Serial.write(f.read());
        }
        f.close();
    } else {
        Serial.println(F("ERROR: could not open file for reading."));
    }
    Serial.println(F("--- End of file ---\n"));

    // --- Directory listing ---
    Serial.println(F("Root directory:"));
    MultiduinoSD.ls("/", Serial);
}

void loop() {
    // Nothing.
}

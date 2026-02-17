/*
  SDMultiFile
  ===========
  Opens SD_MAX_OPEN_FILES (4) files simultaneously and writes to each one,
  demonstrating that all open-file slots work independently.

  Hardware: Multiduino with SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoSD.h>

#define NUM_FILES  SD_MAX_OPEN_FILES   // 4

static const char* NAMES[NUM_FILES] = {
    "FILE0.TXT", "FILE1.TXT", "FILE2.TXT", "FILE3.TXT"
};

void setup() {
    Serial.begin(115200);
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!")); while (true) {}
    }
    Serial.println(F("=== SDMultiFile ===\n"));

    // Remove old files
    for (uint8_t i = 0; i < NUM_FILES; i++) {
        if (MultiduinoSD.exists(NAMES[i])) MultiduinoSD.remove(NAMES[i]);
    }

    // Open all slots simultaneously
    SDFile files[NUM_FILES];
    for (uint8_t i = 0; i < NUM_FILES; i++) {
        files[i] = MultiduinoSD.open(NAMES[i],
                       SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
        if (!files[i]) {
            Serial.print(F("Failed to open ")); Serial.println(NAMES[i]);
        } else {
            Serial.print(F("Opened ")); Serial.println(NAMES[i]);
        }
    }

    // Write distinct content to each
    for (uint8_t i = 0; i < NUM_FILES; i++) {
        if (!files[i]) continue;
        for (uint8_t line = 0; line < 5; line++) {
            files[i].print(NAMES[i]);
            files[i].print(F(" line "));
            char num[4]; itoa(line, num, 10);
            files[i].println(num);
        }
        files[i].close();
        Serial.print(F("Closed ")); Serial.println(NAMES[i]);
    }

    // Read back and verify
    Serial.println();
    for (uint8_t i = 0; i < NUM_FILES; i++) {
        SDFile f = MultiduinoSD.open(NAMES[i], FILE_READ);
        if (!f) { Serial.print(F("read-open failed: ")); Serial.println(NAMES[i]); continue; }
        Serial.print(F("--- ")); Serial.print(NAMES[i]);
        Serial.print(F(" (")); Serial.print(f.size()); Serial.println(F(" bytes) ---"));
        char line[48];
        while (f.available()) {
            f.readLine(line, sizeof(line));
            Serial.println(line);
        }
        f.close();
    }
    Serial.println(F("Done."));
}

void loop() {}

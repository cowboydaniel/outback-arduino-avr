/*
  SDNestedDirectories
  ===================
  Creates a tree of nested directories and files, then lists the full tree
  using ls().  Exercises mkdir() at multiple depth levels and confirms that
  files placed inside nested directories are accessible.

  Tree created:
    /TREE/
    /TREE/A/
    /TREE/A/FILE_A.TXT
    /TREE/B/
    /TREE/B/FILE_B.TXT
    /TREE/B/C/
    /TREE/B/C/FILE_C.TXT

  Hardware: Multiduino with SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoSD.h>

static void writeTextFile(const char* path, const char* content) {
    SDFile f = MultiduinoSD.open(path, SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
    if (f) { f.print(content); f.close(); }
    else { Serial.print(F("create failed: ")); Serial.println(path); }
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!")); while (true) {}
    }
    Serial.println(F("=== SDNestedDirectories ===\n"));

    // Clean up from previous run (deepest first)
    const char* cleanup[] = {
        "TREE/B/C/FILE_C.TXT", "TREE/B/C",
        "TREE/B/FILE_B.TXT",   "TREE/B",
        "TREE/A/FILE_A.TXT",   "TREE/A",
        "TREE"
    };
    for (uint8_t i = 0; i < 7; i++) {
        if (!MultiduinoSD.exists(cleanup[i])) continue;
        // Try remove (file) then rmdir (dir)
        MultiduinoSD.remove(cleanup[i]);
        MultiduinoSD.rmdir(cleanup[i]);
    }

    // Build tree
    MultiduinoSD.mkdir("TREE");
    MultiduinoSD.mkdir("TREE/A");
    MultiduinoSD.mkdir("TREE/B");
    MultiduinoSD.mkdir("TREE/B/C");

    writeTextFile("TREE/A/FILE_A.TXT",   "Content of file A\n");
    writeTextFile("TREE/B/FILE_B.TXT",   "Content of file B\n");
    writeTextFile("TREE/B/C/FILE_C.TXT", "Content of file C\n");

    // Verify all exist
    const char* checks[] = {
        "TREE", "TREE/A", "TREE/B", "TREE/B/C",
        "TREE/A/FILE_A.TXT", "TREE/B/FILE_B.TXT", "TREE/B/C/FILE_C.TXT"
    };
    Serial.println(F("Existence checks:"));
    for (uint8_t i = 0; i < 7; i++) {
        Serial.print(F("  ")); Serial.print(checks[i]);
        Serial.println(MultiduinoSD.exists(checks[i]) ? F(": exists") : F(": MISSING"));
    }

    // Read back deepest file
    Serial.println(F("\nReading TREE/B/C/FILE_C.TXT:"));
    SDFile f = MultiduinoSD.open("TREE/B/C/FILE_C.TXT", FILE_READ);
    if (f) {
        char line[48];
        while (f.available()) { f.readLine(line, sizeof(line)); Serial.println(line); }
        f.close();
    }

    // Full tree listing
    Serial.println(F("\nFull tree:"));
    MultiduinoSD.ls("TREE", Serial, 0);
    Serial.println(F("\nDone."));
}

void loop() {}

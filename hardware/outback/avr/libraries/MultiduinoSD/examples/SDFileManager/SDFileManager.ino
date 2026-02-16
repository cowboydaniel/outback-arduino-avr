/*
  SDFileManager
  -------------
  Demonstrates file and directory management operations using the custom
  MultiduinoSD library:

    - Create directories (nested)
    - Create files inside those directories
    - Write data to files
    - Read data back
    - Check file existence
    - Delete files
    - Remove empty directories
    - List the resulting directory tree

  All output goes to Serial at 115200 baud.

  Hardware: Multiduino (ATmega328P), SD card on SPI (CS = D10).
*/

#include <MultiduinoSD.h>

// Helper: write a line to a file at path, then close it.
static bool writeFile(const char* path, const char* content) {
    SDFile f = MultiduinoSD.open(path, FILE_WRITE | SD_O_TRUNC);
    if (!f) return false;
    f.print(content);
    f.flush();
    f.close();
    return true;
}

// Helper: read a file and print to Serial.
static void printFile(const char* path) {
    SDFile f = MultiduinoSD.open(path, FILE_READ);
    if (!f) { Serial.println(F("  [cannot open]")); return; }
    while (f.position() < f.size()) {
        int c = f.read();
        if (c < 0) break;
        Serial.write((char)c);
    }
    f.close();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println(F("=== SD File Manager ==="));
    Serial.println();

    if (!MultiduinoSD.begin()) {
        Serial.println(F("ERROR: SD init failed."));
        while (true) {}
    }
    MultiduinoSD.printCardInfo(Serial);
    Serial.println();

    // ------------------------------------------------------------------
    // 1. Create directory tree: /DEMO/LOGS, /DEMO/DATA
    // ------------------------------------------------------------------
    Serial.println(F("Creating directories /DEMO/LOGS and /DEMO/DATA..."));
    MultiduinoSD.mkdir("DEMO");
    MultiduinoSD.mkdir("DEMO/LOGS");
    MultiduinoSD.mkdir("DEMO/DATA");

    Serial.print(F("  /DEMO exists    : ")); Serial.println(MultiduinoSD.exists("DEMO")       ? "yes" : "no");
    Serial.print(F("  /DEMO/LOGS exists: ")); Serial.println(MultiduinoSD.exists("DEMO/LOGS") ? "yes" : "no");
    Serial.print(F("  /DEMO/DATA exists: ")); Serial.println(MultiduinoSD.exists("DEMO/DATA") ? "yes" : "no");
    Serial.println();

    // ------------------------------------------------------------------
    // 2. Create and write files
    // ------------------------------------------------------------------
    Serial.println(F("Writing files..."));
    writeFile("DEMO/LOGS/RUN001.TXT", "Boot #1: OK\r\nAll sensors nominal.\r\n");
    writeFile("DEMO/LOGS/RUN002.TXT", "Boot #2: SD slow init.\r\nA0=512 A1=0\r\n");
    writeFile("DEMO/DATA/CAL.TXT",    "offset=1.23\r\ngain=0.997\r\n");

    Serial.print(F("  RUN001.TXT: ")); printFile("DEMO/LOGS/RUN001.TXT");
    Serial.print(F("  RUN002.TXT: ")); printFile("DEMO/LOGS/RUN002.TXT");
    Serial.print(F("  CAL.TXT   : ")); printFile("DEMO/DATA/CAL.TXT");
    Serial.println();

    // ------------------------------------------------------------------
    // 3. Append to an existing file
    // ------------------------------------------------------------------
    Serial.println(F("Appending to RUN001.TXT..."));
    {
        SDFile f = MultiduinoSD.open("DEMO/LOGS/RUN001.TXT", FILE_WRITE);
        if (f) {
            f.println("Append: second session.");
            f.flush(); f.close();
        }
    }
    Serial.print(F("  After append: ")); printFile("DEMO/LOGS/RUN001.TXT");
    Serial.println();

    // ------------------------------------------------------------------
    // 4. Delete a file
    // ------------------------------------------------------------------
    Serial.print(F("Deleting RUN002.TXT ... "));
    bool ok = MultiduinoSD.remove("DEMO/LOGS/RUN002.TXT");
    Serial.println(ok ? F("OK") : F("FAILED"));
    Serial.print(F("  Exists now: "));
    Serial.println(MultiduinoSD.exists("DEMO/LOGS/RUN002.TXT") ? F("yes") : F("no"));
    Serial.println();

    // ------------------------------------------------------------------
    // 5. Remove empty directory (only works if directory is empty)
    // ------------------------------------------------------------------
    // First delete CAL.TXT so DATA dir is empty
    MultiduinoSD.remove("DEMO/DATA/CAL.TXT");
    Serial.print(F("Removing /DEMO/DATA (now empty) ... "));
    ok = MultiduinoSD.rmdir("DEMO/DATA");
    Serial.println(ok ? F("OK") : F("FAILED"));
    Serial.println();

    // ------------------------------------------------------------------
    // 6. Final directory tree
    // ------------------------------------------------------------------
    Serial.println(F("Final directory tree:"));
    MultiduinoSD.ls("/", Serial);
    Serial.println();

    Serial.println(F("Done."));
}

void loop() {
    // Nothing – all work done in setup()
}

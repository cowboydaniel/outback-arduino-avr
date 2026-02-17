/*
  SDDeleteAndRecreate
  ===================
  Demonstrates exists(), remove(), mkdir(), rmdir(), and re-creation of files
  and directories.  Verifies that removed files and directories no longer exist.

  Hardware: Multiduino with SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoSD.h>

static void report(const char* label, bool ok) {
    Serial.print(label); Serial.println(ok ? F(": OK") : F(": FAIL"));
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!")); while (true) {}
    }
    Serial.println(F("=== SDDeleteAndRecreate ===\n"));

    const char* FILE1 = "DTEST.TXT";
    const char* DIR1  = "DDIR";
    const char* FILE2 = "DDIR/INNER.TXT";

    // Clean up any leftovers from a previous run
    if (MultiduinoSD.exists(FILE2))                       MultiduinoSD.remove(FILE2);
    if (MultiduinoSD.exists(DIR1))                        MultiduinoSD.rmdir(DIR1);
    if (MultiduinoSD.exists(FILE1))                       MultiduinoSD.remove(FILE1);

    // 1. Create a file
    {
        SDFile f = MultiduinoSD.open(FILE1, SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
        if (f) { f.println(F("hello")); f.close(); }
    }
    report("File created            ", MultiduinoSD.exists(FILE1));

    // 2. Delete it
    MultiduinoSD.remove(FILE1);
    report("File removed            ", !MultiduinoSD.exists(FILE1));

    // 3. Re-create it
    {
        SDFile f = MultiduinoSD.open(FILE1, SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
        if (f) { f.println(F("recreated")); f.close(); }
    }
    report("File recreated          ", MultiduinoSD.exists(FILE1));

    // 4. Create directory
    MultiduinoSD.mkdir(DIR1);
    report("Directory created       ", MultiduinoSD.exists(DIR1));

    // 5. Create file inside directory
    {
        SDFile f = MultiduinoSD.open(FILE2, SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
        if (f) { f.println(F("inner")); f.close(); }
    }
    report("Inner file created      ", MultiduinoSD.exists(FILE2));

    // 6. Remove inner file then directory
    MultiduinoSD.remove(FILE2);
    report("Inner file removed      ", !MultiduinoSD.exists(FILE2));
    MultiduinoSD.rmdir(DIR1);
    report("Directory removed       ", !MultiduinoSD.exists(DIR1));

    // 7. List root to confirm state
    Serial.println(F("\nRoot listing:"));
    MultiduinoSD.ls("/", Serial);
    Serial.println(F("\nDone."));
}

void loop() {}

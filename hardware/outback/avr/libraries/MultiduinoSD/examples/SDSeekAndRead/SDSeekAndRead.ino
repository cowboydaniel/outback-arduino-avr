/*
  SDSeekAndRead
  =============
  Creates a binary file of known content, then demonstrates seek() by reading
  back specific byte offsets and comparing them with expected values.

  File layout: 256 bytes where byte[i] = i.

  Hardware: Multiduino with SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoSD.h>

#define TEST_FILE "SEEKTEST.BIN"
#define FILE_LEN  256

static bool report(const char* label, bool ok) {
    Serial.print(label); Serial.println(ok ? F(": OK") : F(": FAIL"));
    return ok;
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!")); while (true) {}
    }
    Serial.println(F("=== SDSeekAndRead ===\n"));

    // Write the test file
    if (MultiduinoSD.exists(TEST_FILE)) MultiduinoSD.remove(TEST_FILE);
    {
        SDFile f = MultiduinoSD.open(TEST_FILE,
                       SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
        if (!f) { Serial.println(F("create failed")); return; }
        for (uint16_t i = 0; i < FILE_LEN; i++) f.write((uint8_t)i);
        f.close();
        Serial.println(F("Written 256-byte file."));
    }

    // Read specific positions using seek()
    SDFile f = MultiduinoSD.open(TEST_FILE, FILE_READ);
    if (!f) { Serial.println(F("open failed")); return; }

    bool all = true;

    // Offsets to test
    const uint16_t offsets[] = {0, 1, 64, 127, 128, 200, 255};
    for (uint8_t i = 0; i < 7; i++) {
        uint16_t pos = offsets[i];
        if (!f.seek(pos)) { Serial.print(F("seek failed at ")); Serial.println(pos); all = false; continue; }
        int b = f.read();
        char label[24];
        snprintf(label, sizeof(label), "  seek(%3u) read=0x%02X", pos, (uint8_t)b);
        all &= report(label, b == pos);
    }

    // Test peek() – should not advance position
    f.seek(42);
    int p1 = f.peek();
    int p2 = f.peek();
    int r  = f.read();
    all &= report("  peek() does not advance", p1 == 42 && p2 == 42 && r == 42);

    // Test available()
    f.seek(200);
    all &= report("  available() at 200", f.available() == 56);

    f.close();
    Serial.println();
    Serial.println(all ? F("All checks PASSED") : F("Some checks FAILED"));
}

void loop() {}

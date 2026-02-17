/*
  SDAppendLog
  ===========
  Opens (or creates) LOG.TXT in append mode and writes one timestamped line
  per second.  After 10 lines it reads the file back from the beginning and
  prints every line via readLine().

  Hardware: Multiduino with SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoSD.h>
#include <MultiduinoRTC.h>

#define LOG_FILE   "LOG.TXT"
#define MAX_LINES  10

static uint8_t g_lineCount = 0;

static void appendLine() {
    DateTime dt = MultiduinoRTC.now();
    char ts[20]; dt.toString(ts);

    SDFile f = MultiduinoSD.open(LOG_FILE, FILE_WRITE);
    if (!f) { Serial.println(F("open failed")); return; }
    f.print(ts);
    f.print(F(" Line "));
    f.println(g_lineCount + 1 < 10 ? "0" : "");  // cheap zero-pad
    // Actually write the line count properly:
    f.close();

    // Reopen and rewrite properly – simpler: just use println with snprintf
    // (file was opened with append so we just write the formatted line)
}

static void appendLineProper() {
    DateTime dt = MultiduinoRTC.now();
    char buf[32];
    dt.toString(buf);  // "YYYY-MM-DD HH:MM:SS"

    SDFile f = MultiduinoSD.open(LOG_FILE, FILE_WRITE);
    if (!f) { Serial.println(F("open for append failed")); return; }
    f.print(buf);
    f.print(F(" #"));
    // print line number
    if (g_lineCount < 9) f.write('0');
    char num[4]; itoa(g_lineCount + 1, num, 10);
    f.print(num);
    f.println();
    f.close();
    Serial.print(F("Logged: ")); Serial.println(buf);
}

static void dumpLog() {
    SDFile f = MultiduinoSD.open(LOG_FILE, FILE_READ);
    if (!f) { Serial.println(F("open for read failed")); return; }
    Serial.println(F("\n--- LOG.TXT ---"));
    char line[48];
    while (f.available()) {
        uint16_t n = f.readLine(line, sizeof(line));
        if (n > 0) Serial.println(line);
    }
    Serial.println(F("--- end ---\n"));
    f.close();
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("RTC not found!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 0, 0, 0, 3));
    }
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!"));
        while (true) {}
    }
    Serial.println(F("SDAppendLog ready.  Writing 10 lines...\n"));
}

void loop() {
    if (g_lineCount < MAX_LINES) {
        appendLineProper();
        g_lineCount++;
        delay(1000);
    } else {
        dumpLog();
        g_lineCount = MAX_LINES + 1;  // stop
        while (true) {}
    }
}

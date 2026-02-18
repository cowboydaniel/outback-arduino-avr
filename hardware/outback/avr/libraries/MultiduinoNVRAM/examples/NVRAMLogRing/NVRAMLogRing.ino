/*
  NVRAMLogRing
  ============
  Implements a tiny circular (ring) log in NVRAM using NVRAMSlot<T>.

  Layout (starting at addr 2):
    addr  0–1 : header – write index (uint8_t) + entry count (uint8_t)
    addr  2–55: up to MAX_ENTRIES records of LOG_ENTRY_SIZE bytes each

  Each record stores a 1-byte event code and a full 4-byte Y2K-epoch
  timestamp (uint32_t, little-endian via NVRAM.put/get).
  The ring overwrites the oldest entry when full.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.  Press any key to log an event.
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

struct LogEntry {
    uint8_t  event;      // application-defined event code
    uint32_t epoch;      // full 32-bit Y2K epoch (seconds since 2000-01-01)
};

static const uint8_t LOG_ENTRY_SIZE = sizeof(LogEntry);  // 5 bytes
static const uint8_t HEADER_ADDR    = 0;                 // 2 bytes: head index, count
static const uint8_t LOG_BASE_ADDR  = 2;
static const uint8_t MAX_ENTRIES    = (NVRAM_SIZE - LOG_BASE_ADDR) / LOG_ENTRY_SIZE; // 10

static NVRAMSlot<uint8_t> slotHead(HEADER_ADDR);
static NVRAMSlot<uint8_t> slotCnt (HEADER_ADDR + 1);

static void logEvent(uint8_t eventCode) {
    uint8_t head = slotHead.load();
    uint8_t cnt  = slotCnt.load();

    LogEntry e;
    e.event = eventCode;
    e.epoch = MultiduinoRTC.now().toEpoch();

    uint8_t addr = LOG_BASE_ADDR + head * LOG_ENTRY_SIZE;
    NVRAM.put(addr, e);

    head = (head + 1) % MAX_ENTRIES;
    slotHead.save(head);
    if (cnt < MAX_ENTRIES) slotCnt.save(cnt + 1);

    Serial.print(F("Logged event 0x"));
    if (eventCode < 0x10) Serial.print('0');
    Serial.print(eventCode, HEX);
    Serial.print(F(" epoch=")); Serial.println(e.epoch);
}

static void dumpLog() {
    uint8_t cnt  = slotCnt.load();
    uint8_t head = slotHead.load();
    if (cnt == 0) { Serial.println(F("Log empty.")); return; }

    // Oldest entry is at (head - cnt + MAX_ENTRIES) % MAX_ENTRIES
    uint8_t oldest = (uint8_t)((head + MAX_ENTRIES - cnt) % MAX_ENTRIES);

    Serial.print(F("--- Log (")); Serial.print(cnt); Serial.print(F(" / "));
    Serial.print(MAX_ENTRIES); Serial.println(F(" entries) ---"));
    for (uint8_t i = 0; i < cnt; i++) {
        uint8_t idx  = (oldest + i) % MAX_ENTRIES;
        uint8_t addr = LOG_BASE_ADDR + idx * LOG_ENTRY_SIZE;
        LogEntry e;
        NVRAM.get(addr, e);
        Serial.print(F("  [")); Serial.print(i);
        Serial.print(F("] event=0x"));
        if (e.event < 0x10) Serial.print('0');
        Serial.print(e.event, HEX);
        Serial.print(F("  epoch=")); Serial.println(e.epoch);
    }
}

static uint8_t g_eventCode = 0x10;

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 0, 0, 0, 3));
    }

    // Init header if this is first boot (magic check not used here for brevity)
    if (slotCnt.load() > MAX_ENTRIES) {
        slotHead.save(0);
        slotCnt.save(0);
    }

    Serial.println(F("=== NVRAMLogRing ==="));
    Serial.print(F("Max entries: ")); Serial.println(MAX_ENTRIES);
    Serial.println(F("Send any character to log an event.\n"));
    dumpLog();
}

void loop() {
    if (Serial.available()) {
        while (Serial.available()) Serial.read();
        logEvent(g_eventCode++);
        if (g_eventCode > 0x1F) g_eventCode = 0x10; // cycle event codes
        Serial.println();
        dumpLog();
        Serial.println();
    }
}

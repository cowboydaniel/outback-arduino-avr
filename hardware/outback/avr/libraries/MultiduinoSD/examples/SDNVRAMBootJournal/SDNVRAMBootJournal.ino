/*
  SDNVRAMBootJournal
  ==================
  Cross-library example: MultiduinoRTC + MultiduinoNVRAM + MultiduinoSD

  Maintains a "boot journal" combining fast NVRAM and persistent SD storage:

  NVRAM role (addr 0–7):
    - Byte 0: boot counter (wraps at 255 then rolls to 0)
    - Bytes 1–4: Y2K epoch of last boot (uint32_t)
    - Bytes 5–6: layout magic + CRC

  SD role:
    - File "BOOTLOG.TXT" stores one line per boot:
        "boot=NNN  time=YYYY-MM-DD HH:MM:SS  uptime=UUU s"
    - The last line is the most recent boot.

  On each boot:
    1. Read previous boot info from NVRAM (if valid).
    2. Append a record to SD that includes the previous uptime estimate.
    3. Save the new boot timestamp and incremented counter to NVRAM.

  Hardware: Multiduino with DS1307 RTC + SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>
#include <MultiduinoSD.h>

#define MAGIC_ADDR   5    // 2 bytes: magic marker + CRC
#define CTR_ADDR     0    // 1 byte boot counter
#define EPOCH_ADDR   1    // 4 bytes last-boot epoch
#define BOOT_MAGIC   0xB0
#define BOOT_LOG     "BOOTLOG.TXT"

static NVRAMSlot<uint8_t>  slotCtr  (CTR_ADDR);
static NVRAMSlot<uint32_t> slotEpoch(EPOCH_ADDR);

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("RTC not found!")); while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 0, 0, 0, 3));
    }
    bool sdOk = MultiduinoSD.begin();

    DateTime now    = MultiduinoRTC.now();
    uint32_t epoch  = now.toEpoch();
    char ts[20];    now.toString(ts);

    Serial.println(F("=== SDNVRAMBootJournal ==="));
    Serial.print(F("Current time : ")); Serial.println(ts);

    uint8_t  bootCtr  = 0;
    uint32_t prevEpoch = 0;
    int32_t  uptime    = -1;

    if (NVRAM.checkMagic(MAGIC_ADDR, BOOT_MAGIC)) {
        bootCtr   = slotCtr.load() + 1;
        prevEpoch = slotEpoch.load();
        uptime    = (int32_t)(epoch - prevEpoch);
        Serial.print(F("Previous boot: epoch=")); Serial.println(prevEpoch);
        Serial.print(F("Uptime since : ")); Serial.print(uptime); Serial.println(F(" s"));
    } else {
        Serial.println(F("First boot – no previous NVRAM record."));
        bootCtr = 1;
    }

    // Append to SD log
    if (sdOk) {
        SDFile f = MultiduinoSD.open(BOOT_LOG, FILE_WRITE);
        if (f) {
            f.print(F("boot="));
            char num[8]; itoa(bootCtr, num, 10); f.print(num);
            f.print(F("  time="));   f.print(ts);
            f.print(F("  uptime="));
            if (uptime >= 0) { itoa((int)uptime, num, 10); f.print(num); f.print(F(" s")); }
            else              { f.print(F("n/a")); }
            f.println();
            f.close();
            Serial.println(F("Record written to BOOTLOG.TXT"));
        } else {
            Serial.println(F("SD write failed"));
        }
    }

    // Update NVRAM
    slotCtr.save(bootCtr);
    slotEpoch.save(epoch);
    NVRAM.writeMagic(MAGIC_ADDR, BOOT_MAGIC);

    Serial.print(F("Boot counter : ")); Serial.println(bootCtr);

    // Dump the last few lines of the boot log
    if (sdOk && MultiduinoSD.exists(BOOT_LOG)) {
        SDFile f = MultiduinoSD.open(BOOT_LOG, FILE_READ);
        if (f) {
            Serial.println(F("\n--- BOOTLOG.TXT ---"));
            char line[64];
            while (f.available()) {
                f.readLine(line, sizeof(line));
                if (strlen(line) > 0) Serial.println(line);
            }
            Serial.println(F("---"));
            f.close();
        }
    }
}

void loop() {}

/*
  RTCHeartbeatWatchdog
  ====================
  Uses the RTC to implement a software watchdog based on elapsed seconds.

  A "last-heartbeat" timestamp is stored in NVRAM (addr 0, 4 bytes as Y2K
  epoch).  On each loop iteration the sketch checks whether more than
  WATCHDOG_TIMEOUT_S seconds have elapsed since the heartbeat was refreshed.
  If the timeout expires the watchdog fires (here it just prints an alert;
  in production you would reset the MCU or take recovery action).

  Press any Serial key to refresh the heartbeat manually.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

#define WATCHDOG_TIMEOUT_S  15   // Fire if no heartbeat for 15 seconds
#define HEARTBEAT_ADDR       0   // NVRAM address for the 4-byte epoch timestamp

static void saveHeartbeat(uint32_t epoch) {
    NVRAM.put(HEARTBEAT_ADDR, epoch);
    Serial.print(F("[WDT] Heartbeat saved, epoch=")); Serial.println(epoch);
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        MultiduinoRTC.adjust(DateTime(2025, 1, 1, 0, 0, 0, 3));
    }

    uint32_t now = MultiduinoRTC.now().toEpoch();
    saveHeartbeat(now);
    Serial.println(F("Watchdog armed.  Send any character to reset."));
    Serial.print(F("Timeout: ")); Serial.print(WATCHDOG_TIMEOUT_S); Serial.println(F("s\n"));
}

void loop() {
    // Refresh heartbeat on Serial input
    if (Serial.available()) {
        while (Serial.available()) Serial.read();
        uint32_t now = MultiduinoRTC.now().toEpoch();
        saveHeartbeat(now);
    }

    uint32_t now  = MultiduinoRTC.now().toEpoch();
    uint32_t last = 0;
    NVRAM.get(HEARTBEAT_ADDR, last);

    int32_t age = (int32_t)(now - last);
    Serial.print(F("Age: ")); Serial.print(age); Serial.print(F("s / "));
    Serial.print(WATCHDOG_TIMEOUT_S); Serial.println(F("s"));

    if (age >= WATCHDOG_TIMEOUT_S) {
        Serial.println(F("*** WATCHDOG FIRED! System would reset here. ***"));
        // Reset heartbeat so alert fires only once per period
        saveHeartbeat(now);
    }
    delay(3000);
}

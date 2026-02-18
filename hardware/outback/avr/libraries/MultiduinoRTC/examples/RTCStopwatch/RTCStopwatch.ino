/*
  RTCStopwatch
  ------------
  A lap-timer / stopwatch that uses the DS1307 RTC as the time source.
  Because the RTC is battery-backed, elapsed time is preserved even if
  the MCU is reset or power-cycled mid-session.

  Controls (via Serial – send a single character):
    's'  – Start / resume the stopwatch
    'p'  – Pause the stopwatch
    'l'  – Record a lap (up to MAX_LAPS stored in NVRAM)
    'r'  – Reset (clear elapsed time and laps, wipe NVRAM)
    '?'  – Print current status

  NVRAM layout (up to 46 bytes of the 56 available):
    Addr 0       : state byte (0x00=reset, 0x01=running, 0x02=paused)
    Addr 1–4     : uint32_t accumulated elapsed seconds before last pause
    Addr 5       : uint8_t  start-epoch seconds (low byte)  \  4-byte epoch
    Addr 6–9     : uint32_t start epoch (seconds since 2000-01-01)
    Addr 10      : uint8_t  lap count
    Addr 11–10+laps*3 : lap entries (3 bytes each: HH MM SS of elapsed)

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/
#ifndef ARDUINO_AVR_MULTIDUINO
#error "This example is for Multiduino only — select \'Multiduino\' from the Boards menu."
#endif


#include <MultiduinoRTC.h>

#define MAX_LAPS      10
#define ADDR_STATE    0
#define ADDR_ACCUM    1   // 4 bytes
#define ADDR_START    5   // 4 bytes (epoch seconds when last started)
#define ADDR_LAPCOUNT 9
#define ADDR_LAPS     10  // 3 bytes × MAX_LAPS

// Convert a DateTime to seconds since 2000-01-01 (ignores leap seconds)
uint32_t toEpoch(const DateTime& dt) {
    // Days per month (non-leap)
    static const uint16_t kDoyBase[] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    uint32_t days = (uint32_t)(dt.year - 2000) * 365UL;
    days += (dt.year - 2001) / 4;    // Leap years since 2000
    days += kDoyBase[dt.month] + dt.day - 1;
    if (dt.month > 2 && (dt.year % 4 == 0)) days++;  // Leap day this year
    return days * 86400UL + (uint32_t)dt.hour * 3600UL +
           (uint32_t)dt.minute * 60UL + dt.second;
}

void printElapsed(uint32_t secs) {
    uint32_t h = secs / 3600;
    uint8_t  m = (secs % 3600) / 60;
    uint8_t  s = secs % 60;
    if (h < 10) Serial.print('0'); Serial.print(h); Serial.print(':');
    if (m < 10) Serial.print('0'); Serial.print(m); Serial.print(':');
    if (s < 10) Serial.print('0'); Serial.print(s);
}

void printStatus() {
    uint8_t  state = MultiduinoRTC.readNVRAM(ADDR_STATE);
    uint32_t accum;
    MultiduinoRTC.readNVRAM(ADDR_ACCUM, (uint8_t*)&accum, 4);
    uint32_t startEpoch;
    MultiduinoRTC.readNVRAM(ADDR_START, (uint8_t*)&startEpoch, 4);
    uint8_t lapCount = MultiduinoRTC.readNVRAM(ADDR_LAPCOUNT);

    Serial.print(F("State   : "));
    switch (state) {
        case 0x01: Serial.println(F("RUNNING")); break;
        case 0x02: Serial.println(F("PAUSED"));  break;
        default:   Serial.println(F("RESET"));   break;
    }

    uint32_t elapsed = accum;
    if (state == 0x01) {
        elapsed += toEpoch(MultiduinoRTC.now()) - startEpoch;
    }
    Serial.print(F("Elapsed : "));
    printElapsed(elapsed);
    Serial.println();

    Serial.print(F("Laps    : ")); Serial.println(lapCount);
    for (uint8_t i = 0; i < lapCount; i++) {
        uint8_t lap[3];
        MultiduinoRTC.readNVRAM(ADDR_LAPS + i * 3, lap, 3);
        Serial.print(F("  Lap ")); Serial.print(i + 1); Serial.print(F(" : "));
        if (lap[0] < 10) Serial.print('0'); Serial.print(lap[0]); Serial.print(':');
        if (lap[1] < 10) Serial.print('0'); Serial.print(lap[1]); Serial.print(':');
        if (lap[2] < 10) Serial.print('0'); Serial.println(lap[2]);
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found."));
        while (true) {}
    }
    if (!MultiduinoRTC.isRunning()) {
        const char* ds = __DATE__; const char* ts = __TIME__;
        const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mn[4] = {ds[0], ds[1], ds[2], 0};
        uint8_t mon = 1;
        for (uint8_t i = 0; i < 12; i++) {
            if (strncmp(mn, months + i * 3, 3) == 0) { mon = i + 1; break; }
        }
        MultiduinoRTC.adjust(DateTime((uint16_t)atoi(ds + 7), mon,
            (uint8_t)atoi(ds + 4), (uint8_t)atoi(ts),
            (uint8_t)atoi(ts + 3), (uint8_t)atoi(ts + 6)));
    }

    Serial.println(F("=== RTC Stopwatch ==="));
    Serial.println(F("Commands: s=start  p=pause  l=lap  r=reset  ?=status"));
    printStatus();
}

void loop() {
    if (!Serial.available()) return;
    char cmd = (char)Serial.read();

    uint8_t  state = MultiduinoRTC.readNVRAM(ADDR_STATE);
    uint32_t accum;
    MultiduinoRTC.readNVRAM(ADDR_ACCUM, (uint8_t*)&accum, 4);
    uint32_t startEpoch;
    MultiduinoRTC.readNVRAM(ADDR_START, (uint8_t*)&startEpoch, 4);

    if (cmd == 's') {
        if (state != 0x01) {
            startEpoch = toEpoch(MultiduinoRTC.now());
            MultiduinoRTC.writeNVRAM(ADDR_STATE, 0x01);
            MultiduinoRTC.writeNVRAM(ADDR_START, (uint8_t*)&startEpoch, 4);
            Serial.println(F("Started."));
        }
    } else if (cmd == 'p') {
        if (state == 0x01) {
            accum += toEpoch(MultiduinoRTC.now()) - startEpoch;
            MultiduinoRTC.writeNVRAM(ADDR_STATE, 0x02);
            MultiduinoRTC.writeNVRAM(ADDR_ACCUM, (uint8_t*)&accum, 4);
            Serial.print(F("Paused at ")); printElapsed(accum); Serial.println();
        }
    } else if (cmd == 'l') {
        if (state == 0x01) {
            uint32_t elapsed = accum + toEpoch(MultiduinoRTC.now()) - startEpoch;
            uint8_t lapCount = MultiduinoRTC.readNVRAM(ADDR_LAPCOUNT);
            if (lapCount < MAX_LAPS) {
                uint8_t lapData[3] = {
                    (uint8_t)(elapsed / 3600),
                    (uint8_t)((elapsed % 3600) / 60),
                    (uint8_t)(elapsed % 60)
                };
                MultiduinoRTC.writeNVRAM(ADDR_LAPS + lapCount * 3, lapData, 3);
                MultiduinoRTC.writeNVRAM(ADDR_LAPCOUNT, lapCount + 1);
                Serial.print(F("Lap ")); Serial.print(lapCount + 1);
                Serial.print(F(" : ")); printElapsed(elapsed); Serial.println();
            } else {
                Serial.println(F("Max laps reached."));
            }
        }
    } else if (cmd == 'r') {
        MultiduinoRTC.clearNVRAM();
        Serial.println(F("Reset."));
    } else if (cmd == '?') {
        printStatus();
    }
}

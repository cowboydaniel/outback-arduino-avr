/*
  RTCAlarm
  --------
  Implements a software alarm clock using the DS1307 RTC.
  The alarm time is stored in NVRAM so it persists across resets.

  The DS1307 does not have built-in alarm hardware, so this sketch
  polls the RTC once per second in loop() and compares the current
  time against a stored alarm target.

  When the alarm fires:
    - "ALARM!" is printed to Serial every second for ALARM_DURATION_SEC
    - The built-in LED blinks at 4 Hz for the duration

  NVRAM layout (4 bytes used from the raw DS1307 API):
    Addr 0  : alarm hour   (0–23)
    Addr 1  : alarm minute (0–59)
    Addr 2  : alarm second (0–59)
    Addr 3  : magic byte 0xA1 (indicates alarm is set)

  Modify kAlarmHour / kAlarmMinute / kAlarmSecond below and re-flash.

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>

// ---- Set your desired alarm time here -----------------------------------
static const uint8_t kAlarmHour   = 7;
static const uint8_t kAlarmMinute = 30;
static const uint8_t kAlarmSecond = 0;
// -------------------------------------------------------------------------

#define ALARM_DURATION_SEC  30    // How long the alarm sounds
#define ALARM_MAGIC         0xA1  // Marker stored in NVRAM

void storeAlarm(uint8_t h, uint8_t m, uint8_t s) {
    MultiduinoRTC.writeNVRAM(0, h);
    MultiduinoRTC.writeNVRAM(1, m);
    MultiduinoRTC.writeNVRAM(2, s);
    MultiduinoRTC.writeNVRAM(3, ALARM_MAGIC);
}

bool loadAlarm(uint8_t& h, uint8_t& m, uint8_t& s) {
    if (MultiduinoRTC.readNVRAM(3) != ALARM_MAGIC) return false;
    h = MultiduinoRTC.readNVRAM(0);
    m = MultiduinoRTC.readNVRAM(1);
    s = MultiduinoRTC.readNVRAM(2);
    return true;
}

uint8_t alarmH, alarmM, alarmS;
bool    alarmArmed  = false;
bool    alarmRinging = false;
uint32_t alarmStartMs = 0;
uint32_t lastPollMs   = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    pinMode(LED_BUILTIN, OUTPUT);

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    // Initialise RTC to compile time if not running
    if (!MultiduinoRTC.isRunning()) {
        // Parse __DATE__ ("Mon DD YYYY") and __TIME__ ("HH:MM:SS")
        const char* dateStr = __DATE__;
        const char* timeStr = __TIME__;
        const char* months  = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char mname[4] = {dateStr[0], dateStr[1], dateStr[2], 0};
        uint8_t mon = 1;
        for (uint8_t i = 0; i < 12; i++) {
            if (strncmp(mname, months + i * 3, 3) == 0) { mon = i + 1; break; }
        }
        uint16_t yr  = atoi(dateStr + 7);
        uint8_t  dy  = atoi(dateStr + 4);
        uint8_t  hr  = atoi(timeStr);
        uint8_t  min = atoi(timeStr + 3);
        uint8_t  sec = atoi(timeStr + 6);
        MultiduinoRTC.adjust(DateTime(yr, mon, dy, hr, min, sec));
        Serial.println(F("RTC set to compile time."));
    }

    // Store alarm in NVRAM
    storeAlarm(kAlarmHour, kAlarmMinute, kAlarmSecond);
    alarmArmed = loadAlarm(alarmH, alarmM, alarmS);

    Serial.print(F("Alarm set for "));
    if (alarmH < 10) Serial.print('0');
    Serial.print(alarmH); Serial.print(':');
    if (alarmM < 10) Serial.print('0');
    Serial.print(alarmM); Serial.print(':');
    if (alarmS < 10) Serial.print('0');
    Serial.println(alarmS);
    Serial.println(F("Waiting... (current time printed every 10 s)"));
}

void loop() {
    uint32_t now = millis();

    // Poll RTC once per second
    if (now - lastPollMs >= 1000) {
        lastPollMs = now;
        DateTime dt = MultiduinoRTC.now();
        char buf[20];
        dt.toString(buf);

        if (alarmRinging) {
            // Check if alarm duration has elapsed
            if (now - alarmStartMs >= (uint32_t)ALARM_DURATION_SEC * 1000UL) {
                alarmRinging = false;
                alarmArmed   = false;
                digitalWrite(LED_BUILTIN, LOW);
                Serial.println(F("Alarm dismissed."));
            } else {
                Serial.print(F("*** ALARM! *** "));
                Serial.println(buf);
            }
        } else if (alarmArmed) {
            // Check if it is alarm time
            if (dt.hour == alarmH && dt.minute == alarmM && dt.second == alarmS) {
                alarmRinging  = true;
                alarmStartMs  = now;
                Serial.println(F("*** ALARM FIRED! ***"));
            } else if (dt.second % 10 == 0) {
                // Print time every 10 seconds
                Serial.println(buf);
            }
        }
    }

    // Blink LED at 4 Hz while alarm is ringing
    if (alarmRinging) {
        digitalWrite(LED_BUILTIN, (millis() / 125) % 2);
    }
}

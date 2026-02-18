/*
  RTCSqwOutput
  ------------
  Demonstrates control of the DS1307 Square Wave Output (SQW/OUT pin).

  The DS1307 control register (0x07) allows the SQW pin to output one of
  four frequencies, or to be driven as a simple logic-level output:

    Mode              | SQWE | RS1 | RS0 | SQW behaviour
    ------------------|------|-----|-----|-----------------------------
    1 Hz              |  1   |  0  |  0  | 1 Hz square wave
    4096 Hz           |  1   |  0  |  1  | 4.096 kHz square wave
    8192 Hz           |  1   |  1  |  0  | 8.192 kHz square wave
    32768 Hz          |  1   |  1  |  1  | 32.768 kHz square wave (XTAL)
    Output LOW        |  0   |  x  |  x  | Pin driven LOW
    Output HIGH       |  0   |  x  |  x  | Pin driven HIGH (OUTB=1)

  This sketch cycles through all four square-wave frequencies (3 seconds
  each) while printing the current RTC time, then disables SQW output.

  Connect SQW/OUT (DS1307 pin 7) to an oscilloscope or an LED+resistor
  to observe the output.

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
           SQW/OUT on DS1307 pin 7 (not broken out on all boards –
           check your Multiduino schematic).
*/

#include <MultiduinoRTC.h>

// DS1307 control register bits
#define DS1307_CTRL_REG   0x07
#define DS1307_CTRL_SQWE  0x10   // Square Wave Enable
#define DS1307_CTRL_RS0   0x01   // Rate Select bit 0
#define DS1307_CTRL_RS1   0x02   // Rate Select bit 1
#define DS1307_CTRL_OUT   0x80   // Output level when SQWE=0

// F() is a statement-expression and cannot be used in a file-scope initialiser.
// Declare each label as a named PROGMEM array and reference it via PGM_P instead.
static const char kSqwLabel0[] PROGMEM = "1 Hz      ";
static const char kSqwLabel1[] PROGMEM = "4096 Hz   ";
static const char kSqwLabel2[] PROGMEM = "8192 Hz   ";
static const char kSqwLabel3[] PROGMEM = "32768 Hz  ";
static const char kSqwLabel4[] PROGMEM = "OFF (LOW) ";
static const char kSqwLabel5[] PROGMEM = "OFF (HIGH)";

struct SqwMode {
    PGM_P   label;     // pointer to a PROGMEM string
    uint8_t ctrlByte;
};

static const SqwMode kModes[] = {
    { kSqwLabel0, DS1307_CTRL_SQWE | 0x00 },
    { kSqwLabel1, DS1307_CTRL_SQWE | DS1307_CTRL_RS0 },
    { kSqwLabel2, DS1307_CTRL_SQWE | DS1307_CTRL_RS1 },
    { kSqwLabel3, DS1307_CTRL_SQWE | DS1307_CTRL_RS1 | DS1307_CTRL_RS0 },
    { kSqwLabel4, 0x00 },
    { kSqwLabel5, DS1307_CTRL_OUT },
};

uint8_t  modeIndex  = 0;
uint32_t modeStart  = 0;
uint32_t lastPrint  = 0;
#define  MODE_DURATION_MS  3000UL

void applyMode(uint8_t idx) {
    // Write directly to the DS1307 control register via the NVRAM-adjacent reg.
    // The control register is at address 0x07, right before NVRAM (0x08).
    // Wire is already initialised by MultiduinoRTC.begin().
    Wire.beginTransmission(0x68);
    Wire.write(DS1307_CTRL_REG);
    Wire.write(kModes[idx].ctrlByte);
    Wire.endTransmission();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
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

    Serial.println(F("=== DS1307 SQW Output Demo ==="));
    Serial.println(F("Cycling through SQW modes every 3 seconds."));
    Serial.println(F("Monitor SQW/OUT pin (DS1307 pin 7) on oscilloscope or LED."));
    Serial.println();

    applyMode(0);
    modeStart = millis();
    lastPrint = millis();
}

void loop() {
    uint32_t now = millis();

    // Print current time and mode once per second
    if (now - lastPrint >= 1000) {
        lastPrint = now;
        DateTime dt = MultiduinoRTC.now();
        char buf[20];
        dt.toString(buf);
        Serial.print(buf);
        Serial.print(F("  SQW: "));
        Serial.println((const __FlashStringHelper*)kModes[modeIndex].label);
    }

    // Advance to next mode every 3 seconds
    if (now - modeStart >= MODE_DURATION_MS) {
        modeStart = now;
        modeIndex = (modeIndex + 1) % (sizeof(kModes) / sizeof(kModes[0]));
        applyMode(modeIndex);
        Serial.print(F(">> Mode changed to: "));
        Serial.println((const __FlashStringHelper*)kModes[modeIndex].label);
    }
}

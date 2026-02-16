/*
  NVRAMBootCounter
  ----------------
  Uses NVRAM to persist a boot counter and last-boot timestamp across
  power cycles.  On every reset/power-on the counter is incremented,
  the current RTC time is stamped, and both are printed to Serial.

  NVRAM layout (10 bytes used out of 56):
    Addr  0 – 1  : magic marker + CRC  (checkMagic / writeMagic)
    Addr  2 – 5  : uint32_t boot count (little-endian, put/get)
    Addr  6 – 9  : packed timestamp    (4 bytes: YY MM DD HH)

  The magic byte (0xBC) lets us detect a fresh battery install or a
  deliberate clearNVRAM() and reset the counter to zero in that case.

  Hardware: Multiduino (ATmega328P), DS1307 RTC on I2C (A4=SDA, A5=SCL).
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

// NVRAM address map
#define ADDR_MAGIC      0   // 2 bytes: magic byte + CRC
#define ADDR_COUNT      2   // 4 bytes: uint32_t boot counter
#define ADDR_TIMESTAMP  6   // 4 bytes: year-2000, month, day, hour

// Chosen magic byte – any non-zero value works
#define MAGIC_BYTE      0xBC

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    if (!MultiduinoRTC.begin()) {
        Serial.println(F("ERROR: DS1307 not found. Check wiring."));
        while (true) {}
    }

    Serial.println(F("=== NVRAM Boot Counter ==="));
    Serial.println();

    uint32_t count;

    if (NVRAM.checkMagic(ADDR_MAGIC, MAGIC_BYTE)) {
        // NVRAM is valid – read and increment the existing counter
        NVRAM.get(ADDR_COUNT, count);
        count++;
    } else {
        // First boot after battery install or NVRAM clear
        Serial.println(F("NVRAM not initialised – starting fresh counter."));
        count = 1;
        NVRAM.writeMagic(ADDR_MAGIC, MAGIC_BYTE);
    }

    // Write updated counter back
    NVRAM.put(ADDR_COUNT, count);

    // Stamp current RTC time (4 bytes: YY MM DD HH)
    DateTime dt = MultiduinoRTC.now();
    uint8_t ts[4] = {
        (uint8_t)(dt.year - 2000),
        dt.month,
        dt.day,
        dt.hour
    };
    NVRAM.writeBlock(ADDR_TIMESTAMP, ts, 4);

    // Print results
    Serial.print(F("Boot count : "));
    Serial.println(count);

    char buf[20];
    dt.toString(buf);
    Serial.print(F("Booted at  : "));
    Serial.println(buf);

    Serial.println();
    Serial.println(F("Reset the board to see the count increment!"));
}

void loop() {
    // Nothing – all work done in setup()
}

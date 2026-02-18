/*
 * SRAMTypedAccess
 *
 * Store and retrieve any POD (plain-old-data) struct using SRAM.put() / SRAM.get().
 * Also demonstrates SRAMSlot<T> for named, address-fixed storage.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoSRAM.h>

struct SensorReading {
    uint32_t timestamp;   // Y2K epoch seconds
    int16_t  tempC10;     // temperature × 10 (e.g. 235 = 23.5 °C)
    uint16_t humidPct10;  // humidity × 10
    uint8_t  flags;
};

// Named slot: SensorReading lives at SRAM byte 0
SRAMSlot<SensorReading> latestReading(0);

// Another slot immediately after, no overlap
SRAMSlot<uint32_t> bootCounter(sizeof(SensorReading));

void setup() {
    Serial.begin(115200);
    while (!Serial);

    SRAM.begin();
    Serial.println(F("MultiduinoSRAM — typed access"));

    // Increment and save boot counter
    uint32_t boots = bootCounter.load();
    boots++;
    bootCounter.save(boots);
    Serial.print(F("Boot count: "));
    Serial.println(boots);

    // Write a sensor reading
    SensorReading r;
    r.timestamp  = 789012345UL;
    r.tempC10    = 235;   // 23.5 °C
    r.humidPct10 = 612;   // 61.2 %
    r.flags      = 0x01;
    latestReading.save(r);

    // Read it back
    SensorReading r2;
    latestReading.load(r2);

    Serial.print(F("Timestamp: ")); Serial.println(r2.timestamp);
    Serial.print(F("Temp:      ")); Serial.print(r2.tempC10 / 10);
    Serial.print('.');              Serial.println(abs(r2.tempC10 % 10));
    Serial.print(F("Humidity:  ")); Serial.print(r2.humidPct10 / 10);
    Serial.print('.');              Serial.println(r2.humidPct10 % 10);
    Serial.print(F("Flags:     0x")); Serial.println(r2.flags, HEX);

    bool ok = (r2.timestamp == r.timestamp &&
               r2.tempC10   == r.tempC10   &&
               r2.humidPct10== r.humidPct10&&
               r2.flags     == r.flags);
    Serial.println(ok ? F("Round-trip OK.") : F("ERROR: data mismatch!"));
}

void loop() {}

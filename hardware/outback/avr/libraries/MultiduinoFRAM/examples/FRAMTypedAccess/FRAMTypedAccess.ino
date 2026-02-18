/*
 * FRAMTypedAccess
 *
 * Store and retrieve structs using FRAM.put() / FRAM.get() and FRAMSlot<T>.
 * Values persist across power cycles — run twice to see the stored counter.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoFRAM.h>

struct DeviceState {
    uint32_t bootCount;
    float    lastTempC;
    uint8_t  status;
};

FRAMSlot<DeviceState> stateSlot(0);

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!FRAM.begin()) {
        Serial.println(F("FRAM not found.")); while (1);
    }

    DeviceState state = stateSlot.load();
    state.bootCount++;
    state.lastTempC = 22.5f;
    state.status    = 0x01;
    stateSlot.save(state);

    Serial.print(F("Boot count: "));  Serial.println(state.bootCount);
    Serial.print(F("Last temp:  "));  Serial.println(state.lastTempC, 1);
    Serial.print(F("Status:     0x")); Serial.println(state.status, HEX);
    Serial.println(F("(Boot count increments each power cycle)"));
}

void loop() {}

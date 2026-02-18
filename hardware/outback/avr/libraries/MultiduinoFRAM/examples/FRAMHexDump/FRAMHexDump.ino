/*
 * FRAMHexDump
 *
 * Prints a hex + ASCII dump of the first 64 bytes of FRAM to Serial.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>
#include <MultiduinoFRAM.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    MultiduinoPower.begin();
    delay(5);

    if (!FRAM.begin()) {
        Serial.println(F("FRAM not found.")); while (1);
    }

    Serial.println(F("FRAM hex dump — first 64 bytes:"));
    FRAM.dump(0, 64, Serial);
}

void loop() {}

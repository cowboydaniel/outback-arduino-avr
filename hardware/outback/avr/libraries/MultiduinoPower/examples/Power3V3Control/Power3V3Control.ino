/*
 * Power3V3Control
 *
 * Basic demonstration of enabling and disabling the on-board 3.3V rail
 * via MultiduinoPower.  The rail state is printed to Serial on each change.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_AVR_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoPower.h>

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Start with 3.3V rail ON (default)
    MultiduinoPower.begin();
    Serial.print(F("3.3V rail: "));
    Serial.println(MultiduinoPower.is3V3Enabled() ? F("ON") : F("OFF"));
}

void loop() {
    delay(3000);
    MultiduinoPower.disable3V3();
    Serial.println(F("3.3V rail: OFF"));

    delay(1000);
    MultiduinoPower.enable3V3();
    Serial.println(F("3.3V rail: ON"));
}

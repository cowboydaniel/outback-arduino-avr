#include "MultiduinoPower.h"

void MultiduinoPowerClass::begin(bool enableOnStart) {
    pinMode(PIN_3V3_EN, OUTPUT);
    if (enableOnStart) {
        enable3V3();
    } else {
        disable3V3();
    }
}

void MultiduinoPowerClass::enable3V3() {
    digitalWrite(PIN_3V3_EN, HIGH);
    _enabled = true;
}

void MultiduinoPowerClass::disable3V3() {
    digitalWrite(PIN_3V3_EN, LOW);
    _enabled = false;
}

// Global instance
MultiduinoPowerClass MultiduinoPower;

#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// MultiduinoPower — 3.3V rail control for Multiduino v2
//
// The Multiduino v2 routes the 3.3V power rail enable through PE3 (D23),
// connected via JP6.  Driving PE3 HIGH enables the rail; LOW disables it.
//
// Default after begin(): rail ENABLED (safe default — peripherals powered).
//
// Relevant pin alias from variants/multiduino_v2/pins_arduino.h:
//   static const uint8_t PIN_3V3_EN = 23;  // PE3
//
// Multiduino v2 only.  The 3.3V rail enable circuit is not present on the
// original Multiduino.
// ---------------------------------------------------------------------------

#ifndef PIN_3V3_EN
#define PIN_3V3_EN 23
#endif

class MultiduinoPowerClass {
public:
    // Call once in setup().
    // enableOnStart = true  → rail is immediately enabled (default).
    // enableOnStart = false → rail starts disabled; call enable3V3() when ready.
    void begin(bool enableOnStart = true);

    // Enable the 3.3V rail (drive PE3 HIGH).
    void enable3V3();

    // Disable the 3.3V rail (drive PE3 LOW).
    // Allow at least 1 ms after disable before re-enabling to let decoupling
    // capacitors discharge and the regulator fully shut down.
    void disable3V3();

    // Returns true if the rail is currently enabled.
    bool is3V3Enabled() const { return _enabled; }

private:
    bool _enabled = false;
};

extern MultiduinoPowerClass MultiduinoPower;

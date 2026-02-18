/*
 * PowerPeripheralSleep
 *
 * Demonstrates a typical pattern: take a sensor reading while peripherals
 * are powered, then cut the 3.3V rail between samples to save current.
 *
 * On the Multiduino v2 the RTC, SRAM, and SD card all sit on the 3.3V rail.
 * Disabling it between readings can significantly reduce average current draw
 * in battery-powered applications.
 *
 * Multiduino v2 only.
 */

#include <MultiduinoPower.h>
#include <MultiduinoRTC.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// Watchdog wakeup interval — choose one of the WDP constants below.
// WDTO_1S = 1 second, WDTO_2S = 2 s, WDTO_4S = 4 s, WDTO_8S = 8 s
#define SLEEP_PERIOD WDTO_8S

volatile bool wdtFired = false;

ISR(WDT_vect) {
    wdtFired = true;
}

void armWatchdog(uint8_t period) {
    cli();
    wdt_reset();
    MCUSR &= ~(1 << WDRF);
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR  = (1 << WDIE) | period;   // interrupt mode, no reset
    sei();
}

void doSleep() {
    wdtFired = false;
    armWatchdog(SLEEP_PERIOD);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    wdt_disable();
}

void takeSample() {
    // 3.3V rail is now ON — peripherals have had time to stabilise
    DateTime dt = MultiduinoRTC.now();

    char buf[20];
    dt.toString(buf);
    Serial.print(F("Sample at: "));
    Serial.println(buf);
    // ... read sensors, write to SD, etc. ...
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Bring up peripherals with 3.3V rail ON
    MultiduinoPower.begin(true);
    delay(10);  // allow regulator and decoupling caps to settle

    MultiduinoRTC.begin();
    Serial.println(F("PowerPeripheralSleep — sampling every ~8 s"));
}

void loop() {
    takeSample();

    // Cut 3.3V rail — powers down RTC, SRAM, SD
    MultiduinoPower.disable3V3();

    // Sleep until watchdog fires
    doSleep();

    // Restore 3.3V and allow peripherals to power up before accessing them
    MultiduinoPower.enable3V3();
    delay(5);
}

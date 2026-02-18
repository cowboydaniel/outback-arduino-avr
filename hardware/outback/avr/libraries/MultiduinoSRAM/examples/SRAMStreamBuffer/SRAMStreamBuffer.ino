/*
 * SRAMStreamBuffer
 *
 * Uses the SRAM as a large circular buffer to capture Serial input
 * and play it back. Demonstrates block writes and reads across a
 * 256-byte window that wraps around in the SRAM address space.
 *
 * Multiduino v2 only.
 */
#ifndef ARDUINO_MULTIDUINO_V2
#error "This example is for Multiduino v2 only — select \'Multiduino v2\' from the Boards menu."
#endif


#include <MultiduinoSRAM.h>

static const uint32_t BUF_ADDR = 0x00000UL;
static const uint32_t BUF_SIZE = 1024UL;   // 1 KB ring buffer in SRAM

static uint32_t writeHead = 0;
static uint32_t readHead  = 0;
static uint32_t count     = 0;

void sramPush(uint8_t b) {
    if (count >= BUF_SIZE) return;  // full
    SRAM.write(BUF_ADDR + writeHead, b);
    writeHead = (writeHead + 1) % BUF_SIZE;
    count++;
}

int sramPop() {
    if (count == 0) return -1;  // empty
    uint8_t b = SRAM.read(BUF_ADDR + readHead);
    readHead = (readHead + 1) % BUF_SIZE;
    count--;
    return b;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    SRAM.begin();
    Serial.println(F("MultiduinoSRAM stream buffer — type something, press Enter to echo."));
}

void loop() {
    // Push incoming bytes into SRAM ring buffer
    while (Serial.available()) {
        char c = (char)Serial.read();
        sramPush((uint8_t)c);
        if (c == '\n') {
            // Echo everything buffered
            Serial.print(F("Echo: "));
            int b;
            while ((b = sramPop()) != -1) {
                Serial.write((uint8_t)b);
            }
        }
    }
}

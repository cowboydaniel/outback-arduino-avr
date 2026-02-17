/*
  NVRAMTypeTest
  =============
  Exercises put()/get() with several POD types to confirm the template
  read-back round-trip is correct across all widths.

  Each type is written with a known value, read back, and compared.
  Results are printed to Serial.

  Hardware: Multiduino with DS1307 RTC.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoRTC.h>
#include <MultiduinoNVRAM.h>

// NVRAM layout used by this sketch:
//   addr  0  : uint8_t  (1 byte)
//   addr  1  : int8_t   (1 byte)
//   addr  2  : uint16_t (2 bytes)
//   addr  4  : int16_t  (2 bytes)
//   addr  6  : uint32_t (4 bytes)
//   addr 10  : int32_t  (4 bytes)
//   addr 14  : float    (4 bytes)
//   addr 18  : bool     (1 byte)

static bool check(const char* label, bool ok) {
    Serial.print(label);
    Serial.print(F(": "));
    Serial.println(ok ? F("OK") : F("FAIL"));
    return ok;
}

void setup() {
    Serial.begin(115200);
    if (!MultiduinoRTC.begin()) {
        Serial.println(F("DS1307 not found!"));
        while (true) {}
    }

    Serial.println(F("=== NVRAMTypeTest ===\n"));

    uint8_t  u8  = 0xAB;
    int8_t   i8  = -42;
    uint16_t u16 = 0x1234;
    int16_t  i16 = -1000;
    uint32_t u32 = 0xDEADBEEFUL;
    int32_t  i32 = -123456L;
    float    f32 = 3.14159f;
    bool     b   = true;

    NVRAM.put( 0, u8);
    NVRAM.put( 1, i8);
    NVRAM.put( 2, u16);
    NVRAM.put( 4, i16);
    NVRAM.put( 6, u32);
    NVRAM.put(10, i32);
    NVRAM.put(14, f32);
    NVRAM.put(18, b);

    uint8_t  ru8;  NVRAM.get( 0, ru8);
    int8_t   ri8;  NVRAM.get( 1, ri8);
    uint16_t ru16; NVRAM.get( 2, ru16);
    int16_t  ri16; NVRAM.get( 4, ri16);
    uint32_t ru32; NVRAM.get( 6, ru32);
    int32_t  ri32; NVRAM.get(10, ri32);
    float    rf32; NVRAM.get(14, rf32);
    bool     rb;   NVRAM.get(18, rb);

    bool all = true;
    all &= check("uint8_t ",  ru8  == u8);
    all &= check("int8_t  ",  ri8  == i8);
    all &= check("uint16_t",  ru16 == u16);
    all &= check("int16_t ",  ri16 == i16);
    all &= check("uint32_t",  ru32 == u32);
    all &= check("int32_t ",  ri32 == i32);
    all &= check("float   ",  rf32 == f32);
    all &= check("bool    ",  rb   == b);

    Serial.println();
    Serial.println(all ? F("All types: PASS") : F("Some types: FAIL"));

    // Also show the NVRAM dump for the used region
    Serial.println();
    NVRAM.dump(0, 20, Serial);
}

void loop() {}

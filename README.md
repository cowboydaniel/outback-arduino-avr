# Outback AVR Boards

Arduino board support package for Outback Electronics AVR boards.

## Boards

| Board | MCU | Clock | Notes |
|---|---|---|---|
| Multiduino | ATmega328P | 16 MHz | Original board |
| Multiduino v2 | ATmega328PB | 16 MHz | ATmega328PB, DS3231, SHT4x, SRAM, FRAM, EEPROM |

### Hardware differences: Multiduino v2 vs Multiduino

| Feature | Multiduino | Multiduino v2 |
|---|---|---|
| MCU | ATmega328P | ATmega328PB |
| Digital pins | D0–D19 | D0–D23 (PE0–PE3 extended) |
| RTC | DS1307 (I2C 0x68) | DS3231 temp-compensated (I2C 0x68) |
| RTC alarms | none | Alarm 1 + Alarm 2 (hardware, ±2 ppm) |
| RTC NVRAM | 56 bytes | none |
| Temp/humidity | none | SHT4x ±0.2°C / ±1.8% (I2C 0x44) |
| SPI SRAM | none | 256KB volatile (CS D9) |
| FRAM | none | 8KB non-volatile (I2C 0x57) |
| External EEPROM | none | 4KB + 64-bit unique ID (I2C 0x50/0x58) |
| SD card detect | none | D8 (active LOW) |
| RTC SQW/INT | not connected | D21/PE1 via JP4 |
| RTC 32KHz out | none | D20/PE0 via JP3 |
| SD bus enable | none | D22/PE2 via JP5 |
| 3.3V rail EN | none | D23/PE3 via JP6/JP7 |

> **Important:** On Multiduino v2 the 3.3V rail is off at power-up until the sketch drives PE3 HIGH (default JP6/JP7 configuration). Call `MultiduinoPower.begin()` before accessing any 3.3V peripheral (FRAM, SHT4x, SD card, EEPROM).

## Installation

Add the board manager URL to Arduino IDE preferences:

```
https://raw.githubusercontent.com/cowboydaniel/outback-arduino-avr/main/package_outback_index.json
```

Then install **Outback AVR Boards** from **Tools → Board → Boards Manager**.

## Libraries

All libraries live under `hardware/outback/avr/libraries/` and are automatically available when an Outback board is selected.

### MultiduinoRTC *(Multiduino only)*
DS1307 RTC driver. Provides `DateTime` read/write, oscillator control, SQW output, and 56-byte battery-backed NVRAM access.

```cpp
#include <MultiduinoRTC.h>
MultiduinoRTC.begin();
DateTime dt = MultiduinoRTC.now();
```

### MultiduinoNVRAM *(Multiduino only)*
High-level API for the DS1307's 56-byte NVRAM. Typed `put`/`get` templates, string/struct storage, CRC-8 integrity checking, hex dump.

```cpp
#include <MultiduinoNVRAM.h>
NVRAM.put(0, myStruct);
NVRAM.get(0, myStruct);
```

### MultiduinoSD *(both boards)*
Custom FAT16/FAT32 SD driver. No dependency on the Arduino SD library. Single shared sector cache, up to 4 simultaneous open files.

```cpp
#include <MultiduinoSD.h>
MultiduinoSD.begin(PIN_SD_DET);  // PIN_SD_DET available on v2
SDFile f = MultiduinoSD.open("log.txt", FILE_WRITE);
f.println("hello");
f.close();
```

---

### MultiduinoRTCv2 *(Multiduino v2 only)*
DS3231 temperature-compensated RTC driver. Hardware alarms with interrupt support on PE1 (JP4), on-chip temperature reading, SQW output, 32KHz output. Re-uses the `DateTime` struct from `MultiduinoRTC`.

```cpp
#include <MultiduinoRTCv2.h>
RTCv2.begin();
DateTime now = RTCv2.now();
float tempC  = RTCv2.temperature();

// Alarm interrupt (JP4 must be closed)
RTCv2.setAlarm1(DateTime(2000,1,1, 7,0,0), ALARM1_MATCH_HOURS_MINUTES_SECONDS);
RTCv2.onAlarm1(myCallback);
```

> The original Multiduino uses a DS1307 — use `MultiduinoRTC` on that board.

### MultiduinoSHT4x *(Multiduino v2 only)*
Sensirion SHT4x humidity (±1.8%) and temperature (±0.2°C) sensor driver (I2C 0x44). Three precision levels, CRC-8 validation, soft reset, serial number, and on-chip heater control.

```cpp
#include <MultiduinoSHT4x.h>
SHT4x.begin();
float tempC, humidity;
SHT4x.read(tempC, humidity);
```

### MultiduinoSRAM *(Multiduino v2 only)*
Driver for the 23AA02M 256KB SPI SRAM (CS D9). Byte, block, and typed access with sequential-mode streaming. No write delay.

```cpp
#include <MultiduinoSRAM.h>
SRAM.begin();
SRAM.write(0, 0x42);
SRAM.put(0, myLargeStruct);
```

### MultiduinoFRAM *(Multiduino v2 only)*
8KB ferroelectric RAM driver (I2C 0x57). Non-volatile, instant writes, ~100 trillion write cycles. Ideal for high-frequency logging or counter storage.

```cpp
#include <MultiduinoFRAM.h>
FRAM.begin();
FRAM.put(0, myConfig);   // survives power cycle, no write wear
```

### MultiduinoEEPROM *(Multiduino v2 only)*
4KB I2C EEPROM driver (0x50) with factory 64-bit unique serial number (0x58). Page-aligned writes with automatic write-cycle wait. ~1M write cycle endurance — use FRAM for frequent writes.

```cpp
#include <MultiduinoEEPROM.h>
ExtEEPROM.begin();
ExtEEPROM.printSerialNumber(Serial);  // e.g. A1:B2:C3:D4:E5:F6:07:08
ExtEEPROM.put(0, myConfig);
```

### MultiduinoPower *(Multiduino v2 only)*
Controls the on-board 3.3V power rail via PE3 (D23). **Must be called before accessing any 3.3V peripheral** (SHT4x, FRAM, EEPROM, SD card via slide switch).

```cpp
#include <MultiduinoPower.h>
MultiduinoPower.begin();          // enables 3.3V rail by default
MultiduinoPower.disable3V3();     // cut rail to save current
MultiduinoPower.enable3V3();      // restore
```

## I2C address map (Multiduino v2)

| Address | Device |
|---|---|
| 0x44 | SHT4x (temp/humidity) |
| 0x50 | External EEPROM (data) |
| 0x57 | FRAM |
| 0x58 | External EEPROM (serial number, read-only) |
| 0x68 | DS3231 RTC |

## Repository layout

```
hardware/outback/avr/
├── boards.txt
├── platform.txt
├── programmers.txt
├── bootloaders/optiboot/
├── cores/arduino/          (uses upstream arduino:arduino core)
├── variants/
│   ├── multiduino/         (ATmega328P, 20 pins)
│   └── multiduino_v2/      (ATmega328PB, 24 pins, PE0–PE3)
└── libraries/
    ├── MultiduinoRTC/      (DS1307 — Multiduino only)
    ├── MultiduinoNVRAM/    (DS1307 NVRAM — Multiduino only)
    ├── MultiduinoSD/       (both boards)
    ├── MultiduinoRTCv2/    (DS3231 — v2 only)
    ├── MultiduinoSHT4x/    (v2 only)
    ├── MultiduinoSRAM/     (v2 only)
    ├── MultiduinoFRAM/     (v2 only)
    ├── MultiduinoEEPROM/   (v2 only)
    └── MultiduinoPower/    (v2 only)
```

## Maintainer

Outback Electronics — [outbackhutelectronics@gmail.com](mailto:outbackhutelectronics@gmail.com)
[https://github.com/cowboydaniel/outback-arduino-avr](https://github.com/cowboydaniel/outback-arduino-avr)

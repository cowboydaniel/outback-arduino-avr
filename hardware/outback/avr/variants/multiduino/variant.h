#ifndef _VARIANT_MULTIDUINO_
#define _VARIANT_MULTIDUINO_

// Multiduino feature flags
// ------------------------
// Enable or disable features of the Multiduino board.
// These are evaluated at compile time.

// ---- UART ----
// The ATmega328P has one hardware UART (Serial).
#define HAVE_HWSERIAL0  1

// ---- SPI ----
#define HAVE_SPI        1

// ---- Onboard micro SD card ----
// Connected on the hardware SPI bus. CS is wired permanently to D10 (PB2 / SS).
// Consequence: D10 PWM (Timer1B) is unavailable; do not drive D10 as PWM output.
// Use the SD or SdFat library with SD_CS_PIN as the chip-select.
#define HAVE_SD_CARD    1
#define SD_CS_PIN       10      // PB2, hardware SS

// ---- I2C / TWI ----
#define HAVE_WIRE       1

// ---- Onboard DS1307 RTC + 56-byte NVRAM ----
// Communicates via I2C (SDA = A4/D18, SCL = A5/D19).
// I2C address: 0x68 (fixed, not configurable).
// The DS1307 also exposes 56 bytes of battery-backed NVRAM at registers 0x08–0x3F.
// SQW/OUT pin is not connected on this board.
// Recommended library: RTClib (Adafruit) or DS1307RTC.
#define HAVE_RTC_DS1307 1
#define HAVE_RTC_NVRAM  1
#define RTC_NVRAM_SIZE  56      // bytes, registers 0x08-0x3F
#define RTC_I2C_ADDR    0x68

// ---- Timers ----
// Timer0: 8-bit, used by millis()/micros()
// Timer1: 16-bit, used by Servo / PWM on pins 9, 10
// Timer2: 8-bit, used by Tone / PWM on pins 3, 11
#define HAVE_TIMER0     1
#define HAVE_TIMER1     1
#define HAVE_TIMER2     1

// ---- ADC ----
#define HAVE_ADC        1
#define ADC_RESOLUTION  10  // bits

// ---- EEPROM ----
#define HAVE_EEPROM     1
#define EEPROM_SIZE     1024  // bytes

// ---- USB ----
// The ATmega328P does not have native USB.
// Set to 0 to disable USB-related code paths.
#define HAVE_USB        0

// ---- Bootloader ----
// Optiboot is burned at 0x7E00 (512-word section, 1 kB).
// Sketch flash starts at 0x0000.
#define BOOTLOADER_SIZE 512   // words

#endif /* _VARIANT_MULTIDUINO_ */

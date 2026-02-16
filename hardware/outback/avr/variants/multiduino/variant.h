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

// ---- I2C / TWI ----
#define HAVE_WIRE       1

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

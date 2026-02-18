/*
 * pins_arduino.h — Multiduino v2 pin mapping
 * Outback Electronics
 *
 * ATmega328PB on Multiduino v2.
 * Pin assignments verified against Multiduino.net (KiCad netlist).
 *
 * Standard Arduino Uno-compatible pins (D0–D13, A0–A5) are identical
 * to the Uno R3. Extended pins D20–D23 map to PE0–PE3.
 *
 * On-board peripherals (verified from netlist):
 *   SRAM CS  (U5, 23AA02M)   — D9  / PB1  — net "CS"
 *   SD CS    (U5→U7 buffer)  — D10 / PB2  — net "SS"
 *   SD DET   (J5 pin 9)      — D8  / PB0  — net "IO8"
 *   SPI SCK                  — D13 / PB5  — net "SCK"
 *   SPI MOSI                 — D11 / PB3  — net "MOSI"
 *   SPI MISO                 — D12 / PB4  — net "MISO"
 *   I2C SDA                  — A4  / PC4  — net "A4/SDA"
 *   I2C SCL                  — A5  / PC5  — net "A5/SCL"
 *   PE0 / RTC 32KHz (JP3)   — D20 / PE0
 *   PE1 / RTC INT/SQW (JP4) — D21 / PE1
 *   PE2 / SD enable (JP5)   — D22 / PE2
 *   PE3 / 3.3V rail EN (JP6)— D23 / PE3
 */

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

#define NUM_DIGITAL_PINS            24
#define NUM_ANALOG_INPUTS           6
#define NUM_ANALOG_INPUTS_EXTENDED  6

#define analogInputToDigitalPin(p)  ((p) < 6 ? (p) + 14 : -1)

#define digitalPinHasPWM(p)         ((p) == 3 || (p) == 5 || (p) == 6 || \
                                     (p) == 9 || (p) == 10 || (p) == 11)

// SPI
static const uint8_t SS   = 10;  // PB2 — SD card CS (net "SS")
static const uint8_t MOSI = 11;  // PB3
static const uint8_t MISO = 12;  // PB4
static const uint8_t SCK  = 13;  // PB5

// I2C
static const uint8_t SDA = 18;   // PC4 / A4
static const uint8_t SCL = 19;   // PC5 / A5

// Analog pins (also usable as digital)
static const uint8_t A0 = 14;
static const uint8_t A1 = 15;
static const uint8_t A2 = 16;
static const uint8_t A3 = 17;
static const uint8_t A4 = 18;
static const uint8_t A5 = 19;

// On-board peripheral pin aliases
static const uint8_t PIN_SPI_SS_SD   = 10;  // PB2 — SD card chip select (net "SS")
static const uint8_t PIN_SPI_SS_SRAM =  9;  // PB1 — SRAM chip select (net "CS")
static const uint8_t PIN_SD_DET      =  8;  // PB0 — SD card detect (active LOW, net "IO8")
static const uint8_t PIN_3V3_EN      = 23;  // PE3 — 3.3V rail enable (HIGH = on, via JP6)

// Extended PE pins
static const uint8_t PIN_PE0 = 20;  // PE0 — RTC 32KHz output when JP3 closed
static const uint8_t PIN_PE1 = 21;  // PE1 — RTC SQW/INT when JP4 closed
static const uint8_t PIN_PE2 = 22;  // PE2 — SD enable / slide switch state when JP5 closed
static const uint8_t PIN_PE3 = 23;  // PE3 — 3.3V rail enable (JP6 default A–C position)

#define digitalPinToPCICR(p)    ( ((p) >= 0  && (p) <= 13) ? (&PCICR) : \
                                  ((p) >= 14 && (p) <= 19) ? (&PCICR) : \
                                  NULL )

#define digitalPinToPCICRbit(p) ( ((p) >= 0  && (p) <=  7) ? 2 :  /* PCIE2 = PORTD */ \
                                  ((p) >= 8  && (p) <= 13) ? 0 :  /* PCIE0 = PORTB */ \
                                  ((p) >= 14 && (p) <= 19) ? 1 :  /* PCIE1 = PORTC */ \
                                  0 )

#define digitalPinToPCMSK(p)    ( ((p) >= 0  && (p) <=  7) ? (&PCMSK2) : \
                                  ((p) >= 8  && (p) <= 13) ? (&PCMSK0) : \
                                  ((p) >= 14 && (p) <= 19) ? (&PCMSK1) : \
                                  NULL )

#define digitalPinToPCMSKbit(p) ( ((p) >= 0  && (p) <=  7) ? (p)      : \
                                  ((p) >= 8  && (p) <= 13) ? (p) - 8  : \
                                  ((p) >= 14 && (p) <= 19) ? (p) - 14 : \
                                  0 )

// External interrupt pins: INT0=D2 (PD2), INT1=D3 (PD3)
#define digitalPinToInterrupt(p)  ((p) == 2 ? 0 : ((p) == 3 ? 1 : NOT_AN_INTERRUPT))

#ifdef ARDUINO_MAIN

// Port/pin lookup tables
// Order matches digital pin numbers D0–D23

const uint16_t PROGMEM port_to_mode_PGM[] = {
    NOT_A_PORT,         // 0 — placeholder
    NOT_A_PORT,         // PA — not present on ATmega328PB
    (uint16_t) &DDRB,
    (uint16_t) &DDRC,
    (uint16_t) &DDRD,
    (uint16_t) &DDRE,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
    NOT_A_PORT,
    NOT_A_PORT,
    (uint16_t) &PORTB,
    (uint16_t) &PORTC,
    (uint16_t) &PORTD,
    (uint16_t) &PORTE,
};

const uint16_t PROGMEM port_to_input_PGM[] = {
    NOT_A_PORT,
    NOT_A_PORT,
    (uint16_t) &PINB,
    (uint16_t) &PINC,
    (uint16_t) &PIND,
    (uint16_t) &PINE,
};

// Maps Arduino pin number → port
// PB=2, PC=3, PD=4, PE=5 (matches port_to_*_PGM above)
const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
    PD, // D0  — PD0 — IO0 / RX
    PD, // D1  — PD1 — IO1 / TX
    PD, // D2  — PD2 — IO2
    PD, // D3  — PD3 — IO3  (OC2B, PWM)
    PD, // D4  — PD4 — IO4
    PD, // D5  — PD5 — IO5  (OC0B, PWM)
    PD, // D6  — PD6 — IO6  (OC0A, PWM)
    PD, // D7  — PD7 — IO7
    PB, // D8  — PB0 — SD card detect (net IO8)
    PB, // D9  — PB1 — SRAM CS (net CS)    (OC1A, PWM)
    PB, // D10 — PB2 — SD CS (net SS)      (OC1B, PWM)
    PB, // D11 — PB3 — MOSI                (OC2A, PWM)
    PB, // D12 — PB4 — MISO
    PB, // D13 — PB5 — SCK / LED
    PC, // D14 — PC0 — A0
    PC, // D15 — PC1 — A1
    PC, // D16 — PC2 — A2
    PC, // D17 — PC3 — A3
    PC, // D18 — PC4 — A4 / SDA
    PC, // D19 — PC5 — A5 / SCL
    PE, // D20 — PE0 — extended (JP3: RTC 32KHz)
    PE, // D21 — PE1 — extended (JP4: RTC SQW/INT)
    PE, // D22 — PE2 — extended (JP5: SD enable / SW1)
    PE, // D23 — PE3 — 3.3V rail EN (JP6 default)
};

// Maps Arduino pin number → bit mask within port
const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
    _BV(0), // D0  — PD0
    _BV(1), // D1  — PD1
    _BV(2), // D2  — PD2
    _BV(3), // D3  — PD3
    _BV(4), // D4  — PD4
    _BV(5), // D5  — PD5
    _BV(6), // D6  — PD6
    _BV(7), // D7  — PD7
    _BV(0), // D8  — PB0
    _BV(1), // D9  — PB1
    _BV(2), // D10 — PB2
    _BV(3), // D11 — PB3
    _BV(4), // D12 — PB4
    _BV(5), // D13 — PB5
    _BV(0), // D14 — PC0
    _BV(1), // D15 — PC1
    _BV(2), // D16 — PC2
    _BV(3), // D17 — PC3
    _BV(4), // D18 — PC4
    _BV(5), // D19 — PC5
    _BV(0), // D20 — PE0
    _BV(1), // D21 — PE1
    _BV(2), // D22 — PE2
    _BV(3), // D23 — PE3
};

// Maps Arduino pin number → timer (for analogWrite / PWM)
// Only the six standard Uno PWM pins are active. PE0/PE1 have OC4A/OC4B
// on ATmega328PB but are listed NOT_ON_TIMER until Timer 4 support is added.
const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
    NOT_ON_TIMER, // D0
    NOT_ON_TIMER, // D1
    NOT_ON_TIMER, // D2
    TIMER2B,      // D3  — OC2B
    NOT_ON_TIMER, // D4
    TIMER0B,      // D5  — OC0B
    TIMER0A,      // D6  — OC0A
    NOT_ON_TIMER, // D7
    NOT_ON_TIMER, // D8
    TIMER1A,      // D9  — OC1A
    TIMER1B,      // D10 — OC1B
    TIMER2A,      // D11 — OC2A
    NOT_ON_TIMER, // D12
    NOT_ON_TIMER, // D13
    NOT_ON_TIMER, // D14
    NOT_ON_TIMER, // D15
    NOT_ON_TIMER, // D16
    NOT_ON_TIMER, // D17
    NOT_ON_TIMER, // D18
    NOT_ON_TIMER, // D19
    NOT_ON_TIMER, // D20 — PE0 (OC4A on ATmega328PB, unsupported in standard core)
    NOT_ON_TIMER, // D21 — PE1 (OC4B on ATmega328PB, unsupported in standard core)
    NOT_ON_TIMER, // D22 — PE2
    NOT_ON_TIMER, // D23 — PE3
};

#endif // ARDUINO_MAIN

#endif // Pins_Arduino_h

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <avr/pgmspace.h>

// Multiduino - ATmega328P, 16MHz
//
// Onboard peripherals and their pin reservations:
//   micro SD card : SPI bus (D10 CS, D11 MOSI, D12 MISO, D13 SCK)
//                   D10 is permanently driven as CS — PWM on D10 (Timer1B) is unavailable.
//   DS1307 RTC    : I2C bus (A4/D18 SDA, A5/D19 SCL), address 0x68
//                   Includes 56 bytes of battery-backed NVRAM (reg 0x08–0x3F).
//                   SQW/INT output is not connected.

#define NUM_DIGITAL_PINS            20
#define NUM_ANALOG_INPUTS           6
#define analogInputToDigitalPin(p)  ((p < 6) ? (p) + 14 : -1)

// D10 (Timer1B) is reserved as SD card CS and must not be used as PWM output.
#define digitalPinHasPWM(p)         ((p) == 3 || (p) == 5 || (p) == 6 || (p) == 9 || (p) == 11)

#define PIN_SPI_SS    (10)
#define PIN_SPI_MOSI  (11)
#define PIN_SPI_MISO  (12)
#define PIN_SPI_SCK   (13)

static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

#define PIN_WIRE_SDA        (18)
#define PIN_WIRE_SCL        (19)

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

// Onboard micro SD card (SPI)
// MOSI/MISO/SCK share the hardware SPI bus with any other SPI devices.
// CS is wired to D10; PWM (Timer1B) on D10 is therefore unavailable.
#define PIN_SD_CS     (10)
static const uint8_t SD_CS = PIN_SD_CS;

// Onboard DS1307 RTC / NVRAM (I2C, address 0x68)
// A4 and A5 are dedicated to I2C on this board; avoid using them as GPIO.
#define PIN_RTC_SDA   PIN_WIRE_SDA   // A4 / D18
#define PIN_RTC_SCL   PIN_WIRE_SCL   // A5 / D19

#define LED_BUILTIN 13

#define PIN_A0   (14)
#define PIN_A1   (15)
#define PIN_A2   (16)
#define PIN_A3   (17)
#define PIN_A4   (18)
#define PIN_A5   (19)

static const uint8_t A0 = PIN_A0;
static const uint8_t A1 = PIN_A1;
static const uint8_t A2 = PIN_A2;
static const uint8_t A3 = PIN_A3;
static const uint8_t A4 = PIN_A4;
static const uint8_t A5 = PIN_A5;

#define digitalPinToPCICR(p)    (((p) >= 0 && (p) <= 21) ? (&PCICR) : ((uint8_t *)0))
#define digitalPinToPCICRbit(p) (((p) <= 7) ? 2 : (((p) <= 13) ? 0 : 1))
#define digitalPinToPCMSK(p)    (((p) <= 7) ? (&PCMSK2) : (((p) <= 13) ? (&PCMSK0) : (((p) <= 21) ? (&PCMSK1) : ((uint8_t *)0))))
#define digitalPinToPCMSKbit(p) (((p) <= 7) ? (p) : (((p) <= 13) ? ((p) - 8) : ((p) - 14)))

#define digitalPinToInterrupt(p)  ((p) == 2 ? 0 : ((p) == 3 ? 1 : NOT_AN_INTERRUPT))

#ifdef ARDUINO_MAIN

// On the Arduino board, digital pins are also used
// for the analog output (PWM).  Digital pins 3, 5, 6, 9, 10, 11 are PWM.
//
// ATMEL ATMEGA328P / ARDUINO
//
//                  +-\/-+
//            PC6  1|    |28  PC5 (AI 5)
//      (D 0) PD0  2|    |27  PC4 (AI 4)
//      (D 1) PD1  3|    |26  PC3 (AI 3)
//      (D 2) PD2  4|    |25  PC2 (AI 2)
// PWM+ (D 3) PD3  5|    |24  PC1 (AI 1)
//      (D 4) PD4  6|    |23  PC0 (AI 0)
//            VCC  7|    |22  GND
//            GND  8|    |21  AREF
//            PB6  9|    |20  AVCC
//            PB7 10|    |19  PB5 (D 13)
// PWM+ (D 5) PD5 11|    |18  PB4 (D 12)
// PWM+ (D 6) PD6 12|    |17  PB3 (D 11) PWM
//      (D 7) PD7 13|    |16  PB2 (D 10) PWM
//      (D 8) PB0 14|    |15  PB1 (D  9) PWM
//                  +----+

// these arrays map port names (e.g. port B) to the
// appropriate addresses for various functions (e.g. reading
// and writing)
const uint16_t PROGMEM port_to_mode_PGM[] = {
    NOT_A_PORT,
    NOT_A_PORT,
    (uint16_t) &DDRB,
    (uint16_t) &DDRC,
    (uint16_t) &DDRD,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
    NOT_A_PORT,
    NOT_A_PORT,
    (uint16_t) &PORTB,
    (uint16_t) &PORTC,
    (uint16_t) &PORTD,
};

const uint16_t PROGMEM port_to_input_PGM[] = {
    NOT_A_PORT,
    NOT_A_PORT,
    (uint16_t) &PINB,
    (uint16_t) &PINC,
    (uint16_t) &PIND,
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
    PD, /* 0 */
    PD,
    PD,
    PD,
    PD,
    PD,
    PD,
    PD,
    PB, /* 8 */
    PB,
    PB,
    PB,
    PB,
    PB,
    PC, /* 14 */
    PC,
    PC,
    PC,
    PC,
    PC,
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
    _BV(0), /* 0, port D */
    _BV(1),
    _BV(2),
    _BV(3),
    _BV(4),
    _BV(5),
    _BV(6),
    _BV(7),
    _BV(0), /* 8, port B */
    _BV(1),
    _BV(2),
    _BV(3),
    _BV(4),
    _BV(5),
    _BV(0), /* 14, port C */
    _BV(1),
    _BV(2),
    _BV(3),
    _BV(4),
    _BV(5),
};

const uint8_t PROGMEM digital_pin_to_timer_PGM[] = {
    NOT_ON_TIMER, /* 0 - port D */
    NOT_ON_TIMER,
    NOT_ON_TIMER,
    TIMER2B,      /* 3 */
    NOT_ON_TIMER,
    TIMER0B,      /* 5 */
    TIMER0A,      /* 6 */
    NOT_ON_TIMER,
    NOT_ON_TIMER, /* 8 - port B */
    TIMER1A,      /* 9 */
    NOT_ON_TIMER, /* 10 - SD card CS, Timer1B unavailable */
    TIMER2A,      /* 11 */
    NOT_ON_TIMER,
    NOT_ON_TIMER,
    NOT_ON_TIMER, /* 14 - port C */
    NOT_ON_TIMER,
    NOT_ON_TIMER,
    NOT_ON_TIMER,
    NOT_ON_TIMER,
    NOT_ON_TIMER,
};

#endif /* ARDUINO_MAIN */

#endif /* Pins_Arduino_h */

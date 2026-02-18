Place bootloader hex files here. Two files are needed:

  optiboot_atmega328.hex   — Multiduino (ATmega328P)
  optiboot_atmega328pb.hex — Multiduino v2 (ATmega328PB)

These files are only needed if you use Tools → Burn Bootloader.
Sketch upload (via existing bootloader) works without them.

──────────────────────────────────────────────────────────────
optiboot_atmega328.hex  (ships with Arduino AVR Boards)
──────────────────────────────────────────────────────────────
Copy from the Arduino AVR Boards package:

  Arduino IDE 2.x / arduino-cli:
    Windows : %LOCALAPPDATA%\Arduino15\packages\arduino\hardware\avr\<version>\bootloaders\optiboot\optiboot_atmega328.hex
    macOS   : ~/Library/Arduino15/packages/arduino/hardware/avr/<version>/bootloaders/optiboot/optiboot_atmega328.hex
    Linux   : ~/.arduino15/packages/arduino/hardware/avr/<version>/bootloaders/optiboot/optiboot_atmega328.hex

  Arduino IDE 1.x:
    Windows : C:\Program Files (x86)\Arduino\hardware\arduino\avr\bootloaders\optiboot\optiboot_atmega328.hex
    macOS   : /Applications/Arduino.app/Contents/Java/hardware/arduino/avr/bootloaders/optiboot/optiboot_atmega328.hex
    Linux   : /usr/share/arduino/hardware/arduino/avr/bootloaders/optiboot/optiboot_atmega328.hex

──────────────────────────────────────────────────────────────
optiboot_atmega328pb.hex  (not in Arduino AVR Boards)
──────────────────────────────────────────────────────────────
The ATmega328PB is not included in the official Arduino AVR Boards
optiboot build. Get a pre-built hex from MiniCore:

  https://github.com/MCUdude/MiniCore

The file is located at:
  MiniCore/avr/bootloaders/optiboot_flash/atmega328pb/16000000L/115200/optiboot_flash_atmega328pb_UART0_115200_16000000_B5.hex

Rename it to optiboot_atmega328pb.hex and place it here.

Alternatively, build optiboot from source with MCU=atmega328pb:
  https://github.com/Optiboot/optiboot

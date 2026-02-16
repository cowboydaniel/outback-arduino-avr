Place optiboot_atmega328.hex here.

The file ships with the Arduino AVR Boards package. Copy it from one of these
locations (depending on your OS and IDE version) to this directory:

Arduino IDE 1.x:
  Windows : C:\Program Files (x86)\Arduino\hardware\arduino\avr\bootloaders\optiboot\optiboot_atmega328.hex
  macOS   : /Applications/Arduino.app/Contents/Java/hardware/arduino/avr/bootloaders/optiboot/optiboot_atmega328.hex
  Linux   : /usr/share/arduino/hardware/arduino/avr/bootloaders/optiboot/optiboot_atmega328.hex

Arduino IDE 2.x / arduino-cli:
  Windows : %LOCALAPPDATA%\Arduino15\packages\arduino\hardware\avr\<version>\bootloaders\optiboot\optiboot_atmega328.hex
  macOS   : ~/Library/Arduino15/packages/arduino/hardware/avr/<version>/bootloaders/optiboot/optiboot_atmega328.hex
  Linux   : ~/.arduino15/packages/arduino/hardware/avr/<version>/bootloaders/optiboot/optiboot_atmega328.hex

This file is only needed if you use Tools → Burn Bootloader.
Sketch upload (via existing bootloader) works without it.

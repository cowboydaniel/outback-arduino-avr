#pragma once
#include <Arduino.h>
#include <SD.h>

// The onboard micro SD card CS is hardwired to D10.
#define MULTIDUINO_SD_CS  10

// ---------------------------------------------------------------------------
// MultiduinoSD
//
// Wraps the Arduino SD library pre-configured for the Multiduino's onboard
// micro SD card (CS = D10, hardware SPI). Pass File objects, paths, and
// open-mode flags exactly as you would with the standard SD library.
// ---------------------------------------------------------------------------
class MultiduinoSDClass {
public:
    // Initialize the SD card. Returns true on success.
    // Optionally supply a different SPI speed (default = SPI_HALF_SPEED).
    bool begin(uint32_t spiSpeed = SPI_HALF_SPEED);

    // True if a card is initialised and ready.
    bool ready() const { return _ready; }

    // Print card type and size info to a Print target (e.g. Serial).
    void printCardInfo(Print& out);

    // ---- File operations (thin pass-through to SD.h) ----------------------

    // Open a file or directory. mode: FILE_READ or FILE_WRITE.
    File open(const char* path, uint8_t mode = FILE_READ);

    // True if the path exists (file or directory).
    bool exists(const char* path);

    // Remove a file. Returns true on success.
    bool remove(const char* path);

    // Create a directory (and all missing parents). Returns true on success.
    bool mkdir(const char* path);

    // Remove an empty directory. Returns true on success.
    bool rmdir(const char* path);

    // List the contents of a directory to a Print target.
    // depth controls indentation of nested entries.
    void ls(const char* path, Print& out, uint8_t depth = 0);

private:
    bool _ready = false;

    void _lsDir(File dir, Print& out, uint8_t depth);
};

extern MultiduinoSDClass MultiduinoSD;

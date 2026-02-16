#include "MultiduinoSD.h"

bool MultiduinoSDClass::begin(uint32_t spiSpeed) {
    _ready = SD.begin(MULTIDUINO_SD_CS, spiSpeed);
    return _ready;
}

void MultiduinoSDClass::printCardInfo(Print& out) {
    if (!_ready) {
        out.println(F("SD: not initialised"));
        return;
    }

    Sd2Card card;
    SdVolume volume;
    SdFile root;

    if (!card.init(SPI_HALF_SPEED, MULTIDUINO_SD_CS)) {
        out.println(F("SD: card.init failed"));
        return;
    }

    out.print(F("Card type: "));
    switch (card.type()) {
        case SD_CARD_TYPE_SD1:  out.println(F("SD1"));  break;
        case SD_CARD_TYPE_SD2:  out.println(F("SD2"));  break;
        case SD_CARD_TYPE_SDHC: out.println(F("SDHC")); break;
        default:                out.println(F("Unknown")); break;
    }

    if (volume.init(card)) {
        uint32_t clusters   = volume.clusterCount();
        uint32_t blocksPerCluster = volume.blocksPerCluster();
        uint32_t totalBlocks = clusters * blocksPerCluster;
        uint32_t totalMB     = totalBlocks / 2 / 1024;

        out.print(F("FAT type : FAT"));
        out.println(volume.fatType());
        out.print(F("Capacity : "));
        out.print(totalMB);
        out.println(F(" MB"));
    }
}

File MultiduinoSDClass::open(const char* path, uint8_t mode) {
    return SD.open(path, mode);
}

bool MultiduinoSDClass::exists(const char* path) {
    return SD.exists(path);
}

bool MultiduinoSDClass::remove(const char* path) {
    return SD.remove(path);
}

bool MultiduinoSDClass::mkdir(const char* path) {
    return SD.mkdir(path);
}

bool MultiduinoSDClass::rmdir(const char* path) {
    return SD.rmdir(path);
}

void MultiduinoSDClass::ls(const char* path, Print& out, uint8_t depth) {
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        out.print(F("ls: cannot open "));
        out.println(path);
        if (dir) dir.close();
        return;
    }
    _lsDir(dir, out, depth);
    dir.close();
}

void MultiduinoSDClass::_lsDir(File dir, Print& out, uint8_t depth) {
    while (true) {
        File entry = dir.openNextFile();
        if (!entry) break;

        for (uint8_t i = 0; i < depth; i++) out.print(F("  "));
        out.print(entry.name());

        if (entry.isDirectory()) {
            out.println(F("/"));
            _lsDir(entry, out, depth + 1);
        } else {
            out.print(F("  "));
            out.print(entry.size());
            out.println(F(" B"));
        }
        entry.close();
    }
}

// Global instance
MultiduinoSDClass MultiduinoSD;

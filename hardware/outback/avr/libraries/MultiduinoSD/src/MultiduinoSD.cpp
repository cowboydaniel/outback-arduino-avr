/*
  MultiduinoSD.cpp  –  Full custom FAT16/FAT32 SD card library
  =============================================================
  Implements the SD specification (Physical Layer Simplified Spec v8.00)
  SPI mode plus a FAT16/FAT32 filesystem layer.

  No dependency on the Arduino SD library.  Only Arduino SPI.h is used
  for hardware SPI register access.

  Sector cache strategy
  ─────────────────────
  A single 512-byte buffer (_cacheBuf) holds the most recently accessed
  sector.  On every read the cache is checked; on a cache miss the current
  sector is flushed (if dirty) and the new sector is loaded.  This keeps
  RAM usage predictable on the ATmega328P.

  SD SPI token / response reference
  ──────────────────────────────────
  R1  : 1 byte  (bit 0 = in-idle, bits 1-6 = error flags)
  R3/R7: R1 + 4 bytes
  Data token (read)  : 0xFE
  Data token (write) : 0xFE  (start of data)
  Write response     : 0x05 (accepted), 0x0B (CRC error), 0x0D (write error)
*/

#include "MultiduinoSD.h"
#include <string.h>
#include <ctype.h>

// ---------------------------------------------------------------------------
// Helpers: little-endian 16/32 byte reads from a buffer
// ---------------------------------------------------------------------------
static inline uint16_t rd16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t rd32(const uint8_t* p) {
    return (uint32_t)p[0]        | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void wr32(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF;  p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;  p[3] = (v >> 24) & 0xFF;
}
static inline void wr16(uint8_t* p, uint16_t v) {
    p[0] = v & 0xFF;  p[1] = (v >> 8) & 0xFF;
}

// ---------------------------------------------------------------------------
// SPI helpers (CS active-low)
// ---------------------------------------------------------------------------
static void csLow()  { digitalWrite(SD_CS_PIN, LOW);  }
static void csHigh() { digitalWrite(SD_CS_PIN, HIGH); }

static uint8_t spiXfer(uint8_t b) { return SPI.transfer(b); }

// ---------------------------------------------------------------------------
// SDFile constructor
// ---------------------------------------------------------------------------
SDFile::SDFile() {
    _open = false; _isDir = false; _dirty = false;
    _name[0] = '\0';
    _startCluster = _dirSector = _size = _pos = _curCluster = 0;
    _dirOffset = _clusterOffset = 0;
    _dirCluster = _dirIterSector = 0; _dirIterOffset = 0;
}

// SDFile I/O delegates to the singleton MultiduinoSD
extern MultiduinoSDClass MultiduinoSD;

int SDFile::read(void* buf, uint16_t count) {
    if (!_open || _isDir) return 0;
    return (int)MultiduinoSD._fileRead(this, (uint8_t*)buf, count);
}
int SDFile::read() {
    uint8_t b; return (read(&b, 1) == 1) ? b : -1;
}
uint16_t SDFile::write(const void* buf, uint16_t count) {
    if (!_open || _isDir) return 0;
    return MultiduinoSD._fileWrite(this, (const uint8_t*)buf, count);
}
uint8_t SDFile::write(uint8_t b)  { return (write(&b, 1) == 1) ? 1 : 0; }
void    SDFile::flush()            { if (_open && !_isDir) MultiduinoSD._fileFlush(this); }
bool    SDFile::seek(uint32_t pos) { return (_open && !_isDir) ? MultiduinoSD._fileSeek(this, pos) : false; }
SDFile  SDFile::openNextFile(uint8_t flags) {
    return (_open && _isDir) ? MultiduinoSD._dirNext(this, flags) : SDFile();
}
void SDFile::rewindDirectory() {
    if (_open && _isDir) { _dirIterSector = 0; _dirIterOffset = 0; }
}
void SDFile::close() { if (_open) { flush(); _open = false; } }

// ---------------------------------------------------------------------------
// SD protocol
// ---------------------------------------------------------------------------

// CRC7 for command packets (polynomial 0x09)
static uint8_t crc7(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc <<= 1;
            if ((b ^ crc) & 0x80) crc ^= 0x09;
            b <<= 1;
        }
    }
    return (crc << 1) | 1;
}

uint8_t MultiduinoSDClass::sdWaitReady(uint16_t timeoutMs) {
    uint32_t deadline = millis() + timeoutMs;
    uint8_t r;
    do { r = spiXfer(0xFF); } while (r != 0xFF && (int32_t)(millis() - deadline) < 0);
    return r;
}

uint8_t MultiduinoSDClass::sdCmd(uint8_t cmd, uint32_t arg) {
    sdWaitReady(300);
    uint8_t pkt[6] = {
        (uint8_t)(0x40 | cmd),
        (uint8_t)(arg >> 24), (uint8_t)(arg >> 16),
        (uint8_t)(arg >> 8),  (uint8_t)(arg),
        0x01
    };
    pkt[5] = crc7(pkt, 5);
    for (uint8_t i = 0; i < 6; i++) spiXfer(pkt[i]);
    uint8_t r = 0xFF;
    for (uint8_t i = 0; i < 8; i++) { r = spiXfer(0xFF); if (!(r & 0x80)) break; }
    return r;
}

uint8_t MultiduinoSDClass::sdACmd(uint8_t cmd, uint32_t arg) {
    csLow(); sdCmd(55, 0); csHigh(); spiXfer(0xFF);
    csLow(); return sdCmd(cmd, arg);
}

bool MultiduinoSDClass::sdReadBlock(uint32_t lba, uint8_t* buf) {
    uint32_t addr = _isSDHC ? lba : lba * 512UL;
    csLow();
    if (sdCmd(17, addr) != 0x00) { csHigh(); return false; }
    uint8_t tok = 0xFF;
    uint32_t dl = millis() + 500;
    while (tok != 0xFE && (int32_t)(millis() - dl) < 0) tok = spiXfer(0xFF);
    if (tok != 0xFE) { csHigh(); return false; }
    for (uint16_t i = 0; i < 512; i++) buf[i] = spiXfer(0xFF);
    spiXfer(0xFF); spiXfer(0xFF);  // CRC (ignored)
    csHigh(); spiXfer(0xFF);
    return true;
}

bool MultiduinoSDClass::sdWriteBlock(uint32_t lba, const uint8_t* buf) {
    uint32_t addr = _isSDHC ? lba : lba * 512UL;
    csLow();
    if (sdCmd(24, addr) != 0x00) { csHigh(); return false; }
    spiXfer(0xFF); spiXfer(0xFE);
    for (uint16_t i = 0; i < 512; i++) spiXfer(buf[i]);
    spiXfer(0xFF); spiXfer(0xFF);
    uint8_t resp = spiXfer(0xFF) & 0x1F;
    if (resp != 0x05) { csHigh(); return false; }
    uint32_t dl = millis() + 500;
    while (spiXfer(0xFF) == 0x00 && (int32_t)(millis() - dl) < 0) {}
    csHigh(); spiXfer(0xFF);
    return true;
}

bool MultiduinoSDClass::sdInit() {
    pinMode(SD_CS_PIN, OUTPUT); csHigh();
    SPI.begin();
    SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));
    for (uint8_t i = 0; i < 10; i++) spiXfer(0xFF);  // ≥74 clocks

    csLow();
    uint8_t r = sdCmd(0, 0);
    csHigh();
    if (r != 0x01) return false;

    csLow();
    r = sdCmd(8, 0x000001AAUL);
    if (r == 0x01) {
        uint8_t r7[4];
        for (uint8_t i = 0; i < 4; i++) r7[i] = spiXfer(0xFF);
        csHigh();
        if (r7[3] != 0xAA || (r7[2] & 0x0F) != 0x01) return false;
        _cardVer = 2;
    } else { csHigh(); _cardVer = 1; }

    uint32_t hcs = (_cardVer == 2) ? 0x40000000UL : 0;
    uint32_t dl  = millis() + 2000;
    do {
        csLow(); r = sdACmd(41, hcs); csHigh(); spiXfer(0xFF);
    } while (r == 0x01 && (int32_t)(millis() - dl) < 0);
    if (r != 0x00) return false;

    if (_cardVer == 2) {
        csLow(); r = sdCmd(58, 0);
        uint8_t ocr[4]; for (uint8_t i = 0; i < 4; i++) ocr[i] = spiXfer(0xFF);
        csHigh();
        if (r == 0x00 && (ocr[0] & 0x40)) _isSDHC = true;
    }

    SPI.endTransaction();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    return true;
}

// ---------------------------------------------------------------------------
// Sector cache
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::cacheSector(uint32_t lba) {
    if (_cacheLba == lba) return true;
    if (!flushCache()) return false;
    if (!sdReadBlock(lba, _cacheBuf)) return false;
    _cacheLba = lba; _cacheDirty = false;
    return true;
}
bool MultiduinoSDClass::flushCache() {
    if (!_cacheDirty) return true;
    if (!sdWriteBlock(_cacheLba, _cacheBuf)) return false;
    _cacheDirty = false; return true;
}

// ---------------------------------------------------------------------------
// FAT mount
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::fatMount() {
    if (!sdReadBlock(0, _cacheBuf)) return false;
    _cacheLba = 0; _cacheDirty = false;

    uint32_t bpbLba = 0;
    // Check if sector 0 is an MBR (not a boot sector)
    if (_cacheBuf[510] == 0x55 && _cacheBuf[511] == 0xAA &&
        _cacheBuf[0] != 0xEB && _cacheBuf[0] != 0xE9) {
        bpbLba = rd32(_cacheBuf + 446 + 8);
    }

    if (!cacheSector(bpbLba)) return false;
    uint8_t* bpb = _cacheBuf;

    _bytesPerSec = rd16(bpb + 11);
    _secPerClus  = bpb[13];
    _resSectors  = rd16(bpb + 14);
    _numFATs     = bpb[16];
    _rootEntCnt  = rd16(bpb + 17);

    uint16_t fat16Sz = rd16(bpb + 22);
    _FATSz = fat16Sz ? fat16Sz : rd32(bpb + 36);

    uint16_t totSec16 = rd16(bpb + 19);
    _totalSec32 = totSec16 ? totSec16 : rd32(bpb + 32);

    _partStart = bpbLba;
    _fat1Lba   = bpbLba + _resSectors;

    uint32_t rootDirSectors = (((uint32_t)_rootEntCnt * 32) + 511) / 512;
    _rootLba  = _fat1Lba + (uint32_t)_numFATs * _FATSz;
    _dataLba  = _rootLba + rootDirSectors;

    uint32_t dataSec = _totalSec32 -
        (uint32_t)(bpbLba + _resSectors + (uint32_t)_numFATs * _FATSz + rootDirSectors);
    _clusterCnt = dataSec / _secPerClus;

    if      (_clusterCnt < 4085)  return false;  // FAT12 not supported
    else if (_clusterCnt < 65525) _fatType = 16;
    else {
        _fatType   = 32;
        _rootClus  = rd32(bpb + 44);
        _rootLba   = fatClusterToLba(_rootClus);
    }
    return (_secPerClus > 0 && _bytesPerSec == 512);
}

// ---------------------------------------------------------------------------
// FAT cluster helpers
// ---------------------------------------------------------------------------
uint32_t MultiduinoSDClass::fatClusterToLba(uint32_t cluster) const {
    return _dataLba + (uint32_t)(cluster - 2) * _secPerClus;
}

uint32_t MultiduinoSDClass::fatNextCluster(uint32_t cluster) {
    uint32_t fatOffset, fatSector; uint16_t entryOffset;
    if (_fatType == 16) {
        fatOffset   = cluster * 2;
        fatSector   = _fat1Lba + fatOffset / 512;
        entryOffset = fatOffset % 512;
        if (!cacheSector(fatSector)) return 0x0FFFFFF7UL;
        uint16_t v = rd16(_cacheBuf + entryOffset);
        return (v >= 0xFFF8) ? 0x0FFFFFF8UL : v;
    } else {
        fatOffset   = cluster * 4;
        fatSector   = _fat1Lba + fatOffset / 512;
        entryOffset = fatOffset % 512;
        if (!cacheSector(fatSector)) return 0x0FFFFFF7UL;
        return rd32(_cacheBuf + entryOffset) & 0x0FFFFFFFUL;
    }
}

bool MultiduinoSDClass::fatWriteClusterEntry(uint32_t cluster, uint32_t value) {
    uint32_t fatOffset, fatSector; uint16_t entryOffset;

    if (_fatType == 16) {
        fatOffset   = cluster * 2;
        fatSector   = _fat1Lba + fatOffset / 512;
        entryOffset = fatOffset % 512;
        if (!cacheSector(fatSector)) return false;
        wr16(_cacheBuf + entryOffset, (uint16_t)value);
        _cacheDirty = true;
        if (_numFATs >= 2) {
            flushCache();
            uint32_t fat2 = fatSector + _FATSz;
            if (cacheSector(fat2)) {
                wr16(_cacheBuf + entryOffset, (uint16_t)value);
                _cacheDirty = true; flushCache();
            }
            cacheSector(fatSector);
        }
    } else {
        fatOffset   = cluster * 4;
        fatSector   = _fat1Lba + fatOffset / 512;
        entryOffset = fatOffset % 512;
        if (!cacheSector(fatSector)) return false;
        uint32_t hi = rd32(_cacheBuf + entryOffset) & 0xF0000000UL;
        wr32(_cacheBuf + entryOffset, hi | (value & 0x0FFFFFFFUL));
        _cacheDirty = true;
        if (_numFATs >= 2) {
            flushCache();
            uint32_t fat2 = fatSector + _FATSz;
            if (cacheSector(fat2)) {
                uint32_t hi2 = rd32(_cacheBuf + entryOffset) & 0xF0000000UL;
                wr32(_cacheBuf + entryOffset, hi2 | (value & 0x0FFFFFFFUL));
                _cacheDirty = true; flushCache();
            }
            cacheSector(fatSector);
        }
    }
    return flushCache();
}

uint32_t MultiduinoSDClass::fatAllocCluster(uint32_t prevCluster) {
    uint32_t eoc = (_fatType == 16) ? 0xFFFFUL : 0x0FFFFFFFUL;
    for (uint32_t c = 2; c < _clusterCnt + 2; c++) {
        if (fatNextCluster(c) == 0) {
            fatWriteClusterEntry(c, eoc);
            if (prevCluster) fatWriteClusterEntry(prevCluster, c);
            // Zero the cluster
            for (uint8_t s = 0; s < _secPerClus; s++) {
                uint32_t lba = fatClusterToLba(c) + s;
                if (cacheSector(lba)) { memset(_cacheBuf, 0, 512); _cacheDirty = true; flushCache(); }
            }
            return c;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 8.3 name helpers
// ---------------------------------------------------------------------------
void MultiduinoSDClass::fat83Name(const char* seg, char* out83) const {
    memset(out83, ' ', 11);
    const char* dot = nullptr;
    for (const char* p = seg; *p; p++) if (*p == '.') dot = p;
    uint8_t ni = 0;
    for (const char* p = seg; *p && p != dot && ni < 8; p++, ni++)
        out83[ni] = (char)toupper((uint8_t)*p);
    if (dot) {
        uint8_t ei = 8;
        for (const char* p = dot + 1; *p && ei < 11; p++, ei++)
            out83[ei] = (char)toupper((uint8_t)*p);
    }
}

void MultiduinoSDClass::fat83ToString(const uint8_t* raw83, char* out) const {
    uint8_t ni = 0;
    while (ni < 8 && raw83[ni] != ' ') out[ni] = raw83[ni], ni++;
    uint8_t oi = ni;
    bool hasExt = false;
    for (uint8_t i = 8; i < 11; i++) if (raw83[i] != ' ') { hasExt = true; break; }
    if (hasExt) {
        out[oi++] = '.';
        for (uint8_t i = 8; i < 11 && raw83[i] != ' '; i++) out[oi++] = raw83[i];
    }
    out[oi] = '\0';
}

// ---------------------------------------------------------------------------
// Directory entry search
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::fatFindEntry(uint32_t dirCluster, const char* name83,
                                      uint32_t& outSector, uint16_t& outOffset,
                                      uint8_t* entryBuf) {
    uint32_t cluster = dirCluster;
    bool fat16Root = (_fatType == 16 && cluster == 0);
    uint32_t startLba  = fat16Root ? _rootLba : fatClusterToLba(cluster);
    uint32_t totalSecs = fat16Root ? ((uint32_t)_rootEntCnt * 32 / 512) : _secPerClus;
    uint32_t secIdx = 0;

    while (true) {
        if (secIdx >= totalSecs && !fat16Root) {
            cluster = fatNextCluster(cluster);
            if (cluster >= 0x0FFFFFF8UL) return false;
            startLba = fatClusterToLba(cluster); secIdx = 0;
        }
        if (secIdx >= totalSecs) return false;

        uint32_t lba = startLba + secIdx++;
        if (!cacheSector(lba)) return false;

        for (uint16_t off = 0; off < 512; off += 32) {
            uint8_t first = _cacheBuf[off];
            if (first == 0x00) return false;
            if (first == 0xE5 || _cacheBuf[off + 11] == 0x0F) continue;
            if (name83 && memcmp(_cacheBuf + off, name83, 11) == 0) {
                memcpy(entryBuf, _cacheBuf + off, 32);
                outSector = lba; outOffset = off;
                return true;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Create directory entry
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::fatCreateEntry(uint32_t dirCluster, const char* name83,
                                        bool isDir, uint32_t& outSector,
                                        uint16_t& outOffset, uint8_t* entryBuf) {
    uint32_t cluster = dirCluster;
    bool fat16Root = (_fatType == 16 && cluster == 0);
    uint32_t startLba  = fat16Root ? _rootLba : fatClusterToLba(cluster);
    uint32_t totalSecs = fat16Root ? ((uint32_t)_rootEntCnt * 32 / 512) : _secPerClus;
    uint32_t secIdx = 0;
    uint32_t prevCluster = 0;

    while (true) {
        if (secIdx >= totalSecs) {
            if (fat16Root) return false;
            prevCluster = cluster;
            cluster = fatNextCluster(cluster);
            if (cluster >= 0x0FFFFFF8UL) {
                cluster = fatAllocCluster(prevCluster);
                if (!cluster) return false;
            }
            startLba = fatClusterToLba(cluster); secIdx = 0;
        }

        uint32_t lba = startLba + secIdx++;
        if (!cacheSector(lba)) return false;

        for (uint16_t off = 0; off < 512; off += 32) {
            uint8_t first = _cacheBuf[off];
            if (first == 0x00 || first == 0xE5) {
                memset(_cacheBuf + off, 0, 32);
                memcpy(_cacheBuf + off, name83, 11);
                _cacheBuf[off + 11] = isDir ? 0x10 : 0x20;
                _cacheDirty = true;
                flushCache();
                cacheSector(lba);
                memcpy(entryBuf, _cacheBuf + off, 32);
                outSector = lba; outOffset = off;
                return true;
            }
        }
    }
}

bool MultiduinoSDClass::fatWriteEntry(uint32_t sector, uint16_t offset,
                                       const uint8_t* entryBuf) {
    if (!cacheSector(sector)) return false;
    memcpy(_cacheBuf + offset, entryBuf, 32);
    _cacheDirty = true;
    return flushCache();
}

// ---------------------------------------------------------------------------
// Path resolution
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::fatResolvePath(const char* path,
                                        uint32_t& outDirCluster,
                                        uint32_t& outSector, uint16_t& outOffset,
                                        uint8_t* entryBuf) {
    if (!path || !*path) return false;
    uint32_t cur = (_fatType == 32) ? _rootClus : 0;
    const char* p = path;
    if (*p == '/') p++;

    char seg[13]; char name83[11];

    while (*p) {
        uint8_t si = 0;
        while (*p && *p != '/' && si < 12) seg[si++] = *p++;
        seg[si] = '\0';
        if (*p == '/') p++;
        if (!si) continue;

        fat83Name(seg, name83);
        bool last = (*p == '\0');

        if (last) {
            outDirCluster = cur;
            return fatFindEntry(cur, name83, outSector, outOffset, entryBuf);
        } else {
            uint32_t sec; uint16_t off; uint8_t tmp[32];
            if (!fatFindEntry(cur, name83, sec, off, tmp)) return false;
            if (!(tmp[11] & 0x10)) return false;
            uint32_t fc = ((uint32_t)rd16(tmp + 20) << 16) | rd16(tmp + 26);
            cur = fc ? fc : ((_fatType == 32) ? _rootClus : 0);
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// begin
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::begin() {
    _mounted = false;
    _cacheLba = 0xFFFFFFFFUL; _cacheDirty = false;
    for (uint8_t i = 0; i < SD_MAX_OPEN_FILES; i++) _files[i]._open = false;
    if (!sdInit()) return false;
    if (!fatMount()) return false;
    _mounted = true;
    return true;
}

void MultiduinoSDClass::printCardInfo(Print& out) {
    if (!_mounted) { out.println(F("SD: not mounted")); return; }
    out.print(F("Card type : ")); out.println(cardType());
    out.print(F("FAT type  : FAT")); out.println(_fatType);
    out.print(F("Capacity  : ")); out.print(capacityMB()); out.println(F(" MB (approx)"));
    out.print(F("Sectors/cluster: ")); out.println(_secPerClus);
}

const char* MultiduinoSDClass::cardType() const {
    if (_isSDHC) return "SDHC";
    return (_cardVer == 2) ? "SD2" : "SD1";
}
uint32_t MultiduinoSDClass::capacityMB() const {
    return (uint32_t)_clusterCnt * _secPerClus / 2 / 1024;
}

// ---------------------------------------------------------------------------
// open
// ---------------------------------------------------------------------------
SDFile MultiduinoSDClass::open(const char* path, uint8_t flags) {
    if (!_mounted) return SDFile();

    SDFile* slot = nullptr;
    for (uint8_t i = 0; i < SD_MAX_OPEN_FILES; i++)
        if (!_files[i]._open) { slot = &_files[i]; break; }
    if (!slot) return SDFile();

    uint32_t dirCluster; uint32_t sec; uint16_t off; uint8_t entry[32];
    bool found = fatResolvePath(path, dirCluster, sec, off, entry);

    if (!found) {
        if (!(flags & SD_O_CREAT)) return SDFile();
        const char* fname = path;
        for (const char* p = path; *p; p++) if (*p == '/') fname = p + 1;

        uint32_t parentCluster = (_fatType == 32) ? _rootClus : 0;
        if (fname > path + 1) {
            char pp[64]; uint16_t pl = (uint16_t)(fname - path);
            if (pl > 63) return SDFile();
            memcpy(pp, path, pl); pp[pl] = '\0';
            uint32_t ps; uint16_t po; uint8_t pe[32]; uint32_t pdc;
            if (fatResolvePath(pp, pdc, ps, po, pe)) {
                parentCluster = ((uint32_t)rd16(pe + 20) << 16) | rd16(pe + 26);
            }
        }
        char name83[11]; fat83Name(fname, name83);
        if (!fatCreateEntry(parentCluster, name83, false, sec, off, entry)) return SDFile();
        dirCluster = parentCluster;
    }

    bool isDir = (entry[11] & 0x10) != 0;
    slot->_open          = true;
    slot->_isDir         = isDir;
    slot->_dirty         = false;
    slot->_dirSector     = sec;
    slot->_dirOffset     = off;
    slot->_size          = isDir ? 0 : rd32(entry + 28);
    slot->_pos           = 0;
    slot->_startCluster  = ((uint32_t)rd16(entry + 20) << 16) | rd16(entry + 26);
    slot->_curCluster    = slot->_startCluster;
    slot->_clusterOffset = 0;
    slot->_dirCluster    = dirCluster;
    slot->_dirIterSector = 0;
    slot->_dirIterOffset = 0;
    fat83ToString(entry, slot->_name);

    if (isDir) return *slot;

    if ((flags & SD_O_TRUNC) && (flags & SD_O_WRITE)) {
        uint32_t c = slot->_startCluster;
        while (c >= 2 && c < 0x0FFFFFF8UL) {
            uint32_t nx = fatNextCluster(c); fatWriteClusterEntry(c, 0); c = nx;
        }
        slot->_startCluster = slot->_curCluster = slot->_size = 0;
        uint8_t ent[32]; memcpy(ent, entry, 32);
        wr32(ent + 28, 0); wr16(ent + 26, 0); wr16(ent + 20, 0);
        fatWriteEntry(sec, off, ent);
    }

    if ((flags & SD_O_APPEND) && slot->_size > 0) _fileSeek(slot, slot->_size);
    return *slot;
}

// ---------------------------------------------------------------------------
// File I/O (called by SDFile delegates)
// ---------------------------------------------------------------------------
uint16_t MultiduinoSDClass::_fileRead(SDFile* f, uint8_t* buf, uint16_t count) {
    if (f->_pos >= f->_size) return 0;
    if (f->_pos + count > f->_size) count = (uint16_t)(f->_size - f->_pos);
    uint16_t rd = 0;
    while (rd < count) {
        if (!f->_curCluster) return rd;
        uint32_t clBytes  = (uint32_t)_secPerClus * 512;
        uint32_t clOff    = f->_pos % clBytes;
        uint32_t secInCl  = clOff / 512;
        uint16_t offInSec = (uint16_t)(clOff % 512);
        uint32_t lba      = fatClusterToLba(f->_curCluster) + secInCl;
        if (!cacheSector(lba)) return rd;
        uint16_t canRd = 512 - offInSec;
        if (canRd > count - rd) canRd = count - rd;
        memcpy(buf + rd, _cacheBuf + offInSec, canRd);
        rd += canRd; f->_pos += canRd;
        if ((f->_pos % clBytes) == 0 && f->_pos < f->_size)
            f->_curCluster = fatNextCluster(f->_curCluster);
    }
    return rd;
}

uint16_t MultiduinoSDClass::_fileWrite(SDFile* f, const uint8_t* buf, uint16_t count) {
    uint16_t wr = 0;
    while (wr < count) {
        uint32_t clBytes = (uint32_t)_secPerClus * 512;
        if (!f->_curCluster || (f->_pos > 0 && (f->_pos % clBytes) == 0)) {
            uint32_t prev = f->_curCluster;
            uint32_t nc   = fatAllocCluster(prev);
            if (!nc) return wr;
            if (!f->_startCluster) {
                f->_startCluster = nc;
                if (cacheSector(f->_dirSector)) {
                    wr16(_cacheBuf + f->_dirOffset + 26, (uint16_t)(nc & 0xFFFF));
                    wr16(_cacheBuf + f->_dirOffset + 20, (uint16_t)(nc >> 16));
                    _cacheDirty = true; flushCache();
                }
            }
            f->_curCluster = nc;
        }
        uint32_t clOff    = f->_pos % clBytes;
        uint32_t secInCl  = clOff / 512;
        uint16_t offInSec = (uint16_t)(clOff % 512);
        uint32_t lba      = fatClusterToLba(f->_curCluster) + secInCl;
        if (!cacheSector(lba)) return wr;
        uint16_t canWr = 512 - offInSec;
        if (canWr > count - wr) canWr = count - wr;
        memcpy(_cacheBuf + offInSec, buf + wr, canWr);
        _cacheDirty = true;
        wr += canWr; f->_pos += canWr;
        if (f->_pos > f->_size) f->_size = f->_pos;
    }
    f->_dirty = true;
    return wr;
}

void MultiduinoSDClass::_fileFlush(SDFile* f) {
    if (!f->_dirty) return;
    flushCache();
    if (cacheSector(f->_dirSector)) {
        wr32(_cacheBuf + f->_dirOffset + 28, f->_size);
        uint32_t sc = f->_startCluster;
        wr16(_cacheBuf + f->_dirOffset + 26, (uint16_t)(sc & 0xFFFF));
        wr16(_cacheBuf + f->_dirOffset + 20, (uint16_t)(sc >> 16));
        _cacheDirty = true; flushCache();
    }
    f->_dirty = false;
}

bool MultiduinoSDClass::_fileSeek(SDFile* f, uint32_t pos) {
    if (pos > f->_size) pos = f->_size;
    uint32_t clBytes = (uint32_t)_secPerClus * 512;
    uint32_t tgt     = pos / clBytes;
    f->_curCluster   = f->_startCluster;
    for (uint32_t i = 0; i < tgt && f->_curCluster < 0x0FFFFFF8UL; i++)
        f->_curCluster = fatNextCluster(f->_curCluster);
    f->_pos = pos;
    return true;
}

SDFile MultiduinoSDClass::_dirNext(SDFile* dir, uint8_t flags) {
    uint32_t cluster  = dir->_startCluster;
    bool fat16Root    = (_fatType == 16 && cluster == 0);
    uint32_t startLba = fat16Root ? _rootLba : fatClusterToLba(cluster);
    uint32_t totSecs  = fat16Root ? ((uint32_t)_rootEntCnt * 32 / 512) : _secPerClus;

    uint32_t& iterSec = dir->_dirIterSector;
    uint16_t& iterOff = dir->_dirIterOffset;

    while (true) {
        if (iterSec >= totSecs && !fat16Root) {
            uint32_t nc = fatNextCluster(cluster);
            if (nc >= 0x0FFFFFF8UL) return SDFile();
            cluster = nc; startLba = fatClusterToLba(cluster); iterSec = 0;
        }
        if (iterSec >= totSecs) return SDFile();

        uint32_t lba = startLba + iterSec;
        if (!cacheSector(lba)) return SDFile();

        while (iterOff < 512) {
            uint8_t first = _cacheBuf[iterOff];
            if (first == 0x00) return SDFile();
            uint8_t attr  = _cacheBuf[iterOff + 11];
            if (first == 0xE5 || attr == 0x0F || attr == 0x08 || first == '.') {
                iterOff += 32; continue;
            }
            uint8_t entry[32]; memcpy(entry, _cacheBuf + iterOff, 32);
            uint32_t eSec = lba; uint16_t eOff = iterOff;
            iterOff += 32;
            if (iterOff >= 512) { iterOff = 0; iterSec++; }

            SDFile* slot = nullptr;
            for (uint8_t i = 0; i < SD_MAX_OPEN_FILES; i++)
                if (!_files[i]._open) { slot = &_files[i]; break; }
            if (!slot) return SDFile();

            bool isDir2 = (entry[11] & 0x10) != 0;
            slot->_open          = true;
            slot->_isDir         = isDir2;
            slot->_dirty         = false;
            slot->_dirSector     = eSec;
            slot->_dirOffset     = eOff;
            slot->_size          = isDir2 ? 0 : rd32(entry + 28);
            slot->_pos           = 0;
            slot->_startCluster  = ((uint32_t)rd16(entry + 20) << 16) | rd16(entry + 26);
            slot->_curCluster    = slot->_startCluster;
            slot->_dirCluster    = dir->_startCluster;
            slot->_dirIterSector = 0;
            slot->_dirIterOffset = 0;
            fat83ToString(entry, slot->_name);
            return *slot;
        }
        iterOff = 0; iterSec++;
    }
}

// ---------------------------------------------------------------------------
// exists / remove / mkdir / rmdir / ls
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::exists(const char* path) {
    if (!_mounted) return false;
    uint32_t dc, sec; uint16_t off; uint8_t entry[32];
    return fatResolvePath(path, dc, sec, off, entry);
}

bool MultiduinoSDClass::remove(const char* path) {
    if (!_mounted) return false;
    uint32_t dc, sec; uint16_t off; uint8_t entry[32];
    if (!fatResolvePath(path, dc, sec, off, entry)) return false;
    if (entry[11] & 0x10) return false;
    uint32_t c = ((uint32_t)rd16(entry + 20) << 16) | rd16(entry + 26);
    while (c >= 2 && c < 0x0FFFFFF8UL) {
        uint32_t nx = fatNextCluster(c); fatWriteClusterEntry(c, 0); c = nx;
    }
    if (!cacheSector(sec)) return false;
    _cacheBuf[off] = 0xE5; _cacheDirty = true;
    return flushCache();
}

bool MultiduinoSDClass::mkdir(const char* path) {
    if (!_mounted || exists(path)) return _mounted;
    const char* fname = path;
    for (const char* p = path; *p; p++) if (*p == '/') fname = p + 1;

    uint32_t parentCluster = (_fatType == 32) ? _rootClus : 0;
    uint16_t pl = (uint16_t)(fname - path);
    if (pl > 1) {
        char pp[64]; if (pl > 63) return false;
        memcpy(pp, path, pl); pp[pl] = '\0';
        uint32_t ps, pdc; uint16_t po; uint8_t pe[32];
        if (fatResolvePath(pp, pdc, ps, po, pe))
            parentCluster = ((uint32_t)rd16(pe + 20) << 16) | rd16(pe + 26);
    }

    char name83[11]; fat83Name(fname, name83);
    uint32_t sec; uint16_t off; uint8_t entry[32];
    if (!fatCreateEntry(parentCluster, name83, true, sec, off, entry)) return false;

    uint32_t nc = fatAllocCluster(0);
    if (!nc) return false;

    if (cacheSector(sec)) {
        wr16(_cacheBuf + off + 26, (uint16_t)(nc & 0xFFFF));
        wr16(_cacheBuf + off + 20, (uint16_t)(nc >> 16));
        _cacheDirty = true; flushCache();
    }

    // Write . and .. entries
    uint32_t dotLba = fatClusterToLba(nc);
    if (!cacheSector(dotLba)) return false;
    memset(_cacheBuf, 0, 512);
    memset(_cacheBuf,      ' ', 11); _cacheBuf[0]  = '.';  _cacheBuf[11] = 0x10;
    wr16(_cacheBuf + 26, (uint16_t)(nc & 0xFFFF)); wr16(_cacheBuf + 20, (uint16_t)(nc >> 16));
    memset(_cacheBuf + 32, ' ', 11); _cacheBuf[32] = '.';  _cacheBuf[33] = '.'; _cacheBuf[43] = 0x10;
    wr16(_cacheBuf + 58, (uint16_t)(parentCluster & 0xFFFF));
    wr16(_cacheBuf + 52, (uint16_t)(parentCluster >> 16));
    _cacheDirty = true;
    return flushCache();
}

bool MultiduinoSDClass::rmdir(const char* path) {
    if (!_mounted) return false;
    uint32_t dc, sec; uint16_t off; uint8_t entry[32];
    if (!fatResolvePath(path, dc, sec, off, entry)) return false;
    if (!(entry[11] & 0x10)) return false;
    uint32_t dc2 = ((uint32_t)rd16(entry + 20) << 16) | rd16(entry + 26);
    uint32_t lba = fatClusterToLba(dc2);
    if (!cacheSector(lba)) return false;
    for (uint16_t o = 64; o < 512; o += 32)
        if (_cacheBuf[o] != 0x00 && _cacheBuf[o] != 0xE5) return false;
    uint32_t c = dc2;
    while (c >= 2 && c < 0x0FFFFFF8UL) {
        uint32_t nx = fatNextCluster(c); fatWriteClusterEntry(c, 0); c = nx;
    }
    if (!cacheSector(sec)) return false;
    _cacheBuf[off] = 0xE5; _cacheDirty = true;
    return flushCache();
}

void MultiduinoSDClass::fatLsDir(uint32_t dirCluster, Print& out, uint8_t depth) {
    bool fat16Root = (_fatType == 16 && dirCluster == 0);
    uint32_t cluster  = dirCluster;
    uint32_t startLba = fat16Root ? _rootLba : fatClusterToLba(cluster);
    uint32_t totSecs  = fat16Root ? ((uint32_t)_rootEntCnt * 32 / 512) : _secPerClus;
    uint32_t secIdx   = 0;

    while (true) {
        if (secIdx >= totSecs && !fat16Root) {
            cluster = fatNextCluster(cluster);
            if (cluster >= 0x0FFFFFF8UL) return;
            startLba = fatClusterToLba(cluster); secIdx = 0;
        }
        if (secIdx >= totSecs) return;

        uint32_t lba = startLba + secIdx++;
        if (!cacheSector(lba)) return;

        for (uint16_t off = 0; off < 512; off += 32) {
            uint8_t first = _cacheBuf[off];
            if (first == 0x00) return;
            if (first == 0xE5 || first == '.' ) continue;
            uint8_t attr = _cacheBuf[off + 11];
            if (attr == 0x0F || attr == 0x08) continue;

            for (uint8_t i = 0; i < depth; i++) out.print(F("  "));
            char nm[13]; fat83ToString(_cacheBuf + off, nm); out.print(nm);

            if (attr & 0x10) {
                out.println('/');
                uint32_t sc = ((uint32_t)rd16(_cacheBuf + off + 20) << 16)
                             | rd16(_cacheBuf + off + 26);
                fatLsDir(sc, out, depth + 1);
                cacheSector(lba);  // Restore cache after recursion
            } else {
                uint32_t sz = rd32(_cacheBuf + off + 28);
                out.print(F("  ")); out.print(sz); out.println(F(" B"));
            }
        }
    }
}

void MultiduinoSDClass::ls(const char* path, Print& out, uint8_t depth) {
    if (!_mounted) { out.println(F("SD: not mounted")); return; }
    uint32_t dirCluster;
    if (!path || !*path || strcmp(path, "/") == 0) {
        dirCluster = (_fatType == 32) ? _rootClus : 0;
    } else {
        uint32_t dc, sec; uint16_t off; uint8_t entry[32];
        if (!fatResolvePath(path, dc, sec, off, entry)) {
            out.print(F("ls: not found: ")); out.println(path); return;
        }
        if (!(entry[11] & 0x10)) { out.println(F("ls: not a directory")); return; }
        dirCluster = ((uint32_t)rd16(entry + 20) << 16) | rd16(entry + 26);
    }
    fatLsDir(dirCluster, out, depth);
}

// ---------------------------------------------------------------------------
// SDFile::peek / readLine
// ---------------------------------------------------------------------------
int SDFile::peek() {
    if (!_open || _isDir || _pos >= _size) return -1;
    uint32_t savedPos = _pos;
    uint8_t b;
    int r = (MultiduinoSD._fileRead(this, &b, 1) == 1) ? (int)b : -1;
    if (r != -1) MultiduinoSD._fileSeek(this, savedPos);
    return r;
}

uint16_t SDFile::readLine(char* buf, uint16_t maxLen) {
    if (!buf || maxLen == 0 || !_open || _isDir) return 0;
    uint16_t n = 0;
    while (n < maxLen - 1) {
        uint8_t b;
        if (MultiduinoSD._fileRead(this, &b, 1) != 1) break;
        if (b == '\n') break;
        if (b == '\r') continue;
        buf[n++] = (char)b;
    }
    buf[n] = '\0';
    return n;
}

// ---------------------------------------------------------------------------
// begin() with card-detect pin (active-LOW)
// ---------------------------------------------------------------------------
bool MultiduinoSDClass::begin(uint8_t cdPin) {
    pinMode(cdPin, INPUT_PULLUP);
    if (digitalRead(cdPin) == HIGH) return false;
    return begin();
}

// ---------------------------------------------------------------------------
// freeMB – count free clusters in the FAT
// ---------------------------------------------------------------------------
uint32_t MultiduinoSDClass::freeMB() const {
    if (!_mounted) return 0;
    uint32_t free = 0;
    MultiduinoSDClass* self = const_cast<MultiduinoSDClass*>(this);
    for (uint32_t c = 2; c < _clusterCnt + 2; c++) {
        uint32_t fatOffset = (_fatType == 16) ? c * 2 : c * 4;
        uint32_t fatSector = _fat1Lba + fatOffset / 512;
        uint16_t entOff    = (uint16_t)(fatOffset % 512);
        if (!self->cacheSector(fatSector)) continue;
        if (_fatType == 16) {
            uint16_t v = (uint16_t)_cacheBuf[entOff] |
                         ((uint16_t)_cacheBuf[entOff + 1] << 8);
            if (v == 0) free++;
        } else {
            uint32_t v = (uint32_t)_cacheBuf[entOff]             |
                         ((uint32_t)_cacheBuf[entOff + 1] << 8)  |
                         ((uint32_t)_cacheBuf[entOff + 2] << 16) |
                         ((uint32_t)_cacheBuf[entOff + 3] << 24);
            if ((v & 0x0FFFFFFFUL) == 0) free++;
        }
    }
    return free * _secPerClus / 2 / 1024;
}

// ---------------------------------------------------------------------------
// volumeLabel – find the volume-label directory entry (attr 0x08)
// ---------------------------------------------------------------------------
void MultiduinoSDClass::volumeLabel(char* buf) {
    buf[0] = '\0';
    if (!_mounted) return;
    bool fat16Root    = (_fatType == 16);
    uint32_t startLba = fat16Root ? _rootLba : fatClusterToLba(_rootClus);
    uint32_t totSecs  = fat16Root ? ((uint32_t)_rootEntCnt * 32 / 512) : _secPerClus;
    for (uint32_t si = 0; si < totSecs; si++) {
        if (!cacheSector(startLba + si)) return;
        for (uint16_t off = 0; off < 512; off += 32) {
            uint8_t first = _cacheBuf[off];
            if (first == 0x00) return;
            if (first == 0xE5) continue;
            if (_cacheBuf[off + 11] == 0x08) {
                uint8_t n = 0;
                for (uint8_t i = 0; i < 11; i++) {
                    char c = (char)_cacheBuf[off + i];
                    if (c != ' ') buf[n++] = c;
                }
                buf[n] = '\0';
                return;
            }
        }
    }
}

// Global instance
MultiduinoSDClass MultiduinoSD;

#pragma once
#include <Arduino.h>
#include <SPI.h>

// ---------------------------------------------------------------------------
// MultiduinoSD  –  Full custom FAT16/FAT32 SD card library
// ---------------------------------------------------------------------------

#define SD_CS_PIN          10
#define SD_MAX_OPEN_FILES   4
#define SD_SECTOR_SIZE    512

#define SD_O_READ    0x01
#define SD_O_WRITE   0x02
#define SD_O_CREAT   0x04
#define SD_O_APPEND  0x08
#define SD_O_TRUNC   0x10

#define FILE_READ  (SD_O_READ)
#define FILE_WRITE (SD_O_READ | SD_O_WRITE | SD_O_CREAT | SD_O_APPEND)

// ---------------------------------------------------------------------------
// SDFile – handle to an open file or directory
// ---------------------------------------------------------------------------
class SDFile {
public:
    SDFile();

    operator bool() const { return _open; }

    // Read up to count bytes into buf.  Returns bytes read (0 = EOF).
    int read(void* buf, uint16_t count);

    // Read a single byte.  Returns -1 on EOF.
    int read();

    // Number of bytes remaining to read.
    int available() const { return _open ? (int)(_size - _pos) : 0; }

    // Peek at the next byte without advancing position.  Returns -1 on EOF.
    int peek();

    // Read a text line into buf (up to maxLen-1 chars).  Newline consumed,
    // not stored.  buf is always null-terminated.  Returns chars stored.
    uint16_t readLine(char* buf, uint16_t maxLen);

    // Write count bytes from buf.  Returns bytes written.
    uint16_t write(const void* buf, uint16_t count);

    // Write a single byte.  Returns 1 on success, 0 on failure.
    uint8_t write(uint8_t b);

    void     flush();
    bool     seek(uint32_t pos);
    uint32_t position()    const { return _pos; }
    uint32_t size()        const { return _size; }
    bool     isDirectory() const { return _isDir; }
    const char* name()     const { return _name; }
    void     printName(Print& out) const { out.print(_name); }

    SDFile openNextFile(uint8_t flags = SD_O_READ);
    void   rewindDirectory();
    void   close();

    // Arduino Print compatibility helpers
    size_t print(const char* s)   { return write((const void*)s, strlen(s)); }
    size_t println(const char* s) { print(s); return write((const void*)"\r\n", 2); }
    size_t println()              { return write((const void*)"\r\n", 2); }

private:
    friend class MultiduinoSDClass;

    bool      _open;
    bool      _isDir;
    bool      _dirty;
    char      _name[13];

    uint32_t  _startCluster;
    uint32_t  _dirSector;
    uint16_t  _dirOffset;
    uint32_t  _size;
    uint32_t  _pos;
    uint32_t  _curCluster;
    uint16_t  _clusterOffset;
    uint32_t  _dirCluster;
    uint32_t  _dirIterSector;
    uint16_t  _dirIterOffset;
};

// ---------------------------------------------------------------------------
// MultiduinoSDClass – top-level public API
// ---------------------------------------------------------------------------
class MultiduinoSDClass {
public:
    // Mount the SD card.  Returns true on success.
    bool begin();

    // Mount with card-detect pin (active-LOW).  Returns false if not inserted.
    bool begin(uint8_t cdPin);

    bool ready() const { return _mounted; }

    // Print card type / FAT info to out.
    void printCardInfo(Print& out);

    // ---- Filesystem operations --------------------------------------------
    SDFile open(const char* path, uint8_t flags = FILE_READ);
    bool   exists(const char* path);
    bool   remove(const char* path);
    bool   mkdir(const char* path);
    bool   rmdir(const char* path);
    void   ls(const char* path, Print& out, uint8_t depth = 0);

    // ---- Card information -------------------------------------------------
    const char* cardType() const;
    uint32_t    capacityMB() const;

    // Scan FAT for free clusters; returns approximate free MB.
    uint32_t    freeMB() const;

    // Used space in MB (capacityMB - freeMB).
    uint32_t    usedMB() const { return capacityMB() - freeMB(); }

    // Cluster size in kilobytes.
    uint16_t    clusterSizeKB() const {
        return (uint16_t)_secPerClus * _bytesPerSec / 1024;
    }

    // Max root-directory entries (FAT16 only; 0 for FAT32).
    uint16_t    rootEntriesMax() const { return _rootEntCnt; }

    // Copy volume label (up to 11 chars + null) into buf (must be >= 12 bytes).
    void        volumeLabel(char* buf);

    uint8_t     fatType() const { return _fatType; }

private:
    bool     sdInit();
    uint8_t  sdCmd(uint8_t cmd, uint32_t arg);
    uint8_t  sdACmd(uint8_t cmd, uint32_t arg);
    bool     sdReadBlock(uint32_t lba, uint8_t* buf);
    bool     sdWriteBlock(uint32_t lba, const uint8_t* buf);
    uint8_t  sdWaitReady(uint16_t timeoutMs);

    bool     fatMount();
    uint32_t fatClusterToLba(uint32_t cluster) const;
    uint32_t fatNextCluster(uint32_t cluster);
    uint32_t fatAllocCluster(uint32_t prevCluster);
    bool     fatWriteClusterEntry(uint32_t cluster, uint32_t value);
    void     fat83Name(const char* path, char* out83) const;
    void     fat83ToString(const uint8_t* raw83, char* out) const;
    bool     fatFindEntry(uint32_t dirCluster, const char* name83,
                          uint32_t& outSector, uint16_t& outOffset,
                          uint8_t* entryBuf);
    bool     fatCreateEntry(uint32_t dirCluster, const char* name83,
                            bool isDir, uint32_t& outSector,
                            uint16_t& outOffset, uint8_t* entryBuf);
    bool     fatWriteEntry(uint32_t sector, uint16_t offset,
                           const uint8_t* entryBuf);
    bool     fatResolvePath(const char* path,
                            uint32_t& outDirCluster,
                            uint32_t& outSector, uint16_t& outOffset,
                            uint8_t* entryBuf);
    void     fatLsDir(uint32_t dirCluster, Print& out, uint8_t depth);
    bool     cacheSector(uint32_t lba);
    bool     flushCache();

public:
    uint16_t _fileRead (SDFile* f, uint8_t* buf, uint16_t count);
    uint16_t _fileWrite(SDFile* f, const uint8_t* buf, uint16_t count);
    void     _fileFlush(SDFile* f);
    bool     _fileSeek (SDFile* f, uint32_t pos);
    SDFile   _dirNext  (SDFile* dir, uint8_t flags);

private:
    bool     _mounted    = false;
    bool     _isSDHC     = false;
    uint8_t  _cardVer    = 0;
    uint8_t  _fatType    = 0;

    uint32_t _partStart  = 0;
    uint16_t _bytesPerSec= 512;
    uint8_t  _secPerClus = 0;
    uint16_t _resSectors = 0;
    uint8_t  _numFATs    = 2;
    uint16_t _rootEntCnt = 0;
    uint32_t _totalSec32 = 0;
    uint32_t _FATSz      = 0;
    uint32_t _rootClus   = 0;
    uint32_t _fat1Lba    = 0;
    uint32_t _rootLba    = 0;
    uint32_t _dataLba    = 0;
    uint32_t _clusterCnt = 0;

    uint8_t  _cacheBuf[SD_SECTOR_SIZE];
    uint32_t _cacheLba   = 0xFFFFFFFFUL;
    bool     _cacheDirty = false;

    SDFile   _files[SD_MAX_OPEN_FILES];
};

extern MultiduinoSDClass MultiduinoSD;

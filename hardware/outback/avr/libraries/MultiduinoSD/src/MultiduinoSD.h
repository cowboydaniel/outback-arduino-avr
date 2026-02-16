#pragma once
#include <Arduino.h>
#include <SPI.h>

// ---------------------------------------------------------------------------
// MultiduinoSD  –  Full custom FAT16/FAT32 SD card library
//
// Implements the complete SD card protocol over hardware SPI, plus a
// FAT16/FAT32 filesystem layer.  No dependency on the Arduino SD library.
//
// Hardware:  Multiduino ATmega328P, CS hardwired to D10 (SD_CS_PIN).
//            Uses Arduino SPI library for hardware SPI (MOSI=D11, MISO=D12,
//            SCK=D13).
//
// Constraints (ATmega328P, 2 KB RAM):
//   - One 512-byte sector cache shared by all operations.
//   - Up to SD_MAX_OPEN_FILES (4) SDFile objects open simultaneously.
//   - 8.3 (short) filenames only.  Long filenames (VFAT) are skipped.
//   - FAT16 and FAT32 supported.  FAT12 (< 4 MB) is not supported.
// ---------------------------------------------------------------------------

#define SD_CS_PIN          10
#define SD_MAX_OPEN_FILES   4
#define SD_SECTOR_SIZE    512

// File open flags (compatible with common usage patterns)
#define SD_O_READ    0x01
#define SD_O_WRITE   0x02
#define SD_O_CREAT   0x04   // Create if not found
#define SD_O_APPEND  0x08   // Seek to end before each write
#define SD_O_TRUNC   0x10   // Truncate to zero on open

// Convenience macros
#define FILE_READ  (SD_O_READ)
#define FILE_WRITE (SD_O_READ | SD_O_WRITE | SD_O_CREAT | SD_O_APPEND)

// ---------------------------------------------------------------------------
// SDFile – handle to an open file or directory
// ---------------------------------------------------------------------------
class SDFile {
public:
    SDFile();

    // Returns true if this handle refers to a valid open file/dir.
    operator bool() const { return _open; }

    // Read up to count bytes into buf.  Returns bytes read (0 = EOF).
    int read(void* buf, uint16_t count);

    // Read a single byte.  Returns -1 on EOF.
    int read();

    // Write count bytes from buf.  Returns bytes written.
    uint16_t write(const void* buf, uint16_t count);

    // Write a single byte.  Returns 1 on success, 0 on failure.
    uint8_t write(uint8_t b);

    // Flush write cache to SD card.
    void flush();

    // Seek to byte position pos from start of file.  Returns true on success.
    bool seek(uint32_t pos);

    // Current read/write position.
    uint32_t position() const { return _pos; }

    // Current file size in bytes.
    uint32_t size() const { return _size; }

    // True if this is a directory entry.
    bool isDirectory() const { return _isDir; }

    // File name (8.3, null-terminated, upper-case).
    const char* name() const { return _name; }

    // Print the file name to a Print object.
    void printName(Print& out) const { out.print(_name); }

    // Open next entry in a directory.  Only valid when isDirectory() is true.
    SDFile openNextFile(uint8_t flags = SD_O_READ);

    // Rewind directory to first entry.
    void rewindDirectory();

    // Close the file and release the slot.
    void close();

    // Arduino Print compatibility helpers
    size_t print(const char* s)       { return write((const void*)s, strlen(s)); }
    size_t println(const char* s)     { print(s); return write((const void*)"\r\n", 2); }
    size_t println()                  { return write((const void*)"\r\n", 2); }

private:
    friend class MultiduinoSDClass;

    bool      _open;
    bool      _isDir;
    bool      _dirty;       // Unsaved data in write buffer
    char      _name[13];    // 8 + '.' + 3 + null

    uint32_t  _startCluster;      // First cluster of the file
    uint32_t  _dirSector;         // Sector that holds this file's dir entry
    uint16_t  _dirOffset;         // Byte offset within that sector
    uint32_t  _size;              // File size in bytes (from dir entry)
    uint32_t  _pos;               // Current read/write position

    uint32_t  _curCluster;        // Cluster being accessed
    uint16_t  _clusterOffset;     // Byte offset within current cluster

    // Directory iteration state
    uint32_t  _dirCluster;        // Starting cluster of parent dir (0=root FAT16)
    uint32_t  _dirIterSector;     // Current sector during openNextFile()
    uint16_t  _dirIterOffset;     // Byte offset within that sector
};

// ---------------------------------------------------------------------------
// MultiduinoSDClass – top-level public API
// ---------------------------------------------------------------------------
class MultiduinoSDClass {
public:
    // Initialise SD card and mount the FAT volume.
    // Returns true on success.
    bool begin();

    // True if a card is mounted and ready.
    bool ready() const { return _mounted; }

    // Print card type (SD/SDHC) and FAT volume info to out.
    void printCardInfo(Print& out);

    // ---- Filesystem operations --------------------------------------------

    // Open a file or directory.
    // flags: FILE_READ, FILE_WRITE, or SD_O_* combination.
    SDFile open(const char* path, uint8_t flags = FILE_READ);

    // Returns true if path exists (file or directory).
    bool exists(const char* path);

    // Delete a file.  Returns true on success.
    bool remove(const char* path);

    // Create a directory (and any missing parent directories).
    bool mkdir(const char* path);

    // Remove an empty directory.
    bool rmdir(const char* path);

    // List directory contents to out.
    // depth controls indent level (start with 0).
    void ls(const char* path, Print& out, uint8_t depth = 0);

    // ---- Card information -------------------------------------------------

    // SD card type string: "SD1", "SD2", or "SDHC"
    const char* cardType() const;

    // Total card capacity in megabytes (approximate).
    uint32_t capacityMB() const;

    // FAT type: 16 or 32.
    uint8_t fatType() const { return _fatType; }

private:
    // ---- SD protocol layer ------------------------------------------------
    bool     sdInit();
    uint8_t  sdCmd(uint8_t cmd, uint32_t arg);
    uint8_t  sdACmd(uint8_t cmd, uint32_t arg);
    bool     sdReadBlock(uint32_t lba, uint8_t* buf);
    bool     sdWriteBlock(uint32_t lba, const uint8_t* buf);
    uint8_t  sdWaitReady(uint16_t timeoutMs);

    // ---- FAT layer --------------------------------------------------------
    bool     fatMount();
    uint32_t fatClusterToLba(uint32_t cluster) const;
    uint32_t fatNextCluster(uint32_t cluster);
    uint32_t fatAllocCluster(uint32_t prevCluster);
    bool     fatWriteClusterEntry(uint32_t cluster, uint32_t value);
    void     fat83Name(const char* path, char* out83) const; // path segment → 11-byte 8.3
    void     fat83ToString(const uint8_t* raw83, char* out) const; // 11-byte 8.3 → "NAME.EXT"

    // Find a directory entry by 8.3 name in dir starting at cluster.
    // cluster=0 means root dir on FAT16.
    // Fills sector, offset, and the raw 32-byte dir entry buf[32].
    bool     fatFindEntry(uint32_t dirCluster, const char* name83,
                          uint32_t& outSector, uint16_t& outOffset,
                          uint8_t* entryBuf);

    // Create a new directory entry.
    bool     fatCreateEntry(uint32_t dirCluster, const char* name83,
                            bool isDir, uint32_t& outSector,
                            uint16_t& outOffset, uint8_t* entryBuf);

    // Write 32-byte dir entry back to its sector+offset.
    bool     fatWriteEntry(uint32_t sector, uint16_t offset,
                           const uint8_t* entryBuf);

    // Walk a slash-delimited path and resolve the final component.
    // parentCluster is set to the cluster of the parent directory.
    bool     fatResolvePath(const char* path,
                            uint32_t& outDirCluster,
                            uint32_t& outSector, uint16_t& outOffset,
                            uint8_t* entryBuf);

    // Recursively list a directory.
    void     fatLsDir(uint32_t dirCluster, Print& out, uint8_t depth);

    // Sector I/O with single-sector cache.
    bool     cacheSector(uint32_t lba);
    bool     flushCache();

    // ---- Internal file I/O helpers ----------------------------------------
    // Called by SDFile via the extern singleton.
    // Declared as public so SDFile (a separate class) can call them without
    // making MultiduinoSDClass a friend of each SDFile instance.
public:
    uint16_t _fileRead (SDFile* f, uint8_t* buf, uint16_t count);
    uint16_t _fileWrite(SDFile* f, const uint8_t* buf, uint16_t count);
    void     _fileFlush(SDFile* f);
    bool     _fileSeek (SDFile* f, uint32_t pos);
    SDFile   _dirNext  (SDFile* dir, uint8_t flags);

private:
    // ---- Private state ----------------------------------------------------
    bool     _mounted   = false;
    bool     _isSDHC    = false;
    uint8_t  _cardVer   = 0;     // 1=SD1, 2=SD2, 3=SDHC
    uint8_t  _fatType   = 0;     // 16 or 32

    // Partition / BPB fields
    uint32_t _partStart  = 0;    // LBA of first sector of partition
    uint16_t _bytesPerSec= 512;
    uint8_t  _secPerClus = 0;
    uint16_t _resSectors = 0;
    uint8_t  _numFATs    = 2;
    uint16_t _rootEntCnt = 0;    // FAT16 only
    uint32_t _totalSec32 = 0;
    uint32_t _FATSz      = 0;    // Sectors per FAT
    uint32_t _rootClus   = 0;    // FAT32 root cluster
    uint32_t _fat1Lba    = 0;    // LBA of FAT1
    uint32_t _rootLba    = 0;    // FAT16: LBA of root dir; FAT32: from cluster
    uint32_t _dataLba    = 0;    // LBA of first data sector
    uint32_t _clusterCnt = 0;    // Total data clusters

    // Cache
    uint8_t  _cacheBuf[SD_SECTOR_SIZE];
    uint32_t _cacheLba  = 0xFFFFFFFFUL;
    bool     _cacheDirty= false;

    // Open file slots
    SDFile   _files[SD_MAX_OPEN_FILES];
};

extern MultiduinoSDClass MultiduinoSD;

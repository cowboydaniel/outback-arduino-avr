/*
  SDBenchmark
  ===========
  Measures sequential write and read throughput by timing large block I/O.

  Writes a 16 KB file in 512-byte chunks, then reads it back.  Reports
  throughput in bytes per second for both directions.

  Hardware: Multiduino with SD card on D10.
  Open Serial Monitor at 115200 baud.
*/

#include <MultiduinoSD.h>

#define BENCH_FILE   "BENCH.BIN"
#define CHUNK_SIZE   512
#define NUM_CHUNKS   32          // 32 * 512 = 16384 bytes

static uint8_t g_buf[CHUNK_SIZE];

void setup() {
    Serial.begin(115200);
    if (!MultiduinoSD.begin()) {
        Serial.println(F("SD init failed!")); while (true) {}
    }
    Serial.println(F("=== SDBenchmark ===\n"));

    // Fill chunk with a known pattern
    for (uint16_t i = 0; i < CHUNK_SIZE; i++) g_buf[i] = (uint8_t)(i & 0xFF);

    if (MultiduinoSD.exists(BENCH_FILE)) MultiduinoSD.remove(BENCH_FILE);

    // ---- Write benchmark ----
    SDFile fw = MultiduinoSD.open(BENCH_FILE,
                    SD_O_WRITE | SD_O_CREAT | SD_O_TRUNC);
    if (!fw) { Serial.println(F("create failed")); return; }

    uint32_t t0 = millis();
    for (uint8_t i = 0; i < NUM_CHUNKS; i++) fw.write(g_buf, CHUNK_SIZE);
    fw.close();
    uint32_t writeMs = millis() - t0;

    uint32_t totalBytes = (uint32_t)NUM_CHUNKS * CHUNK_SIZE;
    Serial.print(F("Write: ")); Serial.print(totalBytes); Serial.print(F(" bytes in "));
    Serial.print(writeMs); Serial.print(F(" ms  → "));
    if (writeMs > 0)
        Serial.print((uint32_t)(totalBytes * 1000UL / writeMs));
    else
        Serial.print(F(">inf"));
    Serial.println(F(" B/s"));

    // ---- Read benchmark ----
    SDFile fr = MultiduinoSD.open(BENCH_FILE, FILE_READ);
    if (!fr) { Serial.println(F("read-open failed")); return; }

    t0 = millis();
    uint32_t total = 0;
    while (fr.available()) {
        uint16_t n = fr.read(g_buf, CHUNK_SIZE);
        total += n;
    }
    fr.close();
    uint32_t readMs = millis() - t0;

    Serial.print(F("Read : ")); Serial.print(total); Serial.print(F(" bytes in "));
    Serial.print(readMs); Serial.print(F(" ms  → "));
    if (readMs > 0)
        Serial.print((uint32_t)(total * 1000UL / readMs));
    else
        Serial.print(F(">inf"));
    Serial.println(F(" B/s"));

    // Card info
    Serial.println();
    Serial.print(F("Capacity : ")); Serial.print(MultiduinoSD.capacityMB()); Serial.println(F(" MB"));
    Serial.print(F("Free     : ")); Serial.print(MultiduinoSD.freeMB());     Serial.println(F(" MB"));
    Serial.print(F("Used     : ")); Serial.print(MultiduinoSD.usedMB());     Serial.println(F(" MB"));
    Serial.print(F("Cluster  : ")); Serial.print(MultiduinoSD.clusterSizeKB()); Serial.println(F(" KB"));
}

void loop() {}

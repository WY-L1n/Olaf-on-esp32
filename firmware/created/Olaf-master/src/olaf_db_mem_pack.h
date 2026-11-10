// Olaf memory database record format.
// Fingerprints keep the complete 64-bit hash and the full 32-bit time/id.
#ifndef OLAF_DB_MEM_PACK_H
#define OLAF_DB_MEM_PACK_H

#include <stdint.h>

typedef struct Olaf_Mem_Fingerprint {
    uint64_t hash;
    uint32_t time;
    uint32_t audio_id;
} Olaf_Mem_Fingerprint;

typedef struct Olaf_Mem_Metadata {
    uint32_t audio_id;
    double duration;
    long fingerprints;
    const char *path;
} Olaf_Mem_Metadata;

#endif

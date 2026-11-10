// Olaf: Overly Lightweight Acoustic Fingerprinting
// Copyright (C) 2019-2025 Joren Six
#include <stdlib.h>
#include <stdint.h>
#include "olaf_db.h"
#include "olaf_fp_db_writer.h"
#include "olaf_fp_extractor.h"

/* The memory DB owns the records. This writer simply forwards each batch,
   preserving the same value layout as the regular Olaf DB:
   high 32 bits = fingerprint time, low 32 bits = audio identifier. */
struct Olaf_FP_DB_Writer {
    Olaf_DB *db;
    uint32_t audio_file_identifier;
};

Olaf_FP_DB_Writer *olaf_fp_db_writer_new(Olaf_DB *db, uint32_t audio_file_identifier) {
    Olaf_FP_DB_Writer *writer = (Olaf_FP_DB_Writer *)calloc(1, sizeof(*writer));
    if (writer == NULL) return NULL;
    writer->db = db;
    writer->audio_file_identifier = audio_file_identifier;
    return writer;
}

void olaf_fp_db_writer_store(Olaf_FP_DB_Writer *writer,
                             struct extracted_fingerprints *fingerprints) {
    if (writer == NULL || fingerprints == NULL) return;
    size_t n = fingerprints->fingerprintIndex;
    if (n == 0) return;

    uint64_t *keys = (uint64_t *)malloc(n * sizeof(uint64_t));
    uint64_t *values = (uint64_t *)malloc(n * sizeof(uint64_t));
    if (keys == NULL || values == NULL) {
        free(keys);
        free(values);
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        keys[i] = olaf_fp_extractor_hash(fingerprints->fingerprints[i]);
        values[i] = ((uint64_t)fingerprints->fingerprints[i].timeIndex1 << 32)
                  | (uint64_t)writer->audio_file_identifier;
    }
    olaf_db_store(writer->db, keys, values, n);
    free(keys);
    free(values);
    fingerprints->fingerprintIndex = 0;
}

void olaf_fp_db_writer_delete(Olaf_FP_DB_Writer *writer,
                              struct extracted_fingerprints *fingerprints) {
    (void)writer;
    (void)fingerprints;
}

void olaf_fp_db_writer_destroy(Olaf_FP_DB_Writer *writer, bool store) {
    (void)store;
    free(writer);
}

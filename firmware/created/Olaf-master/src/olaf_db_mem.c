// Olaf: Overly Lightweight Acoustic Fingerprinting
// Copyright (C) 2019-2025 Joren Six
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "olaf_db.h"
#include "olaf_db_mem_pack.h"
#include "olaf_fp_ref_mem.h"

struct Olaf_DB {
    bool building;
    Olaf_Mem_Fingerprint *build_fps;
    size_t build_fp_count;
    size_t build_fp_capacity;
    Olaf_Mem_Metadata *build_meta;
    size_t build_meta_count;
    size_t build_meta_capacity;

    const Olaf_Mem_Fingerprint *ref_fp;
    size_t ref_fp_length;
    const Olaf_Mem_Metadata *ref_meta;
    size_t ref_meta_length;
};

static int fingerprint_compare(const void *a, const void *b) {
    const Olaf_Mem_Fingerprint *x = (const Olaf_Mem_Fingerprint *)a;
    const Olaf_Mem_Fingerprint *y = (const Olaf_Mem_Fingerprint *)b;
    return (x->hash > y->hash) - (x->hash < y->hash);
}
static int fingerprint_key_compare(const void *key, const void *element) {
    uint64_t hash = *(const uint64_t *)key;
    const Olaf_Mem_Fingerprint *fp = (const Olaf_Mem_Fingerprint *)element;
    return (hash > fp->hash) - (hash < fp->hash);
}
static bool reserve_fps(Olaf_DB *db, size_t extra) {
    if (extra > SIZE_MAX - db->build_fp_count) return false;
    size_t need = db->build_fp_count + extra;
    if (need <= db->build_fp_capacity) return true;
    size_t cap = db->build_fp_capacity ? db->build_fp_capacity : 4096;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) { cap = need; break; }
        cap *= 2;
    }
    Olaf_Mem_Fingerprint *p = (Olaf_Mem_Fingerprint *)realloc(
        db->build_fps, cap * sizeof(*p));
    if (!p) return false;
    db->build_fps = p;
    db->build_fp_capacity = cap;
    return true;
}
static bool reserve_meta(Olaf_DB *db) {
    if (db->build_meta_count < db->build_meta_capacity) return true;
    size_t cap = db->build_meta_capacity ? db->build_meta_capacity * 2 : 16;
    Olaf_Mem_Metadata *p = (Olaf_Mem_Metadata *)realloc(
        db->build_meta, cap * sizeof(*p));
    if (!p) return false;
    db->build_meta = p;
    db->build_meta_capacity = cap;
    return true;
}
static char *copy_string(const char *source) {
    if (!source) source = "";
    size_t n = strlen(source) + 1;
    char *copy = (char *)malloc(n);
    if (copy) memcpy(copy, source, n);
    return copy;
}
static const Olaf_Mem_Metadata *find_meta(const Olaf_DB *db, uint32_t id) {
    const Olaf_Mem_Metadata *items = db->building ? db->build_meta : db->ref_meta;
    size_t count = db->building ? db->build_meta_count : db->ref_meta_length;
    for (size_t i = 0; i < count; ++i)
        if (items[i].audio_id == id) return &items[i];
    return NULL;
}

/* Create a build DB when readonly is false, or reference the generated
   immutable arrays when readonly is true. */
Olaf_DB *olaf_db_new(const char *db_file_folder, bool readonly) {
    (void)db_file_folder;
    Olaf_DB *db = (Olaf_DB *)calloc(1, sizeof(*db));
    if (!db) return NULL;
    db->building = !readonly;
    if (readonly) {
        db->ref_fp = olaf_db_mem_fps;
        db->ref_fp_length = olaf_db_mem_fp_length;
        db->ref_meta = olaf_db_mem_metadata;
        db->ref_meta_length = olaf_db_mem_metadata_length;
        fprintf(stderr, "Mem DB: %zu fingerprints, %zu audio files\n",
                db->ref_fp_length, db->ref_meta_length);
    }
    return db;
}

void olaf_db_store(Olaf_DB *db, uint64_t *keys, uint64_t *values, size_t size) {
    if (!db || !db->building || !keys || !values || size == 0) return;
    if (!reserve_fps(db, size)) {
        fprintf(stderr, "ERROR: cannot grow memory fingerprint database\n");
        return;
    }
    for (size_t i = 0; i < size; ++i) {
        Olaf_Mem_Fingerprint *fp = &db->build_fps[db->build_fp_count++];
        fp->hash = keys[i];
        fp->time = (uint32_t)(values[i] >> 32);
        fp->audio_id = (uint32_t)values[i];
    }
}

void olaf_db_delete(Olaf_DB *db, uint64_t *keys, uint64_t *values, size_t size) {
    if (!db || !db->building || !keys || !values) return;
    for (size_t k = 0; k < size; ++k) {
        uint32_t id = (uint32_t)values[k];
        uint32_t time = (uint32_t)(values[k] >> 32);
        for (size_t i = 0; i < db->build_fp_count; ) {
            Olaf_Mem_Fingerprint *fp = &db->build_fps[i];
            if (fp->hash == keys[k] && fp->time == time && fp->audio_id == id) {
                db->build_fps[i] = db->build_fps[--db->build_fp_count];
            } else ++i;
        }
    }
}

size_t olaf_db_find(Olaf_DB *db, uint64_t start_key, uint64_t stop_key,
                    uint64_t *results, size_t results_size) {
    if (!db || !results || start_key > stop_key) return 0;
    const Olaf_Mem_Fingerprint *fps = db->building ? db->build_fps : db->ref_fp;
    size_t count = db->building ? db->build_fp_count : db->ref_fp_length;
    size_t out = 0;

    /* During build mode the array is not sorted; query mode uses the sorted
       generated array. */
    if (db->building) {
        for (size_t i = 0; i < count; ++i) {
            if (fps[i].hash >= start_key && fps[i].hash <= stop_key) {
                if (out < results_size)
                    results[out++] = ((uint64_t)fps[i].time << 32) | fps[i].audio_id;
            }
        }
        return out;
    }

    for (uint64_t hash = start_key; ; ++hash) {
        const Olaf_Mem_Fingerprint *match = (const Olaf_Mem_Fingerprint *)
            bsearch(&hash, fps, count, sizeof(*fps), fingerprint_key_compare);
        if (match) {
            size_t index = (size_t)(match - fps);
            size_t first = index;
            while (first > 0 && fps[first - 1].hash == hash) --first;
            for (size_t i = first; i < count && fps[i].hash == hash; ++i) {
                if (out < results_size)
                    results[out++] = ((uint64_t)fps[i].time << 32) | fps[i].audio_id;
            }
        }
        if (hash == stop_key || hash == UINT64_MAX) break;
    }
    return out;
}

bool olaf_db_find_single(Olaf_DB *db, uint64_t start_key, uint64_t stop_key) {
    if (!db || start_key > stop_key) return false;
    const Olaf_Mem_Fingerprint *fps = db->building ? db->build_fps : db->ref_fp;
    size_t count = db->building ? db->build_fp_count : db->ref_fp_length;
    for (size_t i = 0; i < count; ++i)
        if (fps[i].hash >= start_key && fps[i].hash <= stop_key) return true;
    return false;
}

/* Escape strings so arbitrary source file paths remain valid C literals. */
static void print_c_string(const char *s) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p; ++p) {
        if (*p == '\\' || *p == '"') { putchar('\\'); putchar(*p); }
        else if (*p == '\n') fputs("\\n", stdout);
        else if (*p == '\r') fputs("\\r", stdout);
        else if (*p == '\t') fputs("\\t", stdout);
        else if (*p < 32 || *p >= 127) printf("\\x%02X", *p);
        else putchar(*p);
    }
    putchar('"');
}

void olaf_db_destroy(Olaf_DB *db) {
    if (!db) return;
    if (db->building) {
        qsort(db->build_fps, db->build_fp_count, sizeof(*db->build_fps),
              fingerprint_compare);
        printf("/* Generated Olaf multi-song memory database. */\n");
        printf("#ifndef OLAF_FP_REF_MEM_H\n#define OLAF_FP_REF_MEM_H\n");
        printf("#include <stddef.h>\n#include <stdint.h>\n#include \"olaf_db_mem_pack.h\"\n\n");
        printf("const Olaf_Mem_Fingerprint olaf_db_mem_fps[] = {\n");
        if (db->build_fp_count == 0) printf("    {0ULL, 0U, 0U},\n");
        for (size_t i = 0; i < db->build_fp_count; ++i) {
            Olaf_Mem_Fingerprint *fp = &db->build_fps[i];
            printf("    { UINT64_C(%" PRIu64 "), UINT32_C(%" PRIu32 "), UINT32_C(%" PRIu32 ") },\n",
                   fp->hash, fp->time, fp->audio_id);
        }
        printf("};\nconst size_t olaf_db_mem_fp_length = %zu;\n\n",
               db->build_fp_count);
        printf("const Olaf_Mem_Metadata olaf_db_mem_metadata[] = {\n");
        if (db->build_meta_count == 0)
            printf("    {0U, 0.0, 0L, \"\"},\n");
        for (size_t i = 0; i < db->build_meta_count; ++i) {
            Olaf_Mem_Metadata *m = &db->build_meta[i];
            printf("    { UINT32_C(%" PRIu32 "), %.9g, %ld, ",
                   m->audio_id, m->duration, m->fingerprints);
            print_c_string(m->path);
            printf(" },\n");
        }
        printf("};\nconst size_t olaf_db_mem_metadata_length = %zu;\n#endif\n",
               db->build_meta_count);
    }
    for (size_t i = 0; i < db->build_meta_count; ++i)
        free((void *)db->build_meta[i].path);
    free(db->build_meta);
    free(db->build_fps);
    free(db);
}

void olaf_db_store_meta_data(Olaf_DB *db, uint32_t *key,
                             Olaf_Resource_Meta_data *value) {
    if (!db || !db->building || !key || !value) return;
    for (size_t i = 0; i < db->build_meta_count; ++i) {
        if (db->build_meta[i].audio_id == *key) {
            free((void *)db->build_meta[i].path);
            db->build_meta[i].path = copy_string(value->path);
            db->build_meta[i].duration = value->duration;
            db->build_meta[i].fingerprints = value->fingerprints;
            return;
        }
    }
    if (!reserve_meta(db)) return;
    char *path = copy_string(value->path);
    if (!path) return;
    Olaf_Mem_Metadata *m = &db->build_meta[db->build_meta_count++];
    m->audio_id = *key;
    m->duration = value->duration;
    m->fingerprints = value->fingerprints;
    m->path = path;
}

void olaf_db_find_meta_data(Olaf_DB *db, uint32_t *key,
                            Olaf_Resource_Meta_data *value) {
    if (!db || !key || !value) return;
    const Olaf_Mem_Metadata *m = find_meta(db, *key);
    if (!m) return;
    value->duration = (float)m->duration;
    value->fingerprints = m->fingerprints;
    snprintf(value->path, sizeof(value->path), "%s", m->path ? m->path : "");
}

void olaf_db_delete_meta_data(Olaf_DB *db, uint32_t *key) {
    if (!db || !db->building || !key) return;
    for (size_t i = 0; i < db->build_meta_count; ++i) {
        if (db->build_meta[i].audio_id == *key) {
            free((void *)db->build_meta[i].path);
            db->build_meta[i] = db->build_meta[--db->build_meta_count];
            return;
        }
    }
}

bool olaf_db_has_meta_data(Olaf_DB *db, uint32_t *key) {
    return db && key && find_meta(db, *key) != NULL;
}

void olaf_db_stats(Olaf_DB *db, bool verbose) {
    (void)verbose;
    if (!db) return;
    size_t fps = db->building ? db->build_fp_count : db->ref_fp_length;
    size_t songs = db->building ? db->build_meta_count : db->ref_meta_length;
    printf("Number of fingerprints: %zu\nNumber of audio files: %zu\n", fps, songs);
}
void olaf_db_print_stats(Olaf_DB *db, bool include_files) {
    (void)include_files;
    olaf_db_stats(db, false);
}
void olaf_db_stats_meta_data(Olaf_DB *db, bool verbose) {
    (void)verbose;
    if (!db) return;
    const Olaf_Mem_Metadata *items = db->building ? db->build_meta : db->ref_meta;
    size_t count = db->building ? db->build_meta_count : db->ref_meta_length;
    for (size_t i = 0; i < count; ++i)
        printf("%s;%u;%.3f;%ld\n", items[i].path, items[i].audio_id,
               items[i].duration, items[i].fingerprints);
}

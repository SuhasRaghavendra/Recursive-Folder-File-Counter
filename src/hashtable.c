/*
 * hashtable.c - Hash Table Implementation for Cycle Detection
 * ------------------------------------------------------------
 * Implements an open-addressing hash table with linear probing,
 * storing FolderID values (Windows volume serial + file index pairs).
 *
 * DAA Concepts Demonstrated:
 *   1. Hash Function Design: A good hash function distributes keys
 *      uniformly across buckets. We combine the three 32-bit fields
 *      of a FolderID using the FNV-1a technique (XOR + multiply) to
 *      produce a well-distributed 64-bit hash value.
 *
 *   2. Collision Resolution (Open Addressing / Linear Probing):
 *      When two keys map to the same bucket, linear probing walks
 *      forward through subsequent slots until an empty or matching
 *      bucket is found. This keeps all data in one contiguous array
 *      (good cache behaviour) at the cost of clustering on high load.
 *
 *   3. Load Factor & Rehashing:
 *      A load factor above 70% causes linear-probe chains to grow
 *      rapidly, degrading O(1) expected lookup to O(N) worst case.
 *      When the load exceeds HT_MAX_LOAD_FACTOR we rehash into a
 *      table roughly twice as large (next value after 2x that works),
 *      restoring O(1) amortized performance.
 *
 *   4. Cycle Detection in Directed Graphs:
 *      The filesystem with junctions/symlinks forms a directed graph.
 *      Before descending into any directory we look up its FolderID:
 *        - Not found → first visit → insert and descend.
 *        - Found      → already visited → cycle detected → skip.
 *      This is the visited-set approach from classic DFS cycle detection.
 */

#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * folderIdHash - Compute a hash index for a FolderID.
 *
 * We treat the three 32-bit fields as raw bytes and run them through
 * the FNV-1a (Fowler-Noll-Vo) algorithm, which has excellent avalanche
 * properties: flipping any single input bit flips ~50% of output bits.
 *
 * The result is reduced modulo `capacity` to get a valid bucket index.
 */
static size_t folderIdHash(FolderID id, size_t capacity) {
    /* FNV-1a 64-bit constants. */
    unsigned long long hash   = 14695981039346656037ULL;
    unsigned long long fnvPrime = 1099511628211ULL;

    /* Mix each byte of the three 32-bit integer fields. */
    unsigned char *bytes;
    int i;

    bytes = (unsigned char *)&id.volumeSerial;
    for (i = 0; i < 4; i++) { hash ^= bytes[i]; hash *= fnvPrime; }

    bytes = (unsigned char *)&id.fileIndexLow;
    for (i = 0; i < 4; i++) { hash ^= bytes[i]; hash *= fnvPrime; }

    bytes = (unsigned char *)&id.fileIndexHigh;
    for (i = 0; i < 4; i++) { hash ^= bytes[i]; hash *= fnvPrime; }

    return (size_t)(hash % (unsigned long long)capacity);
}

/*
 * folderIdEqual - Return 1 if two FolderID values represent the same
 * filesystem object, 0 otherwise.
 */
static int folderIdEqual(FolderID a, FolderID b) {
    return a.volumeSerial  == b.volumeSerial  &&
           a.fileIndexLow  == b.fileIndexLow  &&
           a.fileIndexHigh == b.fileIndexHigh;
}

/*
 * htRehash - Grow the table to `newCapacity` buckets and reinsert all
 * live keys. Called automatically when the load factor exceeds the
 * threshold, keeping expected lookup time at O(1).
 *
 * Returns 1 on success, 0 if memory allocation fails (the original
 * table is left intact in that case).
 */
static int htRehash(HashTable *ht, size_t newCapacity) {
    FolderID  *newKeys;
    SlotState *newStates;
    size_t     i;

    newKeys   = (FolderID  *)calloc(newCapacity, sizeof(FolderID));
    newStates = (SlotState *)calloc(newCapacity, sizeof(SlotState));
    if (newKeys == NULL || newStates == NULL) {
        free(newKeys);
        free(newStates);
        return 0;
    }

    /* Reinsert every live key from the old table into the new one. */
    for (i = 0; i < ht->capacity; i++) {
        if (ht->states[i] == HT_OCCUPIED) {
            size_t slot = folderIdHash(ht->keys[i], newCapacity);

            /* Linear probe until an empty slot is found. */
            while (newStates[slot] == HT_OCCUPIED) {
                slot = (slot + 1) % newCapacity;
            }
            newKeys[slot]   = ht->keys[i];
            newStates[slot] = HT_OCCUPIED;
        }
    }

    free(ht->keys);
    free(ht->states);
    ht->keys     = newKeys;
    ht->states   = newStates;
    ht->capacity = newCapacity;
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Public API implementation                                           */
/* ------------------------------------------------------------------ */

int htInit(HashTable *ht, size_t buckets) {
    ht->keys   = (FolderID  *)calloc(buckets, sizeof(FolderID));
    ht->states = (SlotState *)calloc(buckets, sizeof(SlotState));

    if (ht->keys == NULL || ht->states == NULL) {
        free(ht->keys);
        free(ht->states);
        ht->keys     = NULL;
        ht->states   = NULL;
        ht->capacity = 0;
        ht->count    = 0;
        return 0;
    }

    ht->capacity = buckets;
    ht->count    = 0;
    return 1;
}

void htFree(HashTable *ht) {
    free(ht->keys);
    free(ht->states);
    ht->keys     = NULL;
    ht->states   = NULL;
    ht->capacity = 0;
    ht->count    = 0;
}

int htInsert(HashTable *ht, FolderID id) {
    size_t slot;

    /* Rehash before inserting if we are near the load-factor limit. */
    if (ht->capacity > 0 &&
        (double)ht->count / (double)ht->capacity >= HT_MAX_LOAD_FACTOR) {
        /* Grow to roughly twice the current capacity. */
        size_t newCapacity = ht->capacity * 2 + 1;
        if (!htRehash(ht, newCapacity)) {
            return 0; /* Out of memory; insertion fails gracefully. */
        }
    }

    if (ht->capacity == 0) {
        return 0;
    }

    slot = folderIdHash(id, ht->capacity);

    /* Linear probing: advance until an empty or deleted slot is found. */
    while (ht->states[slot] == HT_OCCUPIED) {
        /* If the key already exists, do nothing (set semantics). */
        if (folderIdEqual(ht->keys[slot], id)) {
            return 1;
        }
        slot = (slot + 1) % ht->capacity;
    }

    ht->keys[slot]   = id;
    ht->states[slot] = HT_OCCUPIED;
    ht->count++;
    return 1;
}

int htContains(const HashTable *ht, FolderID id) {
    size_t slot;
    size_t probes;

    if (ht->capacity == 0) {
        return 0;
    }

    slot   = folderIdHash(id, ht->capacity);
    probes = 0;

    /* Walk the probe sequence until we find the key or an empty slot. */
    while (ht->states[slot] != HT_EMPTY && probes < ht->capacity) {
        if (ht->states[slot] == HT_OCCUPIED &&
            folderIdEqual(ht->keys[slot], id)) {
            return 1; /* Found. */
        }
        slot = (slot + 1) % ht->capacity;
        probes++;
    }

    return 0; /* Not found. */
}

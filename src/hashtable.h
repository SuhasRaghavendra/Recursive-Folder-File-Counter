/*
 * hashtable.h - Hash Table for Cycle Detection in Directory Graphs
 * -----------------------------------------------------------------
 * Implements an open-addressing hash table with linear probing,
 * keyed on Windows filesystem object identifiers (volume serial number
 * + file index). Used to detect and prevent infinite traversal loops
 * caused by directory junctions and symbolic links (reparse points).
 *
 * DAA Relevance — Graph Theory / Cycle Detection:
 *   A plain directory tree is acyclic (a DAG). However, Windows allows
 *   directory junctions and symbolic links that can create cycles:
 *
 *       C:\Users\Admin\AppData\Local\Application Data
 *                         ^--- points back to AppData\Local
 *
 *   Without cycle detection, any DFS/BFS traversal will loop indefinitely.
 *   The standard algorithmic solution is to track "visited" nodes using
 *   a hash-based visited set, checking membership before descending into
 *   any subdirectory.
 *
 * Why Hardware File IDs?
 *   Path strings are unreliable for detecting cycles because a junction
 *   presents a different path string than the target it points to. The
 *   kernel assigns each filesystem object a unique (volume serial number,
 *   file index) pair that remains constant regardless of path aliases,
 *   making it the canonical identity for cycle detection.
 *
 * Hash Table Design:
 *   - Open addressing with linear probing for simplicity and cache
 *     efficiency (no pointer chasing).
 *   - Table size is a prime number (HT_DEFAULT_BUCKETS = 65537) to
 *     minimise clustering from the hash function.
 *   - Load factor is capped at 0.7 via automatic rehashing.
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/*  FolderID - Unique hardware identity of one filesystem object.      */
/* ------------------------------------------------------------------ */

/*
 * FolderID wraps the two fields from Win32's BY_HANDLE_FILE_INFORMATION
 * that together uniquely identify a file or directory on a volume:
 *
 *   volumeSerial - Identifies which physical volume the object lives on.
 *   fileIndexLow / fileIndexHigh - The Master File Table (MFT) record
 *                  number on NTFS, or equivalent on other filesystems.
 *                  Together they form a 64-bit unique file ID.
 *
 * Two directory handles refer to the same object if and only if all
 * three fields match.
 */
typedef struct {
    unsigned long  volumeSerial;
    unsigned long  fileIndexLow;
    unsigned long  fileIndexHigh;
} FolderID;

/* ------------------------------------------------------------------ */
/*  HashTable - Open-addressing table for visited FolderID records.    */
/* ------------------------------------------------------------------ */

/*
 * Slot states (stored in the `used` array):
 *   HT_EMPTY    - Slot has never been written to.
 *   HT_OCCUPIED - Slot holds a live FolderID.
 *   HT_DELETED  - Slot held a key that was removed (tombstone).
 *                 Tombstones are required for correct linear-probe
 *                 lookup after deletions (we do not delete in this
 *                 application, but the state is defined for completeness).
 */
typedef enum {
    HT_EMPTY    = 0,
    HT_OCCUPIED = 1,
    HT_DELETED  = 2
} SlotState;

/*
 * HashTable holds:
 *   keys     - Array of FolderID values (one per bucket).
 *   states   - Parallel array of SlotState values.
 *   capacity - Total number of buckets.
 *   count    - Number of occupied (live) entries.
 */
typedef struct {
    FolderID  *keys;
    SlotState *states;
    size_t     capacity;
    size_t     count;
} HashTable;

/* Default number of buckets. 65537 is prime, giving a uniform spread. */
#define HT_DEFAULT_BUCKETS 65537

/* Load factor threshold above which the table is rehashed (70%). */
#define HT_MAX_LOAD_FACTOR 0.70

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/* Allocate and initialise a hash table with `buckets` slots.
 * Returns 1 on success, 0 if memory allocation fails.                */
int htInit(HashTable *ht, size_t buckets);

/* Free all memory owned by the hash table.                            */
void htFree(HashTable *ht);

/*
 * htInsert - Insert a FolderID into the table.
 *
 * Triggers an internal rehash if the load factor exceeds
 * HT_MAX_LOAD_FACTOR before inserting. Returns 1 on success,
 * 0 if memory allocation during rehash fails.
 */
int htInsert(HashTable *ht, FolderID id);

/*
 * htContains - Check whether a FolderID is already in the table.
 *
 * Returns 1 if the id was previously inserted, 0 otherwise.
 * Used before descending into a directory: if the ID is already
 * present, the directory has been visited and descending would
 * create a cycle.
 */
int htContains(const HashTable *ht, FolderID id);

#endif /* HASHTABLE_H */

/*
 * scanner.h - Core Scanning Types and Algorithm Declarations
 * -----------------------------------------------------------
 * Defines the shared data types used across all traversal algorithms
 * (Recursive DFS, Iterative DFS, BFS) and declares the public functions
 * for running scans, comparing results, and printing output.
 *
 * Central Types:
 *   ScanStats      - All metrics collected during a single algorithm run.
 *   ScanOptions    - Configuration flags passed into every scan.
 *   PathNode       - A directory path + depth level; the element stored
 *                    inside the Stack and Queue data structures.
 *   ComparisonResult - Aggregates the ScanStats of all three algorithms
 *                      for side-by-side comparison.
 */

#ifndef SCANNER_H
#define SCANNER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * MAX_PATH_BUFFER - Maximum directory path length supported.
 *
 * The classic Win32 MAX_PATH limit is 260 characters, which is easily
 * exceeded on deep or long-named directory trees. We use 32,768 (2^15),
 * the maximum length supported by the Win32 extended-length path prefix
 * (\\?\), to handle arbitrarily deep hierarchies safely.
 */
#define MAX_PATH_BUFFER 32768

/* ------------------------------------------------------------------ */
/*  GrowthPolicy - forward-declared here so ScanOptions can use it.   */
/*  Full definition lives in stack.h.                                  */
/* ------------------------------------------------------------------ */
typedef enum {
    GROWTH_DOUBLE = 0,
    GROWTH_LINEAR = 1
} GrowthPolicy;

/* ------------------------------------------------------------------ */
/*  ScanStats - All metrics produced by a single traversal run.        */
/* ------------------------------------------------------------------ */

/*
 * ScanStats collects every measurable quantity for one algorithm pass.
 *
 * Fields added for DAA analysis:
 *
 *   peakMemoryNodes - The maximum number of PathNode elements that
 *                     existed simultaneously in the Stack/Queue during
 *                     the run. Empirically validates:
 *                       DFS: O(h) — proportional to max directory depth.
 *                       BFS: O(w) — proportional to max directory width.
 *
 *   reallocCount    - How many times the backing array of the Stack or
 *                     Queue was resized (realloc called). Provides the
 *                     raw data for amortized-analysis comparison:
 *                       GROWTH_DOUBLE → O(log N) resizes.
 *                       GROWTH_LINEAR → O(N)     resizes.
 *
 *   timePerNode     - Execution time divided by total items scanned
 *                     (in nanoseconds). Isolates the constant factor c
 *                     hidden inside the O(N) notation, so that the
 *                     per-item overhead of recursion vs. heap structures
 *                     can be compared directly.
 *
 *   cyclesDetected  - Number of directory cycles detected and skipped
 *                     by the hash-table based cycle detection (Feature 4).
 *                     Only populated when ScanOptions.useCycleDetection
 *                     is enabled.
 */
typedef struct {
    unsigned long long fileCount;       /* Total regular files found.         */
    unsigned long long folderCount;     /* Total subdirectories found.        */
    unsigned long long totalItems;      /* fileCount + folderCount.           */
    unsigned long long skippedFolders;  /* Reparse points skipped.            */
    unsigned long long accessErrors;    /* Directories that could not be read.*/
    unsigned int       maxDepth;        /* Deepest directory level reached.   */
    double             timeTaken;       /* Wall-clock seconds for this run.   */

    /* --- DAA Extension Fields --- */
    size_t             peakMemoryNodes; /* Peak Stack/Queue occupancy (nodes).*/
    size_t             reallocCount;    /* Number of array resize operations. */
    double             timePerNode;     /* Nanoseconds per item scanned.      */
    unsigned long long cyclesDetected;  /* Circular links detected (hash tbl).*/
} ScanStats;

/* ------------------------------------------------------------------ */
/*  PathNode - One entry in the Stack or Queue.                        */
/* ------------------------------------------------------------------ */

/*
 * PathNode represents a directory that still needs to be scanned.
 * Both the Iterative DFS Stack and the BFS Queue store PathNode items.
 *
 * Fields:
 *   path  - Absolute path of the directory (up to MAX_PATH_BUFFER chars).
 *   depth - Nesting level from the scan root (root = 0). Used to track
 *           maxDepth and for display during progress reporting.
 */
typedef struct {
    char         path[MAX_PATH_BUFFER];
    unsigned int depth;
} PathNode;

/* ------------------------------------------------------------------ */
/*  ScanOptions - Runtime configuration flags.                         */
/* ------------------------------------------------------------------ */

/*
 * ScanOptions is passed to every algorithm. Adding new options here
 * automatically makes them available to all three traversal functions
 * without changing their signatures.
 *
 * Fields:
 *   showProgress      - Print live counters to stdout while scanning.
 *   skipReparsePoints - Skip Windows directory junctions and symlinks
 *                       to avoid traversing circular filesystem graphs.
 *   useCycleDetection - Enable hash-table based cycle detection that
 *                       identifies and counts genuine circular links
 *                       (Feature 4: Graph Theory / Cycle Detection).
 *   growthPolicy      - Controls Stack/Queue resize strategy
 *                       (Feature 2: Amortized Analysis).
 */
typedef struct {
    int          showProgress;
    int          skipReparsePoints;
    int          useCycleDetection;
    GrowthPolicy growthPolicy;
} ScanOptions;

/* ------------------------------------------------------------------ */
/*  ComparisonResult - Bundles stats for all three algorithms.         */
/* ------------------------------------------------------------------ */

/*
 * ComparisonResult holds one ScanStats record for each of the three
 * traversal algorithms. It is returned by runComparison() and passed to
 * printComparisonTable() and printJsonResult().
 */
typedef struct {
    ScanStats recursiveDfs;
    ScanStats iterativeDfs;
    ScanStats bfs;
} ComparisonResult;

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

/* Zero-initialise a ScanStats structure.                              */
void initStats(ScanStats *stats);

/* Recursive DFS implementation. Called recursively; not safe against
 * extreme depths due to call-stack size limits.                       */
void scanRecursiveDFS(const char *path, unsigned int depth,
                      ScanStats *stats, const ScanOptions *options);

/* Timed wrapper around scanRecursiveDFS. Returns populated ScanStats. */
ScanStats runRecursiveDFS(const char *path, const ScanOptions *options);

/* Iterative DFS using an explicit heap-allocated Stack.
 * Safe against call-stack overflow on any directory depth.            */
ScanStats runIterativeDFS(const char *path, const ScanOptions *options);

/* BFS using an explicit heap-allocated circular Queue.
 * Explores directory tree level-by-level.                             */
ScanStats runBFS(const char *path, const ScanOptions *options);

/* Run all three algorithms on targetPath and return combined results. */
ComparisonResult runComparison(const char *targetPath,
                               const ScanOptions *options);

/* Print a formatted ASCII comparison table for the three algorithms.  */
void printComparisonTable(const char *targetPath,
                          const ComparisonResult *result);

/* Enumerate all fixed drives, scan each, and print a combined summary.*/
void scanAllFixedDrives(const ScanOptions *options);

#endif /* SCANNER_H */

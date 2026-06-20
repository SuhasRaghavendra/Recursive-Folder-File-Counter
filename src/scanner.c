/*
 * scanner.c - Core Traversal Algorithms (Recursive DFS, Iterative DFS, BFS)
 * --------------------------------------------------------------------------
 * Implements all three directory-traversal algorithms and the benchmarking
 * harnesses that wrap them. This is the algorithmic heart of the project.
 *
 * Algorithm Summary:
 *
 *   1. Recursive DFS   - Uses the call stack implicitly. Simple and clean,
 *                        but risks stack overflow on deeply nested paths.
 *                        Time:  O(N)   Space: O(h) on call stack.
 *
 *   2. Iterative DFS   - Uses an explicit heap-allocated Stack (stack.c).
 *                        Eliminates overflow risk; identical traversal order.
 *                        Time:  O(N)   Space: O(h) on heap.
 *
 *   3. BFS             - Uses an explicit heap-allocated Queue (queue.c).
 *                        Explores directories level by level.
 *                        Time:  O(N)   Space: O(w) on heap (w = max width).
 *
 * DAA Features Integrated:
 *   - Peak memory tracking  (peakMemoryNodes, reallocCount from Stack/Queue).
 *   - Time-per-node metric  (timePerNode = timeTaken / totalItems * 1e9 ns).
 *   - Cycle detection       (hash table of FolderID visited set, Feature 4).
 *   - Growth policy support (passed through ScanOptions to Stack/Queue).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashtable.h"
#include "queue.h"
#include "scanner.h"
#include "stack.h"
#include "utils.h"

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/* Update the recorded maximum depth if `depth` is deeper. */
static void updateDepth(ScanStats *stats, unsigned int depth) {
    if (depth > stats->maxDepth) {
        stats->maxDepth = depth;
    }
}

/* Return 1 for the pseudo-entries "." and ".." which must always be skipped. */
static int shouldSkipEntry(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/* Optionally print a live progress line to stdout. The \r keeps it on
 * one line so the terminal output stays tidy during long scans.       */
static void maybePrintProgress(const char *algorithm,
                               const ScanStats *stats,
                               const ScanOptions *options) {
    if (!options->showProgress) {
        return;
    }
    printf("\r%-14s  files=%llu  folders=%llu  errors=%llu",
           algorithm,
           stats->fileCount,
           stats->folderCount,
           stats->accessErrors);
}

/*
 * getFolderID - Retrieve the hardware identity of a directory.
 *
 * Opens the directory, reads its BY_HANDLE_FILE_INFORMATION, and fills
 * a FolderID struct. Returns 1 on success, 0 if the directory cannot
 * be opened (permission denied, etc.).
 *
 * Used by cycle detection: two paths sharing the same FolderID are
 * the same filesystem object, so one is a cycle-creating alias.
 */
static int getFolderID(const char *path, FolderID *id) {
    HANDLE hDir;
    BY_HANDLE_FILE_INFORMATION info;

    hDir = CreateFileA(
        path,
        0,                          /* No read/write access needed.   */
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, /* Required to open directories.  */
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (!GetFileInformationByHandle(hDir, &info)) {
        CloseHandle(hDir);
        return 0;
    }

    id->volumeSerial  = info.dwVolumeSerialNumber;
    id->fileIndexLow  = info.nFileIndexLow;
    id->fileIndexHigh = info.nFileIndexHigh;

    CloseHandle(hDir);
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Public: initStats                                                   */
/* ------------------------------------------------------------------ */

void initStats(ScanStats *stats) {
    stats->fileCount       = 0;
    stats->folderCount     = 0;
    stats->totalItems      = 0;
    stats->skippedFolders  = 0;
    stats->accessErrors    = 0;
    stats->maxDepth        = 0;
    stats->timeTaken       = 0.0;
    stats->peakMemoryNodes = 0;
    stats->reallocCount    = 0;
    stats->timePerNode     = 0.0;
    stats->cyclesDetected  = 0;
}

/* ------------------------------------------------------------------ */
/*  Algorithm 1: Recursive DFS                                          */
/* ------------------------------------------------------------------ */

/*
 * scanRecursiveDFS - The recursive worker function.
 *
 * Note: peak stack memory cannot be tracked here because it lives on the
 * system call stack, not on the heap. peakMemoryNodes is left at 0 for
 * this algorithm to reflect that it uses implicit (call-stack) storage.
 *
 * Cycle detection via hash table is not applied to Recursive DFS because
 * the hash table itself would need to persist across recursive calls —
 * that requires passing it as a parameter and is handled by a separate
 * wrapper. Reparse-point skipping (skipReparsePoints) still applies.
 */
void scanRecursiveDFS(const char *path, unsigned int depth,
                      ScanStats *stats, const ScanOptions *options) {
    WIN32_FIND_DATAA findData;
    HANDLE           handle;
    /*
     * WHY HEAP ALLOCATION:
     *   Stack-allocated char[MAX_PATH_BUFFER] (32768 bytes) per recursive
     *   frame costs ~64KB of stack space. With the default Windows thread
     *   stack of 1MB, that limits safe recursion to ~15 levels.
     *   Heap allocation reduces each frame to just two pointers (~16 bytes),
     *   allowing safe recursion to any practical depth.
     */
    char *searchPattern = (char *)malloc(MAX_PATH_BUFFER);
    char *childPath     = (char *)malloc(MAX_PATH_BUFFER);

    if (!searchPattern || !childPath) {
        stats->accessErrors++;
        free(searchPattern);
        free(childPath);
        return;
    }

    updateDepth(stats, depth);
    maybePrintProgress("Recursive DFS", stats, options);

    if (!buildSearchPattern(path, searchPattern, MAX_PATH_BUFFER)) {
        stats->accessErrors++;
        free(searchPattern);
        free(childPath);
        return;
    }

    handle = FindFirstFileA(searchPattern, &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        stats->accessErrors++;
        free(searchPattern);
        free(childPath);
        return;
    }

    do {
        if (shouldSkipEntry(findData.cFileName)) {
            continue;
        }

        if (!joinPath(path, findData.cFileName, childPath, MAX_PATH_BUFFER)) {
            stats->accessErrors++;
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            /* Skip reparse points (junctions / symlinks) to avoid loops. */
            if (options->skipReparsePoints &&
                (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                stats->skippedFolders++;
                continue;
            }

            stats->folderCount++;
            scanRecursiveDFS(childPath, depth + 1, stats, options);
        } else {
            stats->fileCount++;
        }
    } while (FindNextFileA(handle, &findData));

    FindClose(handle);
    stats->totalItems = stats->fileCount + stats->folderCount;
    free(searchPattern);
    free(childPath);
}

ScanStats runRecursiveDFS(const char *path, const ScanOptions *options) {
    ScanStats stats;
    double    start;

    initStats(&stats);
    start = nowSeconds();

    scanRecursiveDFS(path, 0, &stats, options);

    stats.timeTaken  = nowSeconds() - start;
    stats.totalItems = stats.fileCount + stats.folderCount;
    stats.peakMemoryNodes = stats.maxDepth + (stats.totalItems > 0 ? 1 : 0);

    /* Compute time-per-node in nanoseconds (Feature 3). */
    if (stats.totalItems > 0) {
        stats.timePerNode = (stats.timeTaken / (double)stats.totalItems) * 1e9;
    }

    if (options->showProgress) {
        printf("\n");
    }
    return stats;
}

/* ------------------------------------------------------------------ */
/*  Algorithm 2: Iterative DFS                                          */
/* ------------------------------------------------------------------ */

ScanStats runIterativeDFS(const char *path, const ScanOptions *options) {
    ScanStats stats;
    Stack     stack;
    PathNode  current;
    HashTable visited;
    int       useHT = options->useCycleDetection;
    double    start;

    initStats(&stats);
    start = nowSeconds();

    /* Initialise hash table for cycle detection if enabled (Feature 4). */
    if (useHT) {
        if (!htInit(&visited, HT_DEFAULT_BUCKETS)) {
            useHT = 0; /* Fall back gracefully if allocation fails. */
        }
    }

    if (!stackInit(&stack, options->growthPolicy)) {
        stats.accessErrors++;
        if (useHT) htFree(&visited);
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    /* Push the root directory as the starting node. */
    strncpy(current.path, path, sizeof(current.path) - 1);
    current.path[sizeof(current.path) - 1] = '\0';
    current.depth = 0;

    if (!stackPush(&stack, &current)) {
        stats.accessErrors++;
        stackFree(&stack);
        if (useHT) htFree(&visited);
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    /* Mark the root as visited if cycle detection is on. */
    if (useHT) {
        FolderID rootId;
        if (getFolderID(path, &rootId)) {
            htInsert(&visited, rootId);
        }
    }

    /* Main DFS loop: pop a directory, scan its contents, push children. */
    while (!stackIsEmpty(&stack)) {
        WIN32_FIND_DATAA findData;
        HANDLE           handle;
        char             searchPattern[MAX_PATH_BUFFER];

        stackPop(&stack, &current);
        updateDepth(&stats, current.depth);
        maybePrintProgress("Iterative DFS", &stats, options);

        if (!buildSearchPattern(current.path, searchPattern,
                                sizeof(searchPattern))) {
            stats.accessErrors++;
            continue;
        }

        handle = FindFirstFileA(searchPattern, &findData);
        if (handle == INVALID_HANDLE_VALUE) {
            stats.accessErrors++;
            continue;
        }

        do {
            PathNode child;

            if (shouldSkipEntry(findData.cFileName)) {
                continue;
            }

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (options->skipReparsePoints &&
                    (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                    stats.skippedFolders++;
                    continue;
                }

                if (!joinPath(current.path, findData.cFileName,
                              child.path, sizeof(child.path))) {
                    stats.accessErrors++;
                    continue;
                }

                /* --- Cycle Detection (Feature 4) --- */
                if (useHT) {
                    FolderID fid;
                    if (getFolderID(child.path, &fid)) {
                        if (htContains(&visited, fid)) {
                            stats.cyclesDetected++;
                            continue; /* Skip this directory — it's a cycle. */
                        }
                        htInsert(&visited, fid);
                    }
                }

                child.depth = current.depth + 1;
                stats.folderCount++;

                if (!stackPush(&stack, &child)) {
                    stats.accessErrors++;
                }
            } else {
                stats.fileCount++;
            }
        } while (FindNextFileA(handle, &findData));

        FindClose(handle);
        stats.totalItems = stats.fileCount + stats.folderCount;
    }

    /* Copy peak-memory and realloc metrics from the stack (Feature 1 & 2). */
    stats.peakMemoryNodes = stack.peakSize;
    stats.reallocCount    = stack.reallocCount;

    stackFree(&stack);
    if (useHT) htFree(&visited);

    stats.timeTaken  = nowSeconds() - start;
    stats.totalItems = stats.fileCount + stats.folderCount;

    /* Compute time-per-node in nanoseconds (Feature 3). */
    if (stats.totalItems > 0) {
        stats.timePerNode = (stats.timeTaken / (double)stats.totalItems) * 1e9;
    }

    if (options->showProgress) {
        printf("\n");
    }
    return stats;
}

/* ------------------------------------------------------------------ */
/*  Algorithm 3: BFS                                                    */
/* ------------------------------------------------------------------ */

ScanStats runBFS(const char *path, const ScanOptions *options) {
    ScanStats stats;
    Queue     queue;
    PathNode  current;
    HashTable visited;
    int       useHT = options->useCycleDetection;
    double    start;

    initStats(&stats);
    start = nowSeconds();

    /* Initialise hash table for cycle detection if enabled (Feature 4). */
    if (useHT) {
        if (!htInit(&visited, HT_DEFAULT_BUCKETS)) {
            useHT = 0;
        }
    }

    if (!queueInit(&queue, options->growthPolicy)) {
        stats.accessErrors++;
        if (useHT) htFree(&visited);
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    /* Enqueue the root directory as the starting node. */
    strncpy(current.path, path, sizeof(current.path) - 1);
    current.path[sizeof(current.path) - 1] = '\0';
    current.depth = 0;

    if (!queueEnqueue(&queue, &current)) {
        stats.accessErrors++;
        queueFree(&queue);
        if (useHT) htFree(&visited);
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    if (useHT) {
        FolderID rootId;
        if (getFolderID(path, &rootId)) {
            htInsert(&visited, rootId);
        }
    }

    /* Main BFS loop: dequeue a directory, scan it, enqueue children. */
    while (!queueIsEmpty(&queue)) {
        WIN32_FIND_DATAA findData;
        HANDLE           handle;
        char             searchPattern[MAX_PATH_BUFFER];

        queueDequeue(&queue, &current);
        updateDepth(&stats, current.depth);
        maybePrintProgress("BFS", &stats, options);

        if (!buildSearchPattern(current.path, searchPattern,
                                sizeof(searchPattern))) {
            stats.accessErrors++;
            continue;
        }

        handle = FindFirstFileA(searchPattern, &findData);
        if (handle == INVALID_HANDLE_VALUE) {
            stats.accessErrors++;
            continue;
        }

        do {
            PathNode child;

            if (shouldSkipEntry(findData.cFileName)) {
                continue;
            }

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (options->skipReparsePoints &&
                    (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                    stats.skippedFolders++;
                    continue;
                }

                if (!joinPath(current.path, findData.cFileName,
                              child.path, sizeof(child.path))) {
                    stats.accessErrors++;
                    continue;
                }

                /* --- Cycle Detection (Feature 4) --- */
                if (useHT) {
                    FolderID fid;
                    if (getFolderID(child.path, &fid)) {
                        if (htContains(&visited, fid)) {
                            stats.cyclesDetected++;
                            continue;
                        }
                        htInsert(&visited, fid);
                    }
                }

                child.depth = current.depth + 1;
                stats.folderCount++;

                if (!queueEnqueue(&queue, &child)) {
                    stats.accessErrors++;
                }
            } else {
                stats.fileCount++;
            }
        } while (FindNextFileA(handle, &findData));

        FindClose(handle);
        stats.totalItems = stats.fileCount + stats.folderCount;
    }

    /* Copy peak-memory and realloc metrics from the queue (Feature 1 & 2). */
    stats.peakMemoryNodes = queue.peakSize;
    stats.reallocCount    = queue.reallocCount;

    queueFree(&queue);
    if (useHT) htFree(&visited);

    stats.timeTaken  = nowSeconds() - start;
    stats.totalItems = stats.fileCount + stats.folderCount;

    if (stats.totalItems > 0) {
        stats.timePerNode = (stats.timeTaken / (double)stats.totalItems) * 1e9;
    }

    if (options->showProgress) {
        printf("\n");
    }
    return stats;
}

/* ------------------------------------------------------------------ */
/*  Comparison runner and printer                                       */
/* ------------------------------------------------------------------ */

ComparisonResult runComparison(const char *targetPath,
                               const ScanOptions *options) {
    ComparisonResult result;

    fprintf(stderr, "\n[1/3] Running Recursive DFS...\n");
    result.recursiveDfs = runRecursiveDFS(targetPath, options);

    fprintf(stderr, "[2/3] Running Iterative DFS...\n");
    result.iterativeDfs = runIterativeDFS(targetPath, options);

    fprintf(stderr, "[3/3] Running BFS...\n");
    result.bfs          = runBFS(targetPath, options);

    return result;
}

void printComparisonTable(const char *targetPath,
                          const ComparisonResult *result) {
    int allMatch = statsMatch(&result->recursiveDfs, &result->iterativeDfs) &&
                   statsMatch(&result->recursiveDfs, &result->bfs);

    printf("\nScan target : %s\n\n", targetPath);

    /* Print the extended table header (includes new DAA columns). */
    printTableHeader();
    printStatsRow("Recursive DFS", &result->recursiveDfs);
    printStatsRow("Iterative DFS", &result->iterativeDfs);
    printStatsRow("BFS",           &result->bfs);
    printTableFooter();

    printf("\nFastest algorithm : %s\n", fastestAlgorithmName(result));

    if (allMatch) {
        printf("Result check      : All three algorithms produced matching counts.\n");
    } else {
        printf("Result check      : WARNING - results did not match!\n");
    }

    printf("\nNote: OS caches directory metadata after the first scan.\n");
    printf("      Subsequent runs on the same path are significantly faster.\n");
    printf("      Run on a freshly booted machine for cold-cache timings.\n");
}

/* ------------------------------------------------------------------ */
/*  Multi-drive scanner                                                 */
/* ------------------------------------------------------------------ */

static void addStats(ScanStats *total, const ScanStats *part) {
    total->fileCount      += part->fileCount;
    total->folderCount    += part->folderCount;
    total->totalItems     += part->totalItems;
    total->skippedFolders += part->skippedFolders;
    total->accessErrors   += part->accessErrors;
    total->cyclesDetected += part->cyclesDetected;
    total->timeTaken      += part->timeTaken;
    if (part->maxDepth > total->maxDepth) {
        total->maxDepth = part->maxDepth;
    }
    /* Peak memory is per-run, so we take the maximum across drives. */
    if (part->peakMemoryNodes > total->peakMemoryNodes) {
        total->peakMemoryNodes = part->peakMemoryNodes;
    }
    total->reallocCount += part->reallocCount;
}

void scanAllFixedDrives(const ScanOptions *options) {
    DWORD            drives = GetLogicalDrives();
    char             drivePath[] = "A:\\";
    ComparisonResult total;
    int              foundDrive = 0;
    int              i;

    initStats(&total.recursiveDfs);
    initStats(&total.iterativeDfs);
    initStats(&total.bfs);

    for (i = 0; i < 26; i++) {
        if (!(drives & (1u << i))) {
            continue;
        }

        drivePath[0] = (char)('A' + i);
        if (GetDriveTypeA(drivePath) != DRIVE_FIXED) {
            continue;
        }

        foundDrive = 1;
        printf("\n================ Drive %s ================\n", drivePath);
        {
            ComparisonResult result = runComparison(drivePath, options);
            printComparisonTable(drivePath, &result);
            addStats(&total.recursiveDfs, &result.recursiveDfs);
            addStats(&total.iterativeDfs, &result.iterativeDfs);
            addStats(&total.bfs,          &result.bfs);
        }
    }

    if (!foundDrive) {
        printf("No fixed drives were found.\n");
        return;
    }

    /* Recompute aggregate timePerNode across all drives. */
    if (total.recursiveDfs.totalItems > 0) {
        total.recursiveDfs.timePerNode =
            (total.recursiveDfs.timeTaken / (double)total.recursiveDfs.totalItems) * 1e9;
    }
    if (total.iterativeDfs.totalItems > 0) {
        total.iterativeDfs.timePerNode =
            (total.iterativeDfs.timeTaken / (double)total.iterativeDfs.totalItems) * 1e9;
    }
    if (total.bfs.totalItems > 0) {
        total.bfs.timePerNode =
            (total.bfs.timeTaken / (double)total.bfs.totalItems) * 1e9;
    }

    printf("\n================ Whole Computer Summary ================\n");
    printComparisonTable("All accessible fixed drives", &total);
}

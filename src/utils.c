/*
 * utils.c - Utility and Display Helper Implementations
 * -----------------------------------------------------
 * Path manipulation, high-resolution timing, and terminal table
 * formatting. All output formatting for the human-readable mode lives
 * here; JSON output lives in output.c.
 */

#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "utils.h"

/* ------------------------------------------------------------------ */
/*  String helpers                                                      */
/* ------------------------------------------------------------------ */

void trimNewline(char *text) {
    size_t length = strlen(text);
    while (length > 0 &&
           (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

void removeTrailingSlash(char *path) {
    size_t length = strlen(path);
    /* Keep at least 3 chars so "C:\" is not truncated to "C:". */
    while (length > 3 &&
           (path[length - 1] == '\\' || path[length - 1] == '/')) {
        path[length - 1] = '\0';
        length--;
    }
}

int isValidDirectory(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

/* ------------------------------------------------------------------ */
/*  Path helpers                                                        */
/* ------------------------------------------------------------------ */

int joinPath(const char *parent, const char *child,
             char *output, size_t outputSize) {
    size_t parentLength = strlen(parent);
    int    needsSlash   = parentLength > 0 &&
                          parent[parentLength - 1] != '\\' &&
                          parent[parentLength - 1] != '/';
    int written = snprintf(output, outputSize,
                           needsSlash ? "%s\\%s" : "%s%s",
                           parent, child);
    return written > 0 && (size_t)written < outputSize;
}

int buildSearchPattern(const char *path, char *output, size_t outputSize) {
    size_t pathLength = strlen(path);
    int    needsSlash = pathLength > 0 &&
                        path[pathLength - 1] != '\\' &&
                        path[pathLength - 1] != '/';
    int written = snprintf(output, outputSize,
                           needsSlash ? "%s\\*" : "%s*",
                           path);
    return written > 0 && (size_t)written < outputSize;
}

/* ------------------------------------------------------------------ */
/*  High-resolution timer                                               */
/* ------------------------------------------------------------------ */

/*
 * nowSeconds - Return the current wall-clock time in seconds.
 *
 * Uses QueryPerformanceCounter (QPC) which has nanosecond-level
 * resolution on modern Windows systems. This replaces the older
 * clock() / CLOCKS_PER_SEC approach which only has millisecond
 * resolution and measures CPU time rather than wall-clock time.
 */
double nowSeconds(void) {
    LARGE_INTEGER counter, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
}

/* ------------------------------------------------------------------ */
/*  Table printing                                                      */
/* ------------------------------------------------------------------ */

/*
 * The table displays all original columns plus the three new DAA columns:
 *   Peak Nodes   - Peak Stack/Queue occupancy (space complexity measure).
 *   Reallocs     - Number of backing-array resize operations.
 *   Time/Node ns - Execution time per scanned item in nanoseconds
 *                  (constant-factor c in O(c*N)).
 *
 * Column widths are chosen so the table fits in a standard 132-character
 * terminal without wrapping.
 */

#define TABLE_BORDER \
"+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+------------+----------+---------------+\n"

#define TABLE_HEADER \
"| Algorithm      | Files      | Folders     | Total Items | Max Depth | Skipped | Access Errors | Time (sec) | Peak Nodes | Reallocs | Time/Node(ns) |\n"

void printTableHeader(void) {
    printf(TABLE_BORDER);
    printf(TABLE_HEADER);
    printf(TABLE_BORDER);
}

void printStatsRow(const char *algorithmName, const ScanStats *stats) {
    printf("| %-14s | %10llu | %11llu | %11llu | %9u | %7llu | %13llu | %10.4f | %10zu | %8zu | %13.2f |\n",
           algorithmName,
           stats->fileCount,
           stats->folderCount,
           stats->totalItems,
           stats->maxDepth,
           stats->skippedFolders,
           stats->accessErrors,
           stats->timeTaken,
           stats->peakMemoryNodes,
           stats->reallocCount,
           stats->timePerNode);
}

void printTableFooter(void) {
    printf(TABLE_BORDER);
}

/* ------------------------------------------------------------------ */
/*  Result analysis                                                     */
/* ------------------------------------------------------------------ */

int statsMatch(const ScanStats *a, const ScanStats *b) {
    return a->fileCount   == b->fileCount   &&
           a->folderCount == b->folderCount &&
           a->totalItems  == b->totalItems  &&
           a->maxDepth    == b->maxDepth;
}

const char *fastestAlgorithmName(const ComparisonResult *result) {
    double recursive = result->recursiveDfs.timeTaken;
    double iterative = result->iterativeDfs.timeTaken;
    double bfs       = result->bfs.timeTaken;

    if (recursive <= iterative && recursive <= bfs) { return "Recursive DFS"; }
    if (iterative <= recursive && iterative <= bfs) { return "Iterative DFS"; }
    return "BFS";
}

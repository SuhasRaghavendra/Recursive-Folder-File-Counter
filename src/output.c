/*
 * output.c - JSON Serialiser for Frontend Integration
 * ----------------------------------------------------
 * Converts a ComparisonResult struct into a JSON string written to
 * stdout. This is the sole output channel when the program is run
 * with the --json flag, enabling any frontend (web, desktop, notebook)
 * to consume and visualise the benchmark data without parsing
 * human-readable text.
 *
 * Design Decisions:
 *   - No third-party JSON library is used; the output is hand-built
 *     with printf to keep the project dependency-free and portable.
 *   - All numeric values are emitted without string quoting so that
 *     JSON parsers treat them as numbers, not strings.
 *   - String fields (target, algorithm names) are JSON-escaped to
 *     handle backslashes in Windows paths correctly.
 *   - The output is a single JSON object on one line followed by a
 *     newline, making it easy for frontend code to detect the end
 *     of a scan result when reading stdout line-by-line.
 */

#include <stdio.h>
#include <string.h>

#include "output.h"
#include "utils.h"   /* fastestAlgorithmName, statsMatch */

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/*
 * printJsonEscapedString - Write a C string to stdout as a JSON string
 * literal, escaping backslashes and double-quotes so that Windows paths
 * (which contain backslashes) are valid JSON.
 */
static void printJsonEscapedString(const char *s) {
    putchar('"');
    while (*s) {
        if (*s == '\\') {
            putchar('\\'); putchar('\\');
        } else if (*s == '"') {
            putchar('\\'); putchar('"');
        } else {
            putchar(*s);
        }
        s++;
    }
    putchar('"');
}

/*
 * printJsonStats - Emit one AlgorithmStats JSON object.
 *
 * All fields from ScanStats are included so that the frontend has
 * access to every metric without needing to re-run the scanner.
 */
static void printJsonStats(const ScanStats *s) {
    printf("{");
    printf("\"fileCount\":%llu,",        s->fileCount);
    printf("\"folderCount\":%llu,",      s->folderCount);
    printf("\"totalItems\":%llu,",       s->totalItems);
    printf("\"maxDepth\":%u,",           s->maxDepth);
    printf("\"skippedFolders\":%llu,",   s->skippedFolders);
    printf("\"accessErrors\":%llu,",     s->accessErrors);
    printf("\"timeTaken\":%.6f,",        s->timeTaken);
    printf("\"peakMemoryNodes\":%zu,",   s->peakMemoryNodes);
    printf("\"reallocCount\":%zu,",      s->reallocCount);
    printf("\"timePerNodeNs\":%.4f,",    s->timePerNode);
    printf("\"cyclesDetected\":%llu",    s->cyclesDetected);
    printf("}");
}

/* ------------------------------------------------------------------ */
/*  Public API                                                          */
/* ------------------------------------------------------------------ */

void printJsonResult(const char *targetPath,
                     const ComparisonResult *result,
                     const ScanOptions *options) {
    int countsMatch;

    countsMatch = statsMatch(&result->recursiveDfs, &result->iterativeDfs) &&
                  statsMatch(&result->recursiveDfs, &result->bfs);

    printf("{");

    /* Target path (JSON-escaped for Windows backslashes). */
    printf("\"target\":");
    printJsonEscapedString(targetPath);
    printf(",");

    /* Growth policy used for this run. */
    printf("\"growthPolicy\":\"%s\",",
           options->growthPolicy == GROWTH_DOUBLE ? "double" : "linear");

    /* Cycle detection flag. */
    printf("\"cycleDetectionEnabled\":%s,",
           options->useCycleDetection ? "true" : "false");

    /* Per-algorithm stats. */
    printf("\"algorithms\":{");
    printf("\"recursiveDfs\":"); printJsonStats(&result->recursiveDfs); printf(",");
    printf("\"iterativeDfs\":"); printJsonStats(&result->iterativeDfs); printf(",");
    printf("\"bfs\":");          printJsonStats(&result->bfs);
    printf("},");

    /* Summary fields. */
    printf("\"fastest\":");
    printJsonEscapedString(fastestAlgorithmName(result));
    printf(",");

    printf("\"countsMatch\":%s", countsMatch ? "true" : "false");

    printf("}\n");
    fflush(stdout); /* Ensure the frontend receives the output immediately. */
}

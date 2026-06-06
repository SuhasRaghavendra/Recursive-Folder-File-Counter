#include <stdio.h>
#include <string.h>

#include "queue.h"
#include "scanner.h"
#include "stack.h"
#include "utils.h"

static void updateDepth(ScanStats *stats, unsigned int depth) {
    if (depth > stats->maxDepth) {
        stats->maxDepth = depth;
    }
}

void initStats(ScanStats *stats) {
    stats->fileCount = 0;
    stats->folderCount = 0;
    stats->totalItems = 0;
    stats->skippedFolders = 0;
    stats->accessErrors = 0;
    stats->maxDepth = 0;
    stats->timeTaken = 0.0;
}

static int shouldSkipEntry(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static void maybePrintProgress(const char *algorithm, const char *path, const ScanStats *stats, const ScanOptions *options) {
    if (!options->showProgress) {
        return;
    }

    printf("\r%-14s scanning: files=%I64u folders=%I64u errors=%I64u",
           algorithm,
           stats->fileCount,
           stats->folderCount,
           stats->accessErrors);
    (void)path;
}

void scanRecursiveDFS(const char *path, unsigned int depth, ScanStats *stats, const ScanOptions *options) {
    WIN32_FIND_DATAA findData;
    HANDLE handle;
    char searchPattern[MAX_PATH_BUFFER];
    char childPath[MAX_PATH_BUFFER];

    updateDepth(stats, depth);
    maybePrintProgress("Recursive DFS", path, stats, options);

    if (!buildSearchPattern(path, searchPattern, sizeof(searchPattern))) {
        stats->accessErrors++;
        return;
    }

    handle = FindFirstFileA(searchPattern, &findData);
    if (handle == INVALID_HANDLE_VALUE) {
        stats->accessErrors++;
        return;
    }

    do {
        if (shouldSkipEntry(findData.cFileName)) {
            continue;
        }

        if (!joinPath(path, findData.cFileName, childPath, sizeof(childPath))) {
            stats->accessErrors++;
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (options->skipReparsePoints && (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
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
}

ScanStats runRecursiveDFS(const char *path, const ScanOptions *options) {
    ScanStats stats;
    double start;

    initStats(&stats);
    start = nowSeconds();
    scanRecursiveDFS(path, 0, &stats, options);
    stats.timeTaken = nowSeconds() - start;
    stats.totalItems = stats.fileCount + stats.folderCount;
    if (options->showProgress) {
        printf("\n");
    }
    return stats;
}

ScanStats runIterativeDFS(const char *path, const ScanOptions *options) {
    ScanStats stats;
    Stack stack;
    PathNode current;
    double start;

    initStats(&stats);
    start = nowSeconds();

    if (!stackInit(&stack)) {
        stats.accessErrors++;
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    strncpy(current.path, path, sizeof(current.path) - 1);
    current.path[sizeof(current.path) - 1] = '\0';
    current.depth = 0;

    if (!stackPush(&stack, &current)) {
        stats.accessErrors++;
        stackFree(&stack);
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    while (!stackIsEmpty(&stack)) {
        WIN32_FIND_DATAA findData;
        HANDLE handle;
        char searchPattern[MAX_PATH_BUFFER];

        stackPop(&stack, &current);
        updateDepth(&stats, current.depth);
        maybePrintProgress("Iterative DFS", current.path, &stats, options);

        if (!buildSearchPattern(current.path, searchPattern, sizeof(searchPattern))) {
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
                if (options->skipReparsePoints && (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                    stats.skippedFolders++;
                    continue;
                }

                if (!joinPath(current.path, findData.cFileName, child.path, sizeof(child.path))) {
                    stats.accessErrors++;
                    continue;
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

    stackFree(&stack);
    stats.timeTaken = nowSeconds() - start;
    stats.totalItems = stats.fileCount + stats.folderCount;
    if (options->showProgress) {
        printf("\n");
    }
    return stats;
}

ScanStats runBFS(const char *path, const ScanOptions *options) {
    ScanStats stats;
    Queue queue;
    PathNode current;
    double start;

    initStats(&stats);
    start = nowSeconds();

    if (!queueInit(&queue)) {
        stats.accessErrors++;
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    strncpy(current.path, path, sizeof(current.path) - 1);
    current.path[sizeof(current.path) - 1] = '\0';
    current.depth = 0;

    if (!queueEnqueue(&queue, &current)) {
        stats.accessErrors++;
        queueFree(&queue);
        stats.timeTaken = nowSeconds() - start;
        return stats;
    }

    while (!queueIsEmpty(&queue)) {
        WIN32_FIND_DATAA findData;
        HANDLE handle;
        char searchPattern[MAX_PATH_BUFFER];

        queueDequeue(&queue, &current);
        updateDepth(&stats, current.depth);
        maybePrintProgress("BFS", current.path, &stats, options);

        if (!buildSearchPattern(current.path, searchPattern, sizeof(searchPattern))) {
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
                if (options->skipReparsePoints && (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                    stats.skippedFolders++;
                    continue;
                }

                if (!joinPath(current.path, findData.cFileName, child.path, sizeof(child.path))) {
                    stats.accessErrors++;
                    continue;
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

    queueFree(&queue);
    stats.timeTaken = nowSeconds() - start;
    stats.totalItems = stats.fileCount + stats.folderCount;
    if (options->showProgress) {
        printf("\n");
    }
    return stats;
}

ComparisonResult runComparison(const char *targetPath, const ScanOptions *options) {
    ComparisonResult result;

    printf("\nRunning Recursive DFS...\n");
    result.recursiveDfs = runRecursiveDFS(targetPath, options);

    printf("Running Iterative DFS...\n");
    result.iterativeDfs = runIterativeDFS(targetPath, options);

    printf("Running BFS...\n");
    result.bfs = runBFS(targetPath, options);

    return result;
}

void printComparisonTable(const char *targetPath, const ComparisonResult *result) {
    int recursiveMatchesIterative = statsMatch(&result->recursiveDfs, &result->iterativeDfs);
    int recursiveMatchesBfs = statsMatch(&result->recursiveDfs, &result->bfs);

    printf("\nScan target: %s\n\n", targetPath);
    printf("+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+\n");
    printf("| Algorithm      | Files      | Folders     | Total Items | Max Depth | Skipped | Access Errors | Time (sec) |\n");
    printf("+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+\n");
    printStatsRow("Recursive DFS", &result->recursiveDfs);
    printStatsRow("Iterative DFS", &result->iterativeDfs);
    printStatsRow("BFS", &result->bfs);
    printf("+----------------+------------+-------------+-------------+-----------+---------+---------------+------------+\n");
    printf("\nFastest algorithm: %s\n", fastestAlgorithmName(result));

    if (recursiveMatchesIterative && recursiveMatchesBfs) {
        printf("Result check: All algorithms produced matching counts.\n");
    } else {
        printf("Result check: WARNING - algorithm results did not match exactly.\n");
    }

    printf("Note: Timing can vary because the operating system caches directory data between runs.\n");
}

static void addStats(ScanStats *total, const ScanStats *part) {
    total->fileCount += part->fileCount;
    total->folderCount += part->folderCount;
    total->totalItems += part->totalItems;
    total->skippedFolders += part->skippedFolders;
    total->accessErrors += part->accessErrors;
    total->timeTaken += part->timeTaken;
    if (part->maxDepth > total->maxDepth) {
        total->maxDepth = part->maxDepth;
    }
}

void scanAllFixedDrives(const ScanOptions *options) {
    DWORD drives = GetLogicalDrives();
    char drivePath[] = "A:\\";
    ComparisonResult total;
    int foundDrive = 0;
    int i;

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
            addStats(&total.bfs, &result.bfs);
        }
    }

    if (!foundDrive) {
        printf("No fixed drives were found.\n");
        return;
    }

    printf("\n================ Whole Computer Summary ================\n");
    printComparisonTable("All accessible fixed drives", &total);
}

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils.h"

void trimNewline(char *text) {
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

void removeTrailingSlash(char *path) {
    size_t length = strlen(path);
    while (length > 3 && (path[length - 1] == '\\' || path[length - 1] == '/')) {
        path[length - 1] = '\0';
        length--;
    }
}

int isValidDirectory(const char *path) {
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

int joinPath(const char *parent, const char *child, char *output, size_t outputSize) {
    size_t parentLength = strlen(parent);
    int needsSlash = parentLength > 0 && parent[parentLength - 1] != '\\' && parent[parentLength - 1] != '/';
    int written = snprintf(output, outputSize, needsSlash ? "%s\\%s" : "%s%s", parent, child);
    return written > 0 && (size_t)written < outputSize;
}

int buildSearchPattern(const char *path, char *output, size_t outputSize) {
    size_t pathLength = strlen(path);
    int needsSlash = pathLength > 0 && path[pathLength - 1] != '\\' && path[pathLength - 1] != '/';
    int written = snprintf(output, outputSize, needsSlash ? "%s\\*" : "%s*", path);
    return written > 0 && (size_t)written < outputSize;
}

double nowSeconds(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

void printStatsRow(const char *algorithmName, const ScanStats *stats) {
    printf("| %-14s | %10I64u | %11I64u | %11I64u | %9u | %7I64u | %13I64u | %10.4f |\n",
           algorithmName,
           stats->fileCount,
           stats->folderCount,
           stats->totalItems,
           stats->maxDepth,
           stats->skippedFolders,
           stats->accessErrors,
           stats->timeTaken);
}

int statsMatch(const ScanStats *a, const ScanStats *b) {
    return a->fileCount == b->fileCount &&
           a->folderCount == b->folderCount &&
           a->totalItems == b->totalItems &&
           a->maxDepth == b->maxDepth;
}

const char *fastestAlgorithmName(const ComparisonResult *result) {
    double recursive = result->recursiveDfs.timeTaken;
    double iterative = result->iterativeDfs.timeTaken;
    double bfs = result->bfs.timeTaken;

    if (recursive <= iterative && recursive <= bfs) {
        return "Recursive DFS";
    }
    if (iterative <= recursive && iterative <= bfs) {
        return "Iterative DFS";
    }
    return "BFS";
}

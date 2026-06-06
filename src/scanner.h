#ifndef SCANNER_H
#define SCANNER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_PATH_BUFFER 32768

typedef struct {
    unsigned long long fileCount;
    unsigned long long folderCount;
    unsigned long long totalItems;
    unsigned long long skippedFolders;
    unsigned long long accessErrors;
    unsigned int maxDepth;
    double timeTaken;
} ScanStats;

typedef struct {
    int showProgress;
    int skipReparsePoints;
} ScanOptions;

typedef struct {
    char path[MAX_PATH_BUFFER];
    unsigned int depth;
} PathNode;

typedef struct {
    ScanStats recursiveDfs;
    ScanStats iterativeDfs;
    ScanStats bfs;
} ComparisonResult;

void initStats(ScanStats *stats);
void scanRecursiveDFS(const char *path, unsigned int depth, ScanStats *stats, const ScanOptions *options);
ScanStats runRecursiveDFS(const char *path, const ScanOptions *options);
ScanStats runIterativeDFS(const char *path, const ScanOptions *options);
ScanStats runBFS(const char *path, const ScanOptions *options);
ComparisonResult runComparison(const char *targetPath, const ScanOptions *options);
void printComparisonTable(const char *targetPath, const ComparisonResult *result);
void scanAllFixedDrives(const ScanOptions *options);

#endif

#ifndef UTILS_H
#define UTILS_H

#include "scanner.h"

void trimNewline(char *text);
void removeTrailingSlash(char *path);
int isValidDirectory(const char *path);
int joinPath(const char *parent, const char *child, char *output, size_t outputSize);
int buildSearchPattern(const char *path, char *output, size_t outputSize);
double nowSeconds(void);
void printStatsRow(const char *algorithmName, const ScanStats *stats);
int statsMatch(const ScanStats *a, const ScanStats *b);
const char *fastestAlgorithmName(const ComparisonResult *result);

#endif

/*
 * utils.h - Utility and Display Helpers
 * --------------------------------------
 * Provides string-manipulation helpers for path handling, a high-
 * resolution timer, and all functions for formatting and printing the
 * comparison table to the terminal.
 *
 * Separating display logic here keeps scanner.c focused on algorithms
 * and makes it easy to swap the output format (e.g., switch to the
 * JSON output module in output.c) without touching the core code.
 */

#ifndef UTILS_H
#define UTILS_H

#include "scanner.h"

/* ---- String helpers ---- */

/* Remove trailing '\n' and '\r' characters from a string in-place.   */
void trimNewline(char *text);

/* Remove trailing backslash or forward-slash, except for drive roots
 * like "C:\".                                                         */
void removeTrailingSlash(char *path);

/* Return 1 if `path` names an existing, accessible directory.        */
int  isValidDirectory(const char *path);

/* ---- Path helpers ---- */

/* Join parent and child with a backslash separator into output[].
 * Returns 1 on success, 0 if the result would exceed outputSize.     */
int  joinPath(const char *parent, const char *child,
              char *output, size_t outputSize);

/* Build a FindFirstFile search pattern by appending "\*" to path.
 * Returns 1 on success, 0 if the result would exceed outputSize.     */
int  buildSearchPattern(const char *path,
                        char *output, size_t outputSize);

/* ---- Timing ---- */

/* Return the current wall-clock time in seconds with high resolution.
 * Uses QueryPerformanceCounter for sub-microsecond accuracy.         */
double nowSeconds(void);

/* ---- Table printing ---- */

/* Print the top header row of the comparison table (column names).   */
void printTableHeader(void);

/* Print one data row for an algorithm.                                */
void printStatsRow(const char *algorithmName, const ScanStats *stats);

/* Print the bottom border of the comparison table.                    */
void printTableFooter(void);

/* ---- Result analysis ---- */

/* Return 1 if a and b produced identical file/folder/item/depth counts. */
int statsMatch(const ScanStats *a, const ScanStats *b);

/* Return the name of the algorithm with the lowest timeTaken.         */
const char *fastestAlgorithmName(const ComparisonResult *result);

#endif /* UTILS_H */

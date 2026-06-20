/*
 * main.c - CLI Entry Point and User Interface
 * --------------------------------------------
 * Provides the interactive menu that lets the user choose a scan mode,
 * configure options, and view results. Also handles the --json command-
 * line flag for non-interactive frontend integration.
 *
 * Menu Options:
 *   1. Scan a specific folder.
 *   2. Scan a specific drive (e.g., C:\).
 *   3. Scan all accessible fixed drives.
 *   4. Run DAA synthetic benchmark (deep tree vs. wide tree).
 *   5. Exit.
 *
 * Command-Line Flags:
 *   --json          Output all results as JSON to stdout.
 *   --path <dir>    Scan the specified directory non-interactively
 *                   (requires --json; used by frontend integrations).
 *
 * Options Prompted Per Scan:
 *   - Show live progress  (y/n)
 *   - Enable cycle detection via hash table  (y/n)
 *   - Growth policy: double (default) or linear  (d/l)
 *     Used to demonstrate amortized analysis differences.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "output.h"
#include "scanner.h"
#include "synth.h"
#include "utils.h"

/* ------------------------------------------------------------------ */
/*  Header / menu display                                               */
/* ------------------------------------------------------------------ */

static void printHeader(void) {
    printf("=====================================================================\n");
    printf(" Recursive File Folder Counter\n");
    printf(" Comparing DFS (Recursive), DFS (Iterative), and BFS algorithms\n");
    printf(" DAA Edition: Space Complexity | Amortized | Cycle Detection\n");
    printf("=====================================================================\n");
}

static void printMenu(void) {
    printf("\n--- MAIN MENU ---\n");
    printf("  1. Scan a specific folder\n");
    printf("  2. Scan a specific drive  (e.g. C:\\)\n");
    printf("  3. Scan all accessible fixed drives\n");
    printf("  4. Run DAA synthetic benchmark  (deep tree vs. wide tree)\n");
    printf("  5. Exit\n");
    printf("\nEnter your choice: ");
}

/* ------------------------------------------------------------------ */
/*  Input helpers                                                       */
/* ------------------------------------------------------------------ */

/* Read one line from stdin into buf[size]. Returns 1 on success. */
static int readLine(char *buf, int size) {
    if (fgets(buf, size, stdin) == NULL) {
        return 0;
    }
    trimNewline(buf);
    return 1;
}

static int askYesNo(const char *prompt) {
    char input[16];
    printf("%s", prompt);
    if (!readLine(input, sizeof(input))) {
        return 0;
    }
    return input[0] == 'y' || input[0] == 'Y';
}

/*
 * askOptions - Prompt the user for all scan configuration flags.
 *
 * This is where the three new DAA options are collected:
 *   - Cycle detection (hash table, Feature 4)
 *   - Growth policy   (amortized analysis, Feature 2)
 */
static ScanOptions askOptions(void) {
    ScanOptions options;
    char        input[16];

    options.skipReparsePoints = 1; /* Always on to prevent infinite loops. */

    options.showProgress = askYesNo("Show live progress? (y/n): ");

    options.useCycleDetection = askYesNo(
        "Enable hash-table cycle detection? (y/n): ");

    printf("Growth policy for Stack/Queue — (d)ouble or (l)inear? [d]: ");
    if (!readLine(input, sizeof(input)) || input[0] == '\0' || input[0] == 'd') {
        options.growthPolicy = GROWTH_DOUBLE;
        printf("  -> Using GROWTH_DOUBLE (O(log N) resizes, amortized O(1) push)\n");
    } else {
        options.growthPolicy = GROWTH_LINEAR;
        printf("  -> Using GROWTH_LINEAR (O(N) resizes, demonstrates quadratic cost)\n");
    }

    return options;
}

/* ------------------------------------------------------------------ */
/*  Scan flows                                                          */
/* ------------------------------------------------------------------ */

static void scanFolderFlow(int jsonMode) {
    char             path[MAX_PATH_BUFFER];
    ScanOptions      options;
    ComparisonResult result;

    printf("Enter folder path: ");
    if (!readLine(path, sizeof(path))) {
        printf("Input error.\n");
        return;
    }

    removeTrailingSlash(path);

    if (!isValidDirectory(path)) {
        printf("Error: path does not exist or is not a directory.\n");
        return;
    }

    options = askOptions();
    result  = runComparison(path, &options);

    if (jsonMode) {
        printJsonResult(path, &result, &options);
    } else {
        printComparisonTable(path, &result);
    }
}

static void scanDriveFlow(int jsonMode) {
    char        input[32];
    char        drivePath[] = "C:\\";
    UINT        driveType;
    ScanOptions      options;
    ComparisonResult result;

    printf("Enter drive letter (e.g. C): ");
    if (!readLine(input, sizeof(input)) || strlen(input) == 0) {
        printf("Invalid drive letter.\n");
        return;
    }

    drivePath[0] = input[0];
    if (drivePath[0] >= 'a' && drivePath[0] <= 'z') {
        drivePath[0] = (char)(drivePath[0] - 'a' + 'A');
    }

    driveType = GetDriveTypeA(drivePath);
    if (driveType == DRIVE_NO_ROOT_DIR || driveType == DRIVE_UNKNOWN) {
        printf("Drive %s is not available.\n", drivePath);
        return;
    }

    options = askOptions();
    result  = runComparison(drivePath, &options);

    if (jsonMode) {
        printJsonResult(drivePath, &result, &options);
    } else {
        printComparisonTable(drivePath, &result);
    }
}

/* ------------------------------------------------------------------ */
/*  DAA Synthetic Benchmark (Feature 5)                                 */
/* ------------------------------------------------------------------ */

/*
 * synthBenchmarkFlow - Create extreme graph shapes and benchmark all
 * three algorithms on each.
 *
 * Two synthetic trees are created in the system's temp directory:
 *   deep_tree/ - 200 levels deep, 1 folder per level (chain graph).
 *   wide_tree/ - 1 level deep, 10,000 folders (star graph).
 *
 * After benchmarking, both trees are deleted automatically.
 *
 * Expected outcomes (which the results should empirically confirm):
 *   Deep Tree: DFS peak stack ~ 200. BFS peak queue ~ 1.
 *   Wide Tree: DFS peak stack ~ 1.   BFS peak queue ~ 10,000.
 */
static void synthBenchmarkFlow(int jsonMode) {
    char         tempBase[MAX_PATH_BUFFER];
    char         deepPath[MAX_PATH_BUFFER];
    char         widePath[MAX_PATH_BUFFER];
    DWORD        tempLen;
    ScanOptions  options;

    /* Use GROWTH_DOUBLE and no cycle detection for the benchmark so the
     * peak memory numbers purely reflect traversal structure, not overhead. */
    options.showProgress     = 0;
    options.skipReparsePoints = 1;
    options.useCycleDetection = 0;
    options.growthPolicy      = GROWTH_DOUBLE;

    /* Get the system temporary directory. */
    tempLen = GetTempPathA(sizeof(tempBase), tempBase);
    if (tempLen == 0 || tempLen >= sizeof(tempBase)) {
        printf("Error: could not determine temp directory.\n");
        return;
    }
    removeTrailingSlash(tempBase);

    /* Use a small buffer — temp paths are always short in practice.
     * MAX_PATH_BUFFER (32768) caused spurious -Wformat-truncation
     * because gcc sees the theoretical max, not the actual temp path length. */
    {
        size_t baseLen = strlen(tempBase);
        if (4 + baseLen + 16 > sizeof(deepPath)) {
            printf("Error: temp path is too long.\n");
            return;
        }
        /* Prepend Win32 extended path prefix \\?\ to bypass the MAX_PATH (260) limit,
         * allowing us to create and delete directories deeper than 34 levels. */
        memcpy(deepPath, "\\\\?\\", 4);
        memcpy(deepPath + 4, tempBase, baseLen);
        memcpy(deepPath + 4 + baseLen, "\\daa_deep_tree", 15); /* 14 chars + NUL */

        memcpy(widePath, "\\\\?\\", 4);
        memcpy(widePath + 4, tempBase, baseLen);
        memcpy(widePath + 4 + baseLen, "\\daa_wide_tree", 15);
    }

    /* --- Deep Tree Benchmark --- */
    fprintf(stderr, "\n=== SYNTHETIC BENCHMARK: DEEP TREE (200 levels deep) ===\n");
    fprintf(stderr, "Creating tree... ");
    fflush(stderr);

    /* Always create root dir; createDeepTree handles pre-existing children. */
    CreateDirectoryA(deepPath, NULL);
    createDeepTree(deepPath, 200);
    fprintf(stderr, "Done. Scanning...\n");
    {
        ComparisonResult result = runComparison(deepPath, &options);
        if (jsonMode) {
            printJsonResult(deepPath, &result, &options);
        } else {
            printComparisonTable(deepPath, &result);
            printf("\nAnalysis: DFS peak stack should be ~200 (O(h)).\n");
            printf("          BFS peak queue should be ~1   (O(w), w=1 here).\n");
        }
    }
    fprintf(stderr, "Cleaning up deep tree... ");
    cleanupSynthTree(deepPath);
    fprintf(stderr, "Done.\n");

    /* --- Wide Tree Benchmark --- */
    /* 1000 folders at root is enough to prove O(w) BFS memory growth.   */
    fprintf(stderr, "\n=== SYNTHETIC BENCHMARK: WIDE TREE (1,000 folders at root) ===\n");
    fprintf(stderr, "Creating tree... ");
    fflush(stderr);

    CreateDirectoryA(widePath, NULL);
    createWideTree(widePath, 1000);
    fprintf(stderr, "Done. Scanning...\n");
    {
        ComparisonResult result = runComparison(widePath, &options);
        if (jsonMode) {
            printJsonResult(widePath, &result, &options);
        } else {
            printComparisonTable(widePath, &result);
            printf("\nAnalysis: DFS peak stack should be ~1     (O(h), h=1 here).\n");
            printf("          BFS peak queue should be ~1,000  (O(w) = O(N)).\n");
        }
    }
    fprintf(stderr, "Cleaning up wide tree... ");
    cleanupSynthTree(widePath);
    fprintf(stderr, "Done.\n");
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    char input[32];
    int  choice;
    int  jsonMode = 0;
    int  i;

    /* ---- Parse command-line flags ---- */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            jsonMode = 1;
        }
        /* --path <dir> for non-interactive frontend use */
        if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            const char  *path = argv[i + 1];
            ScanOptions  options;
            ComparisonResult result;

            options.showProgress      = 0;
            options.skipReparsePoints = 1;
            options.useCycleDetection = 0;
            options.growthPolicy      = GROWTH_DOUBLE;

            if (!isValidDirectory(path)) {
                fprintf(stderr, "Error: '%s' is not a valid directory.\n", path);
                return 1;
            }

            result = runComparison(path, &options);

            if (jsonMode) {
                printJsonResult(path, &result, &options);
            } else {
                printComparisonTable(path, &result);
            }
            return 0;
        }
        /* --benchmark: run synthetic deep+wide trees and emit two JSON lines */
        if (strcmp(argv[i], "--benchmark") == 0) {
            /* synthBenchmarkFlow is called below after jsonMode is parsed.
             * We defer it so --json --benchmark works in any order. */
            (void)0; /* handled after the loop */
        }
    }

    /* Handle --benchmark after all flags are parsed */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--benchmark") == 0) {
            synthBenchmarkFlow(jsonMode);
            return 0;
        }
    }

    /* ---- Interactive mode ---- */
    printHeader();

    while (1) {
        printMenu();

        if (!readLine(input, sizeof(input))) {
            printf("\nInput error. Exiting.\n");
            return 1;
        }

        choice = atoi(input);

        switch (choice) {
            case 1:
                scanFolderFlow(jsonMode);
                break;

            case 2:
                scanDriveFlow(jsonMode);
                break;

            case 3: {
                ScanOptions options = askOptions();
                scanAllFixedDrives(&options);
                break;
            }

            case 4:
                synthBenchmarkFlow(jsonMode);
                break;

            case 5:
                printf("Goodbye.\n");
                return 0;

            default:
                printf("Invalid choice. Please enter 1, 2, 3, 4, or 5.\n");
                break;
        }
    }
}

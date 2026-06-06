#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "utils.h"

static void printHeader(void) {
    printf("=============================================\n");
    printf(" Recursive File Folder Counter\n");
    printf(" DFS, Iterative DFS, and BFS Comparison\n");
    printf("=============================================\n");
}

static void printMenu(void) {
    printf("\n1. Scan a specific folder\n");
    printf("2. Scan a specific drive\n");
    printf("3. Scan all accessible fixed drives\n");
    printf("4. Exit\n");
    printf("\nEnter your choice: ");
}

static int askYesNo(const char *prompt) {
    char input[16];

    printf("%s", prompt);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }

    return input[0] == 'y' || input[0] == 'Y';
}

static ScanOptions askOptions(void) {
    ScanOptions options;
    options.showProgress = askYesNo("Show live progress? (y/n): ");
    options.skipReparsePoints = 1;
    return options;
}

static void scanFolderFlow(void) {
    char path[MAX_PATH_BUFFER];
    ScanOptions options;
    ComparisonResult result;

    printf("Enter folder path: ");
    if (fgets(path, sizeof(path), stdin) == NULL) {
        printf("Input error.\n");
        return;
    }

    trimNewline(path);
    removeTrailingSlash(path);

    if (!isValidDirectory(path)) {
        printf("Invalid folder path or path is not accessible.\n");
        return;
    }

    options = askOptions();
    result = runComparison(path, &options);
    printComparisonTable(path, &result);
}

static void scanDriveFlow(void) {
    char input[32];
    char drivePath[] = "C:\\";
    UINT driveType;
    ScanOptions options;
    ComparisonResult result;

    printf("Enter drive letter, for example C: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Input error.\n");
        return;
    }

    trimNewline(input);
    if (strlen(input) == 0) {
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
    result = runComparison(drivePath, &options);
    printComparisonTable(drivePath, &result);
}

int main(void) {
    char input[32];
    int choice;

    printHeader();

    while (1) {
        printMenu();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nInput error. Exiting.\n");
            return 1;
        }

        choice = atoi(input);

        switch (choice) {
            case 1:
                scanFolderFlow();
                break;
            case 2:
                scanDriveFlow();
                break;
            case 3: {
                ScanOptions options = askOptions();
                scanAllFixedDrives(&options);
                break;
            }
            case 4:
                printf("Goodbye.\n");
                return 0;
            default:
                printf("Invalid choice. Please choose 1, 2, 3, or 4.\n");
                break;
        }
    }
}

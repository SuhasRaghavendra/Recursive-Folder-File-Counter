/*
 * synth.c - Synthetic Directory Tree Generator (Implementation)
 * -------------------------------------------------------------
 * Builds and tears down controlled directory structures used by the
 * DAA synthetic benchmark (menu option 5).
 *
 * Two tree shapes are created to stress-test algorithm characteristics:
 *
 *   Deep Tree  → Chain graph. Exposes recursive call-stack depth limits
 *                and shows Iterative DFS peak stack = O(h) = depth.
 *
 *   Wide Tree  → Star graph. Exposes BFS memory consumption: the queue
 *                must hold all `width` children simultaneously (O(w)).
 *                DFS only ever holds one node on the stack at a time.
 *
 * Implementation Notes:
 *   - All directory creation uses the Win32 CreateDirectoryA API.
 *   - cleanupSynthTree uses FindFirstFileA / FindNextFileA to walk the
 *     tree (the same APIs used by the scanner) and RemoveDirectoryA /
 *     DeleteFileA for removal. It intentionally mirrors the scanner's
 *     own traversal to demonstrate that the same APIs serve both read
 *     and write operations on the filesystem graph.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "synth.h"
#include "scanner.h"   /* For MAX_PATH_BUFFER. */

int createDeepTree(const char *basePath, unsigned int depth) {
    char current[MAX_PATH_BUFFER];
    char next[MAX_PATH_BUFFER];
    unsigned int i;
    int written;

    /* Start from basePath and descend one level at a time. */
    strncpy(current, basePath, sizeof(current) - 1);
    current[sizeof(current) - 1] = '\0';

    for (i = 0; i < depth; i++) {
        /* Build the child path: <current>\L<level>  */
        written = snprintf(next, sizeof(next), "%s\\L%04u", current, i + 1);
        if (written <= 0 || (size_t)written >= sizeof(next)) {
            fprintf(stderr, "[synth] Path too long at depth %u.\n", i);
            return 0;
        }

        if (!CreateDirectoryA(next, NULL)) {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                fprintf(stderr, "[synth] CreateDirectory failed at %s (err=%lu).\n",
                        next, err);
                return 0;
            }
        }

        /* The child becomes the parent for the next iteration. */
        strncpy(current, next, sizeof(current) - 1);
        current[sizeof(current) - 1] = '\0';
    }

    return 1;
}

int createWideTree(const char *basePath, unsigned int width) {
    char child[MAX_PATH_BUFFER];
    unsigned int i;
    int written;

    for (i = 0; i < width; i++) {
        written = snprintf(child, sizeof(child), "%s\\D%05u", basePath, i);
        if (written <= 0 || (size_t)written >= sizeof(child)) {
            fprintf(stderr, "[synth] Path too long at index %u.\n", i);
            return 0;
        }

        if (!CreateDirectoryA(child, NULL)) {
            DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS) {
                fprintf(stderr, "[synth] CreateDirectory failed at %s (err=%lu).\n",
                        child, err);
                return 0;
            }
        }
    }

    return 1;
}

/*
 * cleanupSynthTree - Iteratively remove a directory and all contents.
 *
 * WHY ITERATIVE:
 *   The recursive approach caused a stack overflow on the deep tree.
 *   With depth=200 and two MAX_PATH_BUFFER (32KB each) locals per frame,
 *   each stack frame is ~64KB. 200 × 64KB = 12.8MB which exceeds the
 *   Windows default thread stack of 1MB.
 *
 * HOW IT WORKS (iterative post-order deletion):
 *   We use two heap-allocated path arrays:
 *     visit[]  - directories to process (DFS order via an explicit stack).
 *     delete[] - full list of paths to remove, accumulated in visit order.
 *
 *   Pass 1: Walk depth-first, pushing every path encountered into delete[].
 *   Pass 2: Remove paths in REVERSE order so children are deleted before
 *           their parents.
 *
 *   This guarantees that by the time we call RemoveDirectoryA on any node,
 *   all its children have already been removed — exactly what post-order
 *   traversal requires.
 */
int cleanupSynthTree(const char *basePath) {
    /* Maximum paths we expect to delete. Wide tree has 10,001 paths;
     * deep tree has 201. 20,000 gives comfortable headroom.          */
#define MAX_DELETE_PATHS 20000
#define PATH_BUF_SIZE    4096   /* Short fixed buffer — temp paths are short. */

    char  **deletePaths = NULL; /* Paths to delete, filled in DFS order.  */
    char  **visitStack  = NULL; /* Explicit DFS stack.                    */
    int     deleteCount = 0;
    int     visitTop    = 0;
    char   *buf         = NULL;
    char   *childBuf    = NULL;
    int     i;

    /* Allocate the two path arrays and working buffers on the heap. */
    deletePaths = (char **)malloc(MAX_DELETE_PATHS * sizeof(char *));
    visitStack  = (char **)malloc(MAX_DELETE_PATHS * sizeof(char *));
    buf         = (char *)malloc(PATH_BUF_SIZE);
    childBuf    = (char *)malloc(PATH_BUF_SIZE);

    if (!deletePaths || !visitStack || !buf || !childBuf) {
        goto cleanup;
    }
    for (i = 0; i < MAX_DELETE_PATHS; i++) {
        deletePaths[i] = NULL;
        visitStack[i]  = NULL;
    }

    /* --- Pass 1: DFS walk, record every path into deletePaths[] --- */

    /* Push root onto the visit stack. */
    visitStack[visitTop] = _strdup(basePath);
    if (!visitStack[visitTop]) goto cleanup;
    visitTop++;

    while (visitTop > 0 && deleteCount < MAX_DELETE_PATHS) {
        WIN32_FIND_DATAA findData;
        HANDLE           handle;
        char            *current;

        /* Pop the top path. */
        visitTop--;
        current = visitStack[visitTop];
        visitStack[visitTop] = NULL;

        /* Record it for later deletion. */
        deletePaths[deleteCount] = current;
        deleteCount++;

        /* Build search pattern: <current>\* */
        snprintf(buf, PATH_BUF_SIZE, "%s\\*", current);

        handle = FindFirstFileA(buf, &findData);
        if (handle == INVALID_HANDLE_VALUE) {
            continue; /* Empty directory — nothing to push. */
        }

        do {
            if (strcmp(findData.cFileName, ".") == 0 ||
                strcmp(findData.cFileName, "..") == 0) {
                continue;
            }

            snprintf(childBuf, PATH_BUF_SIZE, "%s\\%s", current, findData.cFileName);

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                /* Push child directory onto visit stack. */
                if (visitTop < MAX_DELETE_PATHS) {
                    visitStack[visitTop] = _strdup(childBuf);
                    if (visitStack[visitTop]) visitTop++;
                }
            } else {
                /* Leaf file — record for deletion directly. */
                if (deleteCount < MAX_DELETE_PATHS) {
                    deletePaths[deleteCount] = _strdup(childBuf);
                    if (deletePaths[deleteCount]) deleteCount++;
                }
            }
        } while (FindNextFileA(handle, &findData));

        FindClose(handle);
    }

    /* --- Pass 2: Delete in reverse order (deepest paths first) --- */
    for (i = deleteCount - 1; i >= 0; i--) {
        if (!deletePaths[i]) continue;
        DWORD attr = GetFileAttributesA(deletePaths[i]);
        if (attr != INVALID_FILE_ATTRIBUTES) {
            if (attr & FILE_ATTRIBUTE_DIRECTORY) {
                RemoveDirectoryA(deletePaths[i]);
            } else {
                DeleteFileA(deletePaths[i]);
            }
        }
        free(deletePaths[i]);
        deletePaths[i] = NULL;
    }

cleanup:
    /* Free any remaining unprocessed visit stack entries. */
    if (visitStack) {
        for (i = 0; i < MAX_DELETE_PATHS; i++) {
            if (visitStack[i]) { free(visitStack[i]); }
        }
        free(visitStack);
    }
    if (deletePaths) {
        for (i = 0; i < MAX_DELETE_PATHS; i++) {
            if (deletePaths[i]) { free(deletePaths[i]); }
        }
        free(deletePaths);
    }
    free(buf);
    free(childBuf);

    return 1;

#undef MAX_DELETE_PATHS
#undef PATH_BUF_SIZE
}

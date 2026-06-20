/*
 * synth.h - Synthetic Directory Tree Generator for DAA Benchmarking
 * ------------------------------------------------------------------
 * Provides functions to programmatically create and destroy controlled
 * directory structures for algorithm worst-case / best-case analysis.
 *
 * DAA Relevance — Algorithm Analysis on Extreme Graph Inputs:
 *   Real-world filesystems mix deep paths and wide directories, making
 *   it hard to isolate specific algorithm behaviours. Synthetic trees
 *   let us create deterministic worst-case inputs:
 *
 *   Deep Tree (Skewed / Chain Graph):
 *     A single path chain nested `depth` levels deep. Example (depth=4):
 *       root\L1\L2\L3\L4
 *     This is the worst case for BFS memory (queue must still do O(w)
 *     per level, but w=1 so it stays tiny) and for Recursive DFS call
 *     depth (may overflow the system call stack if depth is huge).
 *     Iterative DFS stack grows to exactly `depth` nodes — minimum DFS
 *     space usage.
 *
 *   Wide Tree (Flat / Star Graph):
 *     A single root directory containing `width` immediate subdirectories,
 *     none of which have children. Example (width=10000):
 *       root\D0000, root\D0001, ..., root\D9999
 *     This is the worst case for BFS memory: the queue must hold all
 *     `width` nodes simultaneously (O(w) peak). DFS stack peaks at 1
 *     (only one node at a time). This directly demonstrates the
 *     space-complexity difference between the two traversal strategies.
 *
 * Usage:
 *   The benchmark function in main.c:
 *     1. Calls createDeepTree / createWideTree to build the structure.
 *     2. Runs all three algorithms and prints comparison tables.
 *     3. Calls cleanupSynthTree to delete everything.
 */

#ifndef SYNTH_H
#define SYNTH_H

/*
 * createDeepTree - Build a single-chain directory tree.
 *
 * Creates a directory hierarchy `depth` levels deep under `basePath`.
 * Each level contains exactly one subdirectory named "L%04u".
 *
 * Parameters:
 *   basePath - The root directory under which the tree is created.
 *              Must already exist and be writable.
 *   depth    - Number of nested levels to create.
 *
 * Returns 1 on success, 0 if any directory creation fails.
 */
int createDeepTree(const char *basePath, unsigned int depth);

/*
 * createWideTree - Build a flat star-shaped directory tree.
 *
 * Creates `width` subdirectories directly under `basePath`,
 * each named "D%05u" (e.g., D00000 .. D09999). None have children.
 *
 * Parameters:
 *   basePath - The root directory under which the subdirs are created.
 *              Must already exist and be writable.
 *   width    - Number of immediate subdirectories to create.
 *
 * Returns 1 on success, 0 if any directory creation fails.
 */
int createWideTree(const char *basePath, unsigned int width);

/*
 * cleanupSynthTree - Recursively delete a synthetic tree.
 *
 * Walks `basePath` recursively and removes every file and directory,
 * then removes `basePath` itself. This is always called after a
 * synthetic benchmark run to leave the workspace clean.
 *
 * Returns 1 on success, 0 if any removal operation fails.
 */
int cleanupSynthTree(const char *basePath);

#endif /* SYNTH_H */

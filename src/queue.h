/*
 * queue.h - Dynamic Circular Queue for BFS
 * -----------------------------------------
 * Implements a heap-allocated, ring-buffer queue of PathNode elements.
 * Used by the Breadth-First Search (BFS) algorithm to traverse the
 * directory tree level by level.
 *
 * DAA Relevance:
 *   - Demonstrates O(w) space complexity where w = max directory width
 *     (maximum number of sibling subdirectories at any single depth level).
 *   - Tracks peak size to empirically validate BFS space complexity,
 *     contrasting it with the O(h) space of DFS.
 *   - Supports configurable growth policies (double vs. linear) to
 *     demonstrate amortized analysis alongside the stack implementation.
 *
 * Ring-Buffer Design:
 *   The circular (ring) buffer avoids repeatedly copying elements when
 *   the head advances. The head and tail indices wrap around using
 *   modular arithmetic, providing O(1) enqueue and dequeue operations.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "scanner.h"   /* For PathNode, GrowthPolicy, MAX_PATH_BUFFER. */

/*
 * Queue - Dynamically resizing circular (ring) buffer queue.
 *
 * Fields:
 *   items        - Heap-allocated array of PathNode elements.
 *   head         - Index of the front element (next to be dequeued).
 *   tail         - Index of the slot for the next enqueue.
 *   size         - Number of elements currently in the queue.
 *   capacity     - Total allocated slots (may be larger than size).
 *   peakSize     - Highest value 'size' ever reached during use.
 *                  Measures peak O(w) space usage for BFS.
 *   reallocCount - Number of resize operations performed.
 *                  Compare with Stack's reallocCount to contrast
 *                  how growth policies affect differently-shaped
 *                  directory trees.
 *   policy       - Growth policy controlling resize behaviour.
 */
typedef struct {
    PathNode    *items;
    size_t       head;
    size_t       tail;
    size_t       size;
    size_t       capacity;
    size_t       peakSize;
    size_t       reallocCount;
    GrowthPolicy policy;
} Queue;

/* Initialize the queue with the given growth policy. Returns 1 on
 * success, 0 if the initial memory allocation fails.               */
int  queueInit(Queue *queue, GrowthPolicy policy);

/* Free all heap memory owned by the queue.                          */
void queueFree(Queue *queue);

/* Add a PathNode to the back of the queue. Resizes automatically
 * according to the configured GrowthPolicy. Returns 1 on success,
 * 0 if memory allocation fails.                                     */
int  queueEnqueue(Queue *queue, const PathNode *node);

/* Remove a PathNode from the front of the queue into *node.
 * Returns 1 on success, 0 if the queue is empty.                   */
int  queueDequeue(Queue *queue, PathNode *node);

/* Returns 1 if the queue contains no elements, 0 otherwise.        */
int  queueIsEmpty(const Queue *queue);

#endif /* QUEUE_H */

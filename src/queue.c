/*
 * queue.c - Dynamic Circular Queue Implementation for BFS
 * --------------------------------------------------------
 * Provides a heap-allocated, auto-resizing circular (ring-buffer) queue
 * of PathNode items. The queue is central to the BFS traversal algorithm,
 * which explores directories level by level.
 *
 * DAA Concepts Demonstrated:
 *   1. Space Complexity: Peak queue size = O(w), where w is the maximum
 *      number of sibling directories at any single depth level. Tracked
 *      via the `peakSize` field. BFS typically consumes far more memory
 *      than DFS on wide, shallow trees — proven empirically here.
 *
 *   2. Amortized Analysis: Same two growth policies as the stack:
 *      - GROWTH_DOUBLE: O(log N) resize operations. O(N) total copies.
 *      - GROWTH_LINEAR: O(N) resize operations. O(N^2) total copies.
 *      The `reallocCount` field enables side-by-side comparison.
 *
 *   3. Circular Buffer Design: The ring buffer avoids the O(N) cost of
 *      shifting elements on dequeue. Head and tail indices advance with
 *      modular arithmetic, keeping both enqueue and dequeue at O(1).
 *      On resize, elements are linearised into the new buffer so that
 *      the modular indexing can reset cleanly.
 */

#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Initial number of PathNode slots allocated when the queue is created. */
#define INITIAL_QUEUE_CAPACITY  128

/*
 * LINEAR_STEP is the fixed number of extra slots added per resize when
 * using the GROWTH_LINEAR policy. Mirrors the constant in stack.c so
 * that comparisons between the two data structures are fair.
 */
#define LINEAR_STEP             64

/*
 * queueResize - Internal helper that expands the backing array.
 *
 * Because the live elements may wrap around the end of the ring buffer,
 * we allocate a fresh linear block and copy elements in logical order
 * (oldest first) before freeing the old block.
 */
static int queueResize(Queue *queue, size_t newCapacity) {
    PathNode *newItems;
    size_t    i;

    newItems = (PathNode *)malloc(sizeof(PathNode) * newCapacity);
    if (newItems == NULL) {
        return 0;
    }

    /* Copy elements in logical order from head → tail, unwrapping the ring. */
    for (i = 0; i < queue->size; i++) {
        newItems[i] = queue->items[(queue->head + i) % queue->capacity];
    }

    free(queue->items);
    queue->items    = newItems;
    queue->capacity = newCapacity;
    queue->head     = 0;
    queue->tail     = queue->size;   /* Next write slot is right after last element. */
    return 1;
}

int queueInit(Queue *queue, GrowthPolicy policy) {
    queue->items = (PathNode *)malloc(sizeof(PathNode) * INITIAL_QUEUE_CAPACITY);
    if (queue->items == NULL) {
        queue->head        = 0;
        queue->tail        = 0;
        queue->size        = 0;
        queue->capacity    = 0;
        queue->peakSize    = 0;
        queue->reallocCount = 0;
        queue->policy      = policy;
        return 0;
    }

    queue->head        = 0;
    queue->tail        = 0;
    queue->size        = 0;
    queue->capacity    = INITIAL_QUEUE_CAPACITY;
    queue->peakSize    = 0;
    queue->reallocCount = 0;
    queue->policy      = policy;
    return 1;
}

void queueFree(Queue *queue) {
    free(queue->items);
    queue->items    = NULL;
    queue->head     = 0;
    queue->tail     = 0;
    queue->size     = 0;
    queue->capacity = 0;
}

int queueEnqueue(Queue *queue, const PathNode *node) {
    /* Resize if the ring buffer is completely full. */
    if (queue->size == queue->capacity) {
        size_t newCapacity;

        if (queue->policy == GROWTH_DOUBLE) {
            newCapacity = queue->capacity * 2;
        } else {
            newCapacity = queue->capacity + LINEAR_STEP;
        }

        if (!queueResize(queue, newCapacity)) {
            return 0;
        }

        queue->reallocCount++;  /* Track each resize for amortized analysis. */
    }

    queue->items[queue->tail] = *node;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    /* Update peak size for space-complexity measurement. */
    if (queue->size > queue->peakSize) {
        queue->peakSize = queue->size;
    }

    return 1;
}

int queueDequeue(Queue *queue, PathNode *node) {
    if (queue->size == 0) {
        return 0;
    }

    *node       = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return 1;
}

int queueIsEmpty(const Queue *queue) {
    return queue->size == 0;
}

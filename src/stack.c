/*
 * stack.c - Dynamic Stack Implementation for Iterative DFS
 * ----------------------------------------------------------
 * Provides a heap-allocated, auto-resizing stack of PathNode items.
 * The stack is central to the Iterative DFS traversal algorithm, which
 * avoids recursive call-stack overflow by managing its own traversal
 * state on the heap.
 *
 * DAA Concepts Demonstrated:
 *   1. Space Complexity: Peak stack size = O(h), where h is the maximum
 *      directory nesting depth. Tracked via the `peakSize` field.
 *
 *   2. Amortized Analysis: Two growth policies are supported:
 *      - GROWTH_DOUBLE: Capacity doubles on resize. Total element-copies
 *        across all resizes stays O(N) — amortized O(1) per push.
 *      - GROWTH_LINEAR: Capacity grows by a fixed constant. Total copies
 *        across all resizes is O(N^2) — demonstrating the quadratic cost
 *        of linear growth strategies.
 *      The `reallocCount` field tracks how many resize operations occurred,
 *      allowing direct comparison of the two strategies.
 */

#include <stdlib.h>
#include <string.h>

#include "stack.h"

/* Initial number of PathNode slots allocated when the stack is created. */
#define INITIAL_STACK_CAPACITY  128

/*
 * LINEAR_STEP is the number of extra slots added per resize when using
 * the GROWTH_LINEAR policy. A small value (e.g. 64) amplifies the
 * quadratic reallocation cost, making it easy to observe empirically.
 */
#define LINEAR_STEP             64

int stackInit(Stack *stack, GrowthPolicy policy) {
    stack->items = (PathNode *)malloc(sizeof(PathNode) * INITIAL_STACK_CAPACITY);
    if (stack->items == NULL) {
        stack->size        = 0;
        stack->capacity    = 0;
        stack->peakSize    = 0;
        stack->reallocCount = 0;
        stack->policy      = policy;
        return 0;
    }

    stack->size        = 0;
    stack->capacity    = INITIAL_STACK_CAPACITY;
    stack->peakSize    = 0;
    stack->reallocCount = 0;
    stack->policy      = policy;
    return 1;
}

void stackFree(Stack *stack) {
    free(stack->items);
    stack->items    = NULL;
    stack->size     = 0;
    stack->capacity = 0;
}

int stackPush(Stack *stack, const PathNode *node) {
    /* Resize if the array is full. */
    if (stack->size == stack->capacity) {
        PathNode *newItems;
        size_t    newCapacity;

        /* Choose new capacity according to the configured growth policy. */
        if (stack->policy == GROWTH_DOUBLE) {
            newCapacity = stack->capacity * 2;
        } else {
            newCapacity = stack->capacity + LINEAR_STEP;
        }

        newItems = (PathNode *)realloc(stack->items, sizeof(PathNode) * newCapacity);
        if (newItems == NULL) {
            return 0; /* Allocation failed; original array is unchanged. */
        }

        stack->items    = newItems;
        stack->capacity = newCapacity;
        stack->reallocCount++;  /* Track each resize for amortized analysis. */
    }

    stack->items[stack->size] = *node;
    stack->size++;

    /* Update peak size for space-complexity measurement. */
    if (stack->size > stack->peakSize) {
        stack->peakSize = stack->size;
    }

    return 1;
}

int stackPop(Stack *stack, PathNode *node) {
    if (stack->size == 0) {
        return 0;
    }

    stack->size--;
    *node = stack->items[stack->size];
    return 1;
}

int stackIsEmpty(const Stack *stack) {
    return stack->size == 0;
}

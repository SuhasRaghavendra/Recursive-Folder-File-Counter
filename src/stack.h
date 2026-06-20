/*
 * stack.h - Dynamic Stack for Iterative DFS
 * ------------------------------------------
 * Implements a heap-allocated, dynamically resizing stack of PathNode
 * elements. Used by the Iterative DFS algorithm to avoid call-stack
 * overflow that would occur with deep directory hierarchies in a
 * recursive approach.
 *
 * DAA Relevance:
 *   - Demonstrates O(h) space complexity where h = max directory depth.
 *   - Tracks peak size to empirically validate DFS space complexity.
 *   - Supports configurable growth policies (double vs. linear) to
 *     demonstrate amortized analysis of dynamic array resizing.
 */

#ifndef STACK_H
#define STACK_H

#include "scanner.h"

/*
 * Stack - Dynamically resizing array-based stack.
 *
 * Fields:
 *   items       - Heap-allocated array of PathNode elements.
 *   size        - Number of elements currently in the stack.
 *   capacity    - Total allocated slots (may be larger than size).
 *   peakSize    - Highest value 'size' ever reached during use.
 *                 Used to measure peak space complexity (O(h) for DFS).
 *   reallocCount - Number of times the array was reallocated (resized).
 *                  Compared across growth policies to prove amortized
 *                  analysis: doubling causes O(log N) resizes,
 *                  linear causes O(N) resizes.
 *   policy      - The growth policy in effect for this stack instance.
 */
typedef struct {
    PathNode    *items;
    size_t       size;
    size_t       capacity;
    size_t       peakSize;
    size_t       reallocCount;
    GrowthPolicy policy;
} Stack;

/* Initialize the stack with the given growth policy. Returns 1 on
 * success, 0 if the initial memory allocation fails.               */
int  stackInit(Stack *stack, GrowthPolicy policy);

/* Free all heap memory owned by the stack.                          */
void stackFree(Stack *stack);

/* Push a PathNode onto the top of the stack. Resizes automatically
 * according to the configured GrowthPolicy. Returns 1 on success,
 * 0 if memory allocation fails.                                     */
int  stackPush(Stack *stack, const PathNode *node);

/* Pop a PathNode from the top of the stack into *node. Returns 1
 * on success, 0 if the stack is empty.                             */
int  stackPop(Stack *stack, PathNode *node);

/* Returns 1 if the stack contains no elements, 0 otherwise.        */
int  stackIsEmpty(const Stack *stack);

#endif /* STACK_H */

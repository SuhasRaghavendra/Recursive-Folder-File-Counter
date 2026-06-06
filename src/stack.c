#include <stdlib.h>
#include <string.h>

#include "stack.h"

#define INITIAL_STACK_CAPACITY 128

int stackInit(Stack *stack) {
    stack->items = (PathNode *)malloc(sizeof(PathNode) * INITIAL_STACK_CAPACITY);
    if (stack->items == NULL) {
        stack->size = 0;
        stack->capacity = 0;
        return 0;
    }

    stack->size = 0;
    stack->capacity = INITIAL_STACK_CAPACITY;
    return 1;
}

void stackFree(Stack *stack) {
    free(stack->items);
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

int stackPush(Stack *stack, const PathNode *node) {
    PathNode *newItems;
    size_t newCapacity;

    if (stack->size == stack->capacity) {
        newCapacity = stack->capacity * 2;
        newItems = (PathNode *)realloc(stack->items, sizeof(PathNode) * newCapacity);
        if (newItems == NULL) {
            return 0;
        }
        stack->items = newItems;
        stack->capacity = newCapacity;
    }

    stack->items[stack->size] = *node;
    stack->size++;
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

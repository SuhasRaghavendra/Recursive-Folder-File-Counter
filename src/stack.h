#ifndef STACK_H
#define STACK_H

#include "scanner.h"

typedef struct {
    PathNode *items;
    size_t size;
    size_t capacity;
} Stack;

int stackInit(Stack *stack);
void stackFree(Stack *stack);
int stackPush(Stack *stack, const PathNode *node);
int stackPop(Stack *stack, PathNode *node);
int stackIsEmpty(const Stack *stack);

#endif

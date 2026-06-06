#ifndef QUEUE_H
#define QUEUE_H

#include "scanner.h"

typedef struct {
    PathNode *items;
    size_t head;
    size_t tail;
    size_t size;
    size_t capacity;
} Queue;

int queueInit(Queue *queue);
void queueFree(Queue *queue);
int queueEnqueue(Queue *queue, const PathNode *node);
int queueDequeue(Queue *queue, PathNode *node);
int queueIsEmpty(const Queue *queue);

#endif

#include <stdlib.h>

#include "queue.h"

#define INITIAL_QUEUE_CAPACITY 128

static int queueResize(Queue *queue) {
    PathNode *newItems;
    size_t newCapacity;
    size_t i;

    newCapacity = queue->capacity * 2;
    newItems = (PathNode *)malloc(sizeof(PathNode) * newCapacity);
    if (newItems == NULL) {
        return 0;
    }

    for (i = 0; i < queue->size; i++) {
        newItems[i] = queue->items[(queue->head + i) % queue->capacity];
    }

    free(queue->items);
    queue->items = newItems;
    queue->capacity = newCapacity;
    queue->head = 0;
    queue->tail = queue->size;
    return 1;
}

int queueInit(Queue *queue) {
    queue->items = (PathNode *)malloc(sizeof(PathNode) * INITIAL_QUEUE_CAPACITY);
    if (queue->items == NULL) {
        queue->head = 0;
        queue->tail = 0;
        queue->size = 0;
        queue->capacity = 0;
        return 0;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    queue->capacity = INITIAL_QUEUE_CAPACITY;
    return 1;
}

void queueFree(Queue *queue) {
    free(queue->items);
    queue->items = NULL;
    queue->head = 0;
    queue->tail = 0;
    queue->size = 0;
    queue->capacity = 0;
}

int queueEnqueue(Queue *queue, const PathNode *node) {
    if (queue->size == queue->capacity && !queueResize(queue)) {
        return 0;
    }

    queue->items[queue->tail] = *node;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    return 1;
}

int queueDequeue(Queue *queue, PathNode *node) {
    if (queue->size == 0) {
        return 0;
    }

    *node = queue->items[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return 1;
}

int queueIsEmpty(const Queue *queue) {
    return queue->size == 0;
}

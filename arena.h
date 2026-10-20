#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct {
    char *block;
    size_t used, cap;
} Arena;

void arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t n);

#endif
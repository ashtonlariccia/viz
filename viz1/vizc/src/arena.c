#include "arena.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ARENA_BLOCK (64 * 1024)

void arena_init(Arena *a) {
    a->block = NULL;
    a->used = 0;
    a->cap = 0;
}

void *arena_alloc(Arena *a, size_t n) {
    n = (n + 7) & ~(size_t)7;
    if (a->used + n > a->cap) {
        size_t size = n > ARENA_BLOCK ? n : ARENA_BLOCK;
        char *b = malloc(size);
        if (b == NULL) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        a->block = b; a->cap = size; a->used = 0;
    }
    void *p = a->block + a->used;
    a->used = a->used + n;
    memset(p, 0, n);
    return p;
}
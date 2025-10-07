#ifndef MEMORY_H
#define MEMORY_H

typedef struct Memory
{
    CF_Arena scratch;
} Memory;

void memory_init();

void* scratch_alloc(u64 size);
void scratch_reset();

#endif //MEMORY_H

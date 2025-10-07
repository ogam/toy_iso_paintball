#include "common/memory.h"

void memory_init()
{
    Memory* memory = cf_calloc(sizeof(Memory), 1);
    s32 alignment = 16;
    s32 size = 1024 * 1024 * 8;
    memory->scratch = cf_make_arena(alignment, size);
    s_app->memory = memory;
}

void* scratch_alloc(u64 size)
{
    return cf_arena_alloc(&s_app->memory->scratch, (s32)size);
}

void scratch_reset()
{
    cf_arena_reset(&s_app->memory->scratch);
}

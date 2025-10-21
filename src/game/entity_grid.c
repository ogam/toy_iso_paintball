#include "game/entity_grid.h"

Entity_Grid* entity_grid_make(s32 width, s32 height, s32 depth)
{
    Entity_Grid* grid = cf_calloc(sizeof(Entity_Grid), 1);
    
    CF_ASSERT(width > 0 && height > 0);
    
    grid->w = width;
    grid->h = height;
    depth = cf_max(depth, 8);
    
    s32 count = width * height;
    
    grid->slots = cf_calloc(sizeof(ecs_id_t*), count);
    for (s32 index = 0; index < count; ++index)
    {
        cf_array_fit(grid->slots[index], depth);
    }
    
    return grid;
}

void entity_grid_insert(Entity_Grid* grid, V2i tile, ecs_id_t id)
{
    CF_ASSERT(grid);
    
    if (!(tile.x >= 0 && tile.x < grid->w && tile.y >= 0 && tile.y < grid->h))
    {
        return;
    }
    
    b32 do_insert = false;
    
    if (cf_hashtable_has(grid->lookup, id))
    {
        V2i cached_tile = cf_hashtable_get(grid->lookup, id);
        if (v2i_distance(cached_tile, tile) != 0)
        {
            entity_grid_remove_internal(grid, cached_tile, id);
            do_insert = true;
        }
    }
    else
    {
        do_insert = true;
    }
    
    if (do_insert)
    {
        s32 index = tile.x + tile.y * grid->w;
        cf_array_push(grid->slots[index], id);
        cf_hashtable_set(grid->lookup, id, tile);
    }
}

void entity_grid_remove(Entity_Grid* grid, ecs_id_t id)
{
    CF_ASSERT(grid);
    if (cf_hashtable_has(grid->lookup, id))
    {
        V2i cached_tile = cf_hashtable_get(grid->lookup, id);
        entity_grid_remove_internal(grid, cached_tile, id);
    }
}

void entity_grid_clear(Entity_Grid* grid)
{
    CF_ASSERT(grid);
    cf_hashtable_clear(grid->lookup);
    
    for (s32 y = 0; y < grid->h; ++y)
    {
        for (s32 x = 0; x < grid->w; ++x)
        {
            s32 index = x + y * grid->w;
            dyna ecs_id_t* entities = grid->slots[index];
            cf_array_clear(entities);
        }
    }
}

dyna ecs_id_t* entity_grid_query(Entity_Grid* grid, V2i tile)
{
    CF_ASSERT(grid);
    
    if (!(tile.x >= 0 && tile.x < grid->w && tile.y >= 0 && tile.y < grid->h))
    {
        return NULL;
    }
    
    s32 index = tile.x + tile.y * grid->w;
    return grid->slots[index];
}

void entity_grid_remove_internal(Entity_Grid* grid, V2i tile, ecs_id_t id)
{
    CF_ASSERT(grid);
    cf_hashtable_del(grid->lookup, id);
    s32 index = tile.x + tile.y * grid->w;
    dyna ecs_id_t* entities = grid->slots[index];
    for (s32 entity_index = 0; entity_index < cf_array_count(entities); ++entity_index)
    {
        if (entities[entity_index] == id)
        {
            cf_array_del(entities, entity_index);
            break;
        }
    }
}

void entity_grid_destroy(Entity_Grid* grid)
{
    s32 count = grid->w * grid->h;
    for (s32 index = 0; index < count; ++index)
    {
        cf_array_free(grid->slots[index]);
    }
    cf_free(grid->slots);
    cf_free(grid);
}
#ifndef ENTITY_GRID_H
#define ENTITY_GRID_H

typedef struct Entity_Grid
{
    dyna ecs_id_t** slots;
    cf_htbl V2i* lookup;
    s32 w;
    s32 h;
} Entity_Grid;

Entity_Grid* entity_grid_make(s32 width, s32 height, s32 depth);
void entity_grid_insert(Entity_Grid* grid, V2i slot, ecs_id_t id);
void entity_grid_remove(Entity_Grid* grid, ecs_id_t id);
void entity_grid_clear(Entity_Grid* grid);
dyna ecs_id_t* entity_grid_query(Entity_Grid* grid, V2i tile);
void entity_grid_remove_internal(Entity_Grid* grid, V2i tile, ecs_id_t id);
void entity_grid_destroy(Entity_Grid* grid);

#endif //ENTITY_GRID_H

#include "game/entity.h"

// ---------------
// world
// ---------------

Asset_Resource* world_get_campaign_resource(Asset_Object_ID id)
{
    Asset_Resource* resource = assets_get_resource_from_id(id);
    if (resource)
    {
        if (resource->type == Asset_Resource_Type_Campaign)
        {
            dyna const char** level_names = resource_get(resource, "levels");
            if (!cf_array_count(level_names))
            {
                printf("Failed to get campaign %s, no missing level list\n", resource->name);
                resource = NULL;
            }
        }
        else
        {
            printf("Failed to get campaign %s, invalid resource type\n", resource->name);
            resource = NULL;
        }
    }
    
    return resource;
}

const char* world_get_campaign_name()
{
    World* world = s_app->world;
    const char* name = NULL;
    Asset_Resource* resource = world_get_campaign_resource(world->campaign_id);
    if (resource)
    {
        name = resource->name;
    }
    return name;
}

b32 world_load_campaign(Asset_Object_ID id)
{
    World* world = s_app->world;
    b32 load_successful = false;
    Asset_Resource* resource = world_get_campaign_resource(id);
    if (resource)
    {
        dyna const char** level_names = resource_get(resource, "levels");
        if (cf_array_count(level_names))
        {
            world->campaign_id = id;
            world->level_index = -1;
            load_successful = true;
            printf("Loaded campaign: %s\n", resource->name);
        }
    }
    
    return load_successful;
}

void world_unload_campaign()
{
    World* world = s_app->world;
    world->campaign_id = 0;
    world->level_index = -1;
}

b32 world_advance_campaign()
{
    World* world = s_app->world;
    b32 begin_next_campaign_level = false;
    
    Asset_Resource* resource = world_get_campaign_resource(world->campaign_id);
    if (resource)
    {
        dyna const char** level_names = resource_get(resource, "levels");
        ++world->level_index;
        if (world->level_index < cf_array_count(level_names))
        {
            make_event_load_level(level_names[world->level_index]);
            begin_next_campaign_level = true;;
            printf("Advance campaign to %s\n", level_names[world->level_index]);
        }
    }
    
    return begin_next_campaign_level;
}

Campaign_State world_get_campaign_state()
{
    World* world = s_app->world;
    Campaign_State campaign_state = Campaign_State_Invalid;
    
    Asset_Resource* resource = world_get_campaign_resource(world->campaign_id);
    if (resource)
    {
        dyna const char** level_names = resource_get(resource, "levels");
        campaign_state = Campaign_State_In_Progress;
        if (world->level_index >= cf_array_count(level_names))
        {
            campaign_state = Campaign_State_Finish;
        }
    }
    
    return campaign_state;
}

s32 world_remaining_campaign_levels()
{
    World* world = s_app->world;
    s32 remaining_level_count = 0;
    
    Asset_Resource* resource = world_get_campaign_resource(world->campaign_id);
    if (resource)
    {
        dyna const char** level_names = resource_get(resource, "levels");
        remaining_level_count = cf_array_count(level_names) - world->level_index;
    }
    
    return remaining_level_count;
}

inline f32 elevation_s32_to_f32(s32 elevation)
{
    return (f32)elevation * 0.5f;
}

inline s32 elevation_f32_to_s32(f32 elevation)
{
    return (s32)CF_FLOORF(elevation * 2.0f);
}

inline s32 get_tile_index(V2i tile)
{
    return tile.x + tile.y * s_app->world->level.size.x;
}

inline s32 get_tile_elevation(V2i tile)
{
    s32 elevation = 0;
    
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    if (tile_ptr && *tile_id)
    {
        elevation = tile_ptr->elevation;
    }
    return elevation;
}

f32 get_tile_elevation_f32(V2i tile)
{
    return elevation_s32_to_f32(get_tile_elevation(tile));
}

inline f32 get_tile_elevation_offset(V2i tile)
{
    World* world = s_app->world;
    f32 elevation = 0;
    if (is_tile_in_bounds(tile))
    {
        s32 tile_index = get_tile_index(tile);
        Asset_Object_ID* tile_id = world->level.tile_ids + tile_index;
        if (*tile_id)
        {
            elevation = world->level.tile_elevation_offsets[tile_index];
        }
    }
    
    return elevation;
}

inline f32 get_tile_elevation_velocity(V2i tile)
{
    World* world = s_app->world;
    f32 velocity = 0;
    if (is_tile_in_bounds(tile))
    {
        s32 tile_index = get_tile_index(tile);
        Asset_Object_ID* tile_id = world->level.tile_ids + tile_index;
        if (*tile_id)
        {
            velocity = world->level.tile_elevation_velocity_offsets[tile_index];
        }
    }
    
    return velocity;
}

inline f32 get_tile_total_elevation(V2i tile)
{
    f32 elevation = elevation_s32_to_f32(get_tile_elevation(tile));
    if (is_tile_in_bounds(tile))
    {
        elevation += get_tile_elevation_offset(tile);
    }
    
    return elevation;
}

b32 tile_set_meta_data(V2i tile, Asset_Object_ID id)
{
    b32 changed = false;
    Asset_Object_ID* tile_id = get_tile_id(tile);
    Tile* tile_ptr = get_tile(tile);
    
    if (tile_ptr && tile_id)
    {
        changed = *tile_id != id;
        
        *tile_id = id;
        
        if (id == 0)
        {
            CF_MEMSET(tile_ptr, 0, sizeof(Tile));
        }
        else
        {
            Asset_Resource* resource = assets_get_resource_from_id(*tile_id);
            CF_ASSERT(resource->type == Asset_Resource_Type_Tile);
            s8 walkable = *(s8*)resource_get(resource, "walkable");
            tile_ptr->walkable = walkable;
            
            cf_htbl const char** animations = resource_get(resource, "animations");
            if (!cf_hashtable_has(animations, cf_sintern("stack")))
            {
                tile_ptr->stackless = true;
            }
        }
    }
    return changed;
}

b32 tile_set_minimum_elevation(V2i tile, Asset_Object_ID id, s32 minimum_elevation)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    b32 changed = false;
    
    if (tile_ptr && tile_id)
    {
        tile_ptr->elevation = cf_max(tile_ptr->elevation, minimum_elevation);
        tile_set_meta_data(tile, id);
        changed = true;
    }
    
    return changed;
}

b32 tile_raise(V2i tile, s32 amount, s32 max_elevation)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    b32 changed = false;
    
    if (tile_ptr && tile_id && *tile_id)
    {
        s32 prev_elevation = tile_ptr->elevation;
        tile_ptr->elevation = cf_min(tile_ptr->elevation + amount, max_elevation);
        
        if (prev_elevation != tile_ptr->elevation)
        {
            changed = true;
        }
    }
    
    return changed;
}

b32 tile_lower(V2i tile, s32 amount, s32 min_elevation)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    b32 changed = false;
    
    if (tile_ptr && tile_id && *tile_id)
    {
        s32 prev_elevation = tile_ptr->elevation;
        tile_ptr->elevation = cf_max(tile_ptr->elevation - amount, min_elevation);
        
        if (prev_elevation != tile_ptr->elevation)
        {
            changed = true;
        }
    }
    
    return changed;
}

b32 tile_place_or_raise(V2i tile, Asset_Object_ID id, s32 amount, s32 max_elevation)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    b32 changed = false;
    
    if (tile_ptr && tile_id)
    {
        if (tile_set_meta_data(tile, id))
        {
            changed = true;
        }
        else
        {
            tile_ptr->elevation = cf_min(tile_ptr->elevation + amount, max_elevation);
            changed = true;
        }
    }
    
    return changed;
}

b32 tile_remove(V2i tile)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    b32 changed = false;
    
    if (tile_ptr && tile_id)
    {
        if (*tile_id != 0)
        {
            changed = true;
        }
        
        *tile_id = 0;
        CF_MEMSET(tile_ptr, 0, sizeof(Tile));
    }
    
    return changed;
}

b32 tile_remove_or_lower(V2i tile, s32 amount, s32 min_elevation)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    b32 changed = false;
    
    if (tile_ptr && tile_id && *tile_id)
    {
        s32 prev_elevation = tile_ptr->elevation;
        tile_ptr->elevation = tile_ptr->elevation - amount;
        
        if (tile_ptr->elevation != prev_elevation)
        {
            changed = true;
        }
        
        if (tile_ptr->elevation < 0)
        {
            *tile_id = 0;
            CF_MEMSET(tile_ptr, 0, sizeof(Tile));
            changed = true;;
        }
        else
        {
            tile_ptr->elevation = cf_max(tile_ptr->elevation, min_elevation);
        }
    }
    
    return changed;
}

Tile* get_tile(V2i tile)
{
    Tile* tile_ptr = NULL;
    
    if (is_tile_in_bounds(tile))
    {
        s32 tile_index = get_tile_index(tile);
        tile_ptr = s_app->world->level.tiles + tile_index;
    }
    
    return tile_ptr;
}

Asset_Object_ID* get_tile_id(V2i tile)
{
    Asset_Object_ID* tile_id = NULL;
    
    if (is_tile_in_bounds(tile))
    {
        s32 tile_index = get_tile_index(tile);
        tile_id = s_app->world->level.tile_ids + tile_index;
    }
    
    return tile_id;
}

inline b32 is_culled(CF_V2 position)
{
    CF_V2 tile_size = assets_get_tile_size();
    CF_Aabb bounds = cf_screen_bounds_to_world();
    
    bounds = cf_expand_aabb_f(bounds, cf_max(tile_size.x, tile_size.y) * 2.0f);
    return !cf_contains_point(bounds, position);
}

CF_Aabb get_level_aabb()
{
    CF_V2 tile_size = assets_get_tile_size();
    V2i level_size = s_app->world->level.size;
    f32 max_y = 0.0f;
    f32 min_x = 0.0f;
    f32 max_x = 0.0f;
    // since world is isometric, the mins and maxes needs to be calculated
    // from each respective corner
    
    V2i top = v2i(.x = level_size.x - 1, .y = level_size.y - 1);
    V2i left = v2i(.x = 0, .y = level_size.y - 1);
    V2i right = v2i(.x = level_size.x - 1, .y = 0);
    
    f32 top_elevation = get_tile_total_elevation(top);
    f32 left_elevation = get_tile_total_elevation(left);
    f32 right_elevation = get_tile_total_elevation(right);
    
    max_y = v2i_to_v2_iso_center(top, top_elevation).y + tile_size.y;
    min_x = v2i_to_v2_iso_center(left, left_elevation).x - tile_size.x;
    max_x = v2i_to_v2_iso_center(right, right_elevation).x + tile_size.x;
    
    CF_V2 min = cf_v2(min_x, 0);
    CF_V2 max = cf_v2(max_x, max_y);
    return cf_make_aabb(min, max);
}

V2i get_tile_sample_with_depth(CF_V2 position)
{
    V2i level_size = s_app->world->level.size;
    b32 adjust_sample_tile = false;
    V2i sample_tile = screen_v2_to_v2i(position);
    if (sample_tile.x >= level_size.x || sample_tile.x < 0)
    {
        if (sample_tile.y > 0)
        {
            adjust_sample_tile = true;
        }
    }
    else if (sample_tile.y >= level_size.y)
    {
        if (sample_tile.x < level_size.x)
        {
            adjust_sample_tile = true;
        }
    }
    
    if (adjust_sample_tile)
    {
        level_size.x--;
        level_size.y--;
        s32 max_axis_distance = cf_max(sample_tile.x - level_size.x, sample_tile.y - level_size.y);
        V2i adjustment = v2i(.x = max_axis_distance, .y = max_axis_distance);
        sample_tile = v2i_sub(sample_tile, adjustment);
    }
    
    return sample_tile;
}

V2i get_tile_from_world(CF_V2 position)
{
    f32 epsilon = 1e-2f;
    b32 is_mouse_on_tile_surface = false;
    V2i sample_tile = get_tile_sample_with_depth(position);
    
    V2i offsets[] =
    {
        v2i(),
        v2i(.x = -1),
        v2i(.y = -1),
        v2i(.x = -1, .y = -1),
    };
    
    // to avoid crawling all the way from the very tip top of a level to some out of bounds tile
    s32 max_iterations = 20;
    
    // try to find closest tile surface to mouse cursor
    // only using 3 offsets since most likely when hovering a tile we're only above, slightly to the left or right
    // at the moment we don't support tiles moving laterally, only vertically up and down along the z (elevation)
    while (max_iterations-- && !is_mouse_on_tile_surface)
    {
        f32 closest_to_center = F32_MAX;
        
        for (s32 index = 0; index < CF_ARRAY_SIZE(offsets); ++index)
        {
            V2i tile = v2i_add(sample_tile, offsets[index]);
            
            // if a tile is empty it can still provide a tile surface at 0 elevation
            // so if tile sampling does not need to check empty tiles then set allow_empty_tiles to false
            if (is_tile_empty(tile))
            {
                continue;
            }
            
            CF_Poly poly = get_tile_top_surface_poly(tile);
            f32 area = cf_calc_area(poly);
            poly.verts[poly.count++] = position;
            cf_make_poly(&poly);
            f32 new_area = cf_calc_area(poly);
            
            // compare area to see if the new convex hull generated has changed, the vertex order may change but
            // area should roughly be the same
            if (cf_abs(new_area - area) < epsilon)
            {
                f32 distance = cf_abs(cf_distance(position, cf_centroid(poly.verts, poly.count)));
                if (closest_to_center > distance)
                {
                    closest_to_center = distance;
                    sample_tile = tile;
                }
                is_mouse_on_tile_surface = true;
            }
        }
        
        if (!is_mouse_on_tile_surface)
        {
            --sample_tile.x;
            --sample_tile.y;
        }
    }
    
    if (!is_mouse_on_tile_surface)
    {
        sample_tile = v2i(.x = -1, .y = -1);
    }
    
    return sample_tile;
}

V2i get_tile_from_input_cursor(CF_V2 position, b32 allow_empty_tiles)
{
    f32 epsilon = 1e-2f;
    V2i level_size = s_app->world->level.size;
    V2i sample_tile = get_tile_sample_with_depth(position);
    V2i found_tile = v2i(.x = -1, .y = -1);
    
    V2i offsets[] =
    {
        v2i(),
        v2i(.x = -1),
        v2i(.y = -1),
        v2i(.x = -1, .y = -1),
    };
    
    // to avoid crawling all the way from the very tip top of a level to some out of bounds tile
    s32 max_iterations = cf_max(level_size.x, level_size.y);
    s32 closest_depth = S32_MAX;
    V2i closest_tile = found_tile;
    
    // try to find closest tile surface to mouse cursor in terms of depth
    // only using 3 offsets since most likely when hovering a tile we're only above, slightly to the left or right
    // at the moment we don't support tiles moving laterally, only vertically up and down along the z (elevation)
    while (max_iterations--)
    {
        for (s32 index = 0; index < CF_ARRAY_SIZE(offsets); ++index)
        {
            V2i tile = v2i_add(sample_tile, offsets[index]);
            
            // if a tile is empty it can still provide a tile surface at 0 elevation
            // so if tile sampling does not need to check empty tiles then set allow_empty_tiles to false
            if (!is_tile_in_bounds(tile) || (!allow_empty_tiles && is_tile_empty(tile)))
            {
                continue;
            }
            
            CF_Poly poly = get_tile_top_surface_poly(tile);
            f32 area = cf_calc_area(poly);
            poly.verts[poly.count++] = position;
            cf_make_poly(&poly);
            f32 new_area = cf_calc_area(poly);
            
            // compare area to see if the new convex hull generated has changed, the vertex order may change but
            // area should roughly be the same
            if (cf_abs(new_area - area) < epsilon)
            {
                f32 distance = cf_abs(cf_distance(position, cf_centroid(poly.verts, poly.count)));
                s32 depth = tile.x + tile.y * level_size.x;
                if (closest_depth > depth)
                {
                    closest_depth = depth;
                    closest_tile = tile;
                }
            }
        }
        
        --sample_tile.x;
        --sample_tile.y;
    }
    
    if (closest_depth != S32_MAX)
    {
        found_tile = closest_tile;
    }
    
    return found_tile;
}

const char* get_tile_animation(Tile* tile_ptr)
{
    fixed char* animation_name = make_scratch_string(256);
    pq const char** cardinal_counts = NULL;
    MAKE_SCRATCH_PQ(cardinal_counts, 4);
    pq_set_descending(cardinal_counts);
    
    pq_set_weight(cardinal_counts, cf_sintern("w"), 0);
    pq_set_weight(cardinal_counts, cf_sintern("n"), 0);
    pq_set_weight(cardinal_counts, cf_sintern("e"), 0);
    pq_set_weight(cardinal_counts, cf_sintern("s"), 0);
    
    if (tile_ptr->w)
    {
        pq_add_weight(cardinal_counts, cf_sintern("w"), 1);
    }
    if (tile_ptr->n)
    {
        pq_add_weight(cardinal_counts, cf_sintern("n"), 1);
    }
    if (tile_ptr->e)
    {
        pq_add_weight(cardinal_counts, cf_sintern("e"), 1);
    }
    if (tile_ptr->s)
    {
        pq_add_weight(cardinal_counts, cf_sintern("s"), 1);
    }
    cf_string_fmt(animation_name, "%s", "tall_");
    if (tile_ptr->elevation & 1)
    {
        cf_string_fmt(animation_name, "%s", "short_");
    }
    
    f32* weights = pq_weights(cardinal_counts);
    f32 prev_weight = weights[0];
    s32 stop_index = 0;
    for (s32 index = 0; index < pq_count(cardinal_counts); ++index)
    {
        if (prev_weight > weights[index])
        {
            stop_index = index;
            break;
        }
    }
    
    if (prev_weight == 0.0f)
    {
        // this means all sides does not have any open connections so return a center tile
        cf_string_fmt_append(animation_name, "%s", "c");
    }
    else if (stop_index < 4)
    {
        // re-sort so that n and s are first so this would end up as `ne`, `nwe`, `nsw`, `new`
        for (s32 index = 0; index < stop_index; ++index)
        {
            if (cardinal_counts[index] == cf_sintern("n"))
            {
                pq_add_weight(cardinal_counts, cardinal_counts[index], 4);
            }
            else if (cardinal_counts[index] == cf_sintern("s"))
            {
                pq_add_weight(cardinal_counts, cardinal_counts[index], 3);
            }
            else if (cardinal_counts[index] == cf_sintern("w"))
            {
                pq_add_weight(cardinal_counts, cardinal_counts[index], 2);
            }
            else if (cardinal_counts[index] == cf_sintern("e"))
            {
                pq_add_weight(cardinal_counts, cardinal_counts[index], 1);
            }
        }
        
        for (s32 index = 0; index < stop_index; ++index)
        {
            cf_string_fmt_append(animation_name, "%s", cardinal_counts[index]);
        }
    }
    else
    {
        // all 4 sides equal so return an open animation 
        cf_string_fmt_append(animation_name, "%s", "o");
    }
    
    return animation_name;
}

CF_Poly get_tile_top_surface_poly(V2i tile)
{
    f32 elevation = get_tile_total_elevation(tile);
    CF_M2x2 m = calc_iso_m2();
    CF_V2 p = v2i_to_v2(tile, elevation);
    
    //      p2
    //  p3      p1
    //      p0
    CF_Poly poly;
    poly.verts[0] = cf_mul_m2_v2(m, cf_v2(p.x    , p.y    ));
    poly.verts[1] = cf_mul_m2_v2(m, cf_v2(p.x + 1, p.y    ));
    poly.verts[2] = cf_mul_m2_v2(m, cf_v2(p.x + 1, p.y + 1));
    poly.verts[3] = cf_mul_m2_v2(m, cf_v2(p.x    , p.y + 1));
    poly.count = 4;
    
    return poly;
}

void draw_tile_stack(V2i tile)
{
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    if (!tile_ptr || !tile_id || *tile_id == 0)
    {
        return;
    }
    
    s32 tile_elevation = tile_ptr->elevation;
    
    Asset_Resource* resource = assets_get_resource_from_id(*tile_id);
    CF_V2* scale = resource_get(resource, "scale");
    const char* sprite_name = resource_get(resource, "sprite");
    cf_htbl const char** animations = resource_get(resource, "animations");
    const char* animation_stack = cf_hashtable_get(animations, cf_sintern("stack"));
    
    // don't attempt to draw if animation is missing
    if (animation_stack == NULL)
    {
        return;
    }
    
    CF_Sprite tile_sprite = cf_make_sprite(sprite_name);
    tile_sprite.scale = *scale;
    
    V2i occlusion_tile = v2i_add(tile, v2i(.x = -1, .y = -1));
    s32 stack_occlusion_elevation = 0;
    
    // check if occlusion tile has a stackable animation
    {
        Tile* occlusion_tile_ptr = get_tile(occlusion_tile);
        if (occlusion_tile_ptr)
        {
            if (!occlusion_tile_ptr->stackless)
            {
                stack_occlusion_elevation = occlusion_tile_ptr->elevation;
            }
        }
    }
    
    // lower this occlusion start by 1 additional full tile block so 
    // visually blocks looks less jank
    f32 occlusion_tile_elevation_offset = cf_max(get_tile_elevation_offset(occlusion_tile) - 2, 0);
    
    // clear off any half/short elevation
    stack_occlusion_elevation = stack_occlusion_elevation & ~(1);
    cf_sprite_play(&tile_sprite, animation_stack);
    
    // elevation culling, start from occluded highest tile
    for (s32 current_elevation = stack_occlusion_elevation; current_elevation < tile_elevation; current_elevation += 2)
    {
        f32 stacked_elevation = elevation_s32_to_f32(current_elevation);
        CF_V2 stacked_position = v2i_to_v2_iso(tile, stacked_elevation);
        
        // skip drawing a stacked tile if it's not visible
        if (!is_culled(stacked_position))
        {
            tile_sprite.transform.p = stacked_position;
            
            draw_push_sprite(Draw_Sort_Key_Type_Tile_Stack, tile, stacked_elevation, &tile_sprite);
            s_app->draw_tile_count++;
        }
    }
}

//  @todo:  more aggressive culling, can do this after initial demo
void draw_tile(V2i tile)
{
    if (is_tile_in_bounds(tile))
    {
        // due to how the current tiles look, all drawn sprite tiles needs to actually be 1 entire
        // tile size above, otherwise the drawn tiles will overlap
        Tile* tile_ptr = get_tile(tile);
        Asset_Object_ID* tile_id = get_tile_id(tile);
        s32 tile_elevation = tile_ptr->elevation;
        f32 elevation = get_tile_total_elevation(tile);
        f32 half_elevation = 0.0f;
        
        if (tile_elevation & 1)
        {
            half_elevation = 0.5f;
        }
        
        // don't draw empty tiles
        if (*tile_id == 0)
        {
            return;
        }
        
        CF_V2 position = v2i_to_v2_iso(tile, elevation + half_elevation);
        
        // this means the tile is moving and top part is clipped off but lower is stack is still
        // visibile
        if (is_culled(position))
        {
            draw_tile_stack(tile);
        }
        else if (!is_culled(position))
        {
            // not culled, draw normally
            Asset_Resource* resource = assets_get_resource_from_id(*tile_id);
            if (!resource)
            {
                return;
            }
            
            CF_V2* scale = resource_get(resource, "scale");
            const char* sprite_name = resource_get(resource, "sprite");
            cf_htbl const char** animations = resource_get(resource, "animations");
            const char* tiled_animation_name = get_tile_animation(tile_ptr);
            const char* animation = cf_hashtable_get(animations, cf_sintern(tiled_animation_name));
            if (animation == NULL)
            {
                if (tile_ptr->elevation & 1)
                {
                    animation = cf_hashtable_get(animations, cf_sintern("short_c"));
                }
                else
                {
                    animation = cf_hashtable_get(animations, cf_sintern("tall_c"));
                }
            }
            
            CF_Sprite tile_sprite = cf_make_sprite(sprite_name);
            tile_sprite.scale = *scale;
            
            if (tile_elevation > 0)
            {
                draw_tile_stack(tile);
            }
            
            tile_sprite.transform.p = position;
            cf_sprite_play(&tile_sprite, animation);
            draw_push_sprite(Draw_Sort_Key_Type_Tile, tile, elevation, &tile_sprite);
            s_app->draw_tile_count++;
        }
    }
}

void draw_tile_outline(V2i tile)
{
    Tile* tile_ptr = get_tile(tile);
    if (tile_ptr)
    {
        f32 elevation = get_tile_total_elevation(tile);
        CF_V2 position = v2i_to_v2_iso(tile, elevation);
        
        if (!is_culled(position))
        {
            CF_Poly poly = get_tile_top_surface_poly(tile);
            
            draw_push_thickenss(1.0f);
            draw_push_polyline(Draw_Sort_Key_Type_Tile_Outline, tile, elevation, poly);
            draw_pop_thickness();
            s_app->draw_tile_count++;
        }
    }
}

void draw_tile_fill(V2i tile)
{
    Tile* tile_ptr = get_tile(tile);
    if (tile_ptr)
    {
        f32 elevation = get_tile_total_elevation(tile);
        CF_V2 position = v2i_to_v2_iso(tile, elevation);
        
        if (!is_culled(position))
        {
            CF_Poly poly = get_tile_top_surface_poly(tile);
            
            draw_push_polyline_fill(Draw_Sort_Key_Type_Tile_Fill, tile, elevation, poly);
            s_app->draw_tile_count++;
        }
    }
}

b32 is_tile_in_bounds(V2i tile)
{
    World* world = s_app->world;
    return tile.x >= 0 && tile.x < world->level.size.x && tile.y >= 0 && tile.y < world->level.size.y;
}

b32 is_tile_viewable(V2i current, V2i next)
{
    Tile* current_tile = get_tile(current);
    Tile* next_tile = get_tile(next);
    b32 is_viewable = true;
    
    if (current_tile && next_tile)
    {
        s32 elevation_delta = next_tile->elevation - current_tile->elevation;
        if (elevation_delta >= 2)
        {
            is_viewable = true;
        }
    }
    
    return is_viewable;
}

b32 is_tile_reachable(V2i current, V2i next)
{
    b32 is_reachable = false;
    
    Tile* current_tile = get_tile(current);
    Tile* next_tile = get_tile(next);
    
    if (current_tile && next_tile)
    {
        f32 current_tile_elevation = get_tile_total_elevation(current);
        f32 next_tile_elevation = get_tile_total_elevation(next);
        f32 elevation_delta = next_tile_elevation - current_tile_elevation;
        if (elevation_delta < CLIMBABLE_ELEVATION)
        {
            is_reachable = true;
        }
    }
    
    return is_reachable;
}

b32 is_tile_empty(V2i tile)
{
    b32 is_empty = true;
    
    Asset_Object_ID* tile_id = get_tile_id(tile);
    
    if (tile_id)
    {
        is_empty = *tile_id == 0;
    }
    
    return is_empty;
}

b32 is_tile_hazardous(V2i tile)
{
    b32 is_hazardous = false;
    
    Tile* tile_ptr = get_tile(tile);
    
    if (tile_ptr)
    {
        is_hazardous = !tile_ptr->walkable;
    }
    
    return is_hazardous;
}

#define MAX_ASTAR_DISTANCE 60

fixed V2i* astar(V2i start, V2i end, s32 max_distance)
{
    static cf_htbl V2i* came_from = NULL;
    static cf_htbl u8* cost_so_far = NULL;
    
    pq V2i* frontier = NULL;
    MAKE_SCRATCH_PQ(frontier, max_distance);
    pq_set_descending(frontier);
    pq_add(frontier, start, 0);
    
    cf_hashtable_set(came_from, *(u64*)&start, start);
    cf_hashtable_set(cost_so_far, *(u64*)&start, 0);
    
    b32 goal_reached = false;
    while (pq_count(frontier))
    {
        V2i current = pq_pop(frontier);
        
        if (v2i_distance(current, end) == 0)
        {
            goal_reached = true;
            break;
        }
        
        V2i neighbors[8] = 
        {
            v2i(.x = current.x - 1, .y = current.y    ),
            v2i(.x = current.x + 1, .y = current.y    ),
            v2i(.x = current.x    , .y = current.y - 1),
            v2i(.x = current.x    , .y = current.y + 1),
            
            v2i(.x = current.x - 1, .y = current.y - 1),
            v2i(.x = current.x + 1, .y = current.y - 1),
            v2i(.x = current.x - 1, .y = current.y + 1),
            v2i(.x = current.x + 1, .y = current.y + 1),
        };
        
        for (s32 index = 0; index < CF_ARRAY_SIZE(neighbors); ++index)
        {
            V2i next = neighbors[index];
            if (is_tile_in_bounds(next) && is_tile_reachable(current, next) && !is_tile_hazardous(next))
            {
                u8 new_cost = cf_hashtable_get(cost_so_far, *(u64*)&current) + 1;
                b32 inserted = false;
                u64 next_hash = *(u64*)&next;
                
                if (!cf_hashtable_has(cost_so_far, next_hash))
                {
                    cf_hashtable_set(cost_so_far, next_hash, new_cost);
                    inserted = true;
                }
                else
                {
                    u8 *cached_cost = cf_hashtable_get_ptr(cost_so_far, next_hash);
                    if (new_cost < *cached_cost)
                    {
                        *cached_cost = new_cost;
                        inserted = true;
                    }
                }
                
                if (inserted)
                {
                    s32 priority = new_cost + cf_abs_int(next.x - end.x) + cf_abs_int(next.y - end.y);
                    pq_add(frontier, next, (f32)priority);
                    V2i *cached_from = cf_hashtable_get_ptr(came_from, next_hash);
                    
                    if (cached_from)
                    {
                        *cached_from = current;
                    }
                    else
                    {
                        cf_hashtable_set(came_from, next_hash, current);
                    }
                }
            }
        }
    }
    
    fixed V2i* path = NULL;
    MAKE_SCRATCH_ARRAY(path, pq_count(frontier));
    
    s32 count = 0;
    s32 distance = 0;
    if (goal_reached)
    {
        V2i tile = end;
        while (v2i_distance(tile, start))
        {
            if (count >= pq_capacity(frontier))
            {
                goal_reached = false;
                break;
            }
            
            frontier[count++] = tile;
            tile = cf_hashtable_get(came_from, *(u64*)&tile);
        }
    }
    
    if (goal_reached)
    {
        // include the start so we can path in reverse if needed
        cf_array_push(path, start);
        if (count <= max_distance)
        {
            for (s32 index = count - 1; index >= 0; --index)
            {
                cf_array_push(path, frontier[index]);
            }
        }
    }
    
    cf_hashtable_clear(came_from);
    cf_hashtable_clear(cost_so_far);
    
    return path;
}

enum
{
    Line_Hit_Result_No_Hit,
    Line_Hit_Result_Hit_Before,
    Line_Hit_Result_Hit,
    Line_Hit_Result_Out_Of_Bounds,
};

typedef struct Line_Hit
{
    s32 hit;
    V2i tile;
} Line_Hit;

fixed V2i* make_tile_line(V2i start, V2i end)
{
    fixed V2i* tiles = NULL;
    s32 count = cf_max(v2i_distance(start, end), 1);
    MAKE_SCRATCH_ARRAY(tiles, count * 2);
    
    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    {
        s32 x0 = start.x;
        s32 y0 = start.y;
        s32 x1 = end.x;
        s32 y1 = end.y;
        s32 dx = cf_abs_int(x1 - x0);
        s32 sx = x0 < x1 ? 1 : -1;
        s32 dy = -cf_abs_int(y1 - y0);
        s32 sy = y0 < y1 ? 1 : -1;
        s32 error = dx + dy;
        
        while (1)
        {
            cf_array_push(tiles, v2i(.x = x0, .y = y0));
            
            if (x0 == x1 && y0 == y1)
            {
                break;
            }
            s32 e2 = 2 * error;
            if (e2 >= dy)
            {
                if (x0 == x1)
                {
                    break;
                }
                error = error + dy;
                x0 = x0 + sx;
            }
            
            if (e2 <= dx)
            {
                if (y0 == y1)
                {
                    break;
                }
                error = error + dx;
                y0 = y0 + sy;
            }
        }
    }
    
    return tiles;
}

// normally a ray is a position and direction,
// being in tile space having a direction being <[-1,1], [-1,1]> you 
// end up with a very restrited amount of possible lines, so here 
// you'll need to pass in a sampled end position to generate a direction
fixed V2i* make_tile_ray(V2i start, V2i end, s32 distance)
{
    V2i direction = v2i_sub(end, start);
    // technically this norm isn't a regular normal but a gcd normal
    // so distance can technically be further than 1-2, so make sure
    // to restrict the distance after making a line
    direction = v2i_norm(direction);
    V2i new_end = v2i_add(start, v2i_mul_i(direction, distance));
    fixed V2i* line = make_tile_line(start, new_end);
    if (cf_array_count(line) > distance)
    {
        cf_array_len(line) = distance;
    }
    return line;
}

Line_Hit line_cast(V2i start, V2i end)
{
    Line_Hit result = { .hit = Line_Hit_Result_No_Hit, .tile = v2i() };
    
    if (!is_tile_in_bounds(start))
    {
        result.hit = Line_Hit_Result_Out_Of_Bounds;
        return result;
    }
    
    b32 hit = false;
    s32 start_elevation = get_tile_elevation(start);
    
    V2i hit_tile = start;
    
    // bresenham line
    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    {
        s32 x0 = start.x;
        s32 y0 = start.y;
        s32 x1 = end.x;
        s32 y1 = end.y;
        s32 dx = cf_abs_int(x1 - x0);
        s32 sx = x0 < x1 ? 1 : -1;
        s32 dy = -cf_abs_int(y1 - y0);
        s32 sy = y0 < y1 ? 1 : -1;
        s32 error = dx + dy;
        
        while (1)
        {
            
            {
                V2i tile = v2i(.x = x0, .y = y0);
                s32 tile_elevation = get_tile_elevation(tile);
                s32 elevation_delta = tile_elevation - start_elevation;
                
                if (elevation_delta > 1)
                {
                    hit_tile = tile;
                    hit = true;
                    break;
                }
            }
            
            if (x0 == x1 && y0 == y1)
            {
                break;
            }
            s32 e2 = 2 * error;
            if (e2 >= dy)
            {
                if (x0 == x1)
                {
                    break;
                }
                error = error + dy;
                x0 = x0 + sx;
            }
            
            if (e2 <= dx)
            {
                if (y0 == y1)
                {
                    break;
                }
                error = error + dx;
                y0 = y0 + sy;
            }
        }
    }
    
    if (hit)
    {
        if (v2i_distance(start, end) == v2i_distance(start, hit_tile))
        {
            result.hit = Line_Hit_Result_Hit;
            result.tile = hit_tile;
        }
        else
        {
            result.hit = Line_Hit_Result_Hit_Before;
            result.tile = hit_tile;
        }
    }
    
    return result;
}

//  @todo:  cone angle, for now assume 45 degrees
fixed V2i* cone_cast(V2i start, V2i direction, s32 distance)
{
    fixed V2i* tiles = NULL;
    MAKE_SCRATCH_ARRAY(tiles, distance * distance);
    
    direction = v2i_sign(direction);
    V2i left = v2i_perp(direction);
    V2i right = v2i_neg(left);
    
    // diagonal
    if (v2i_len_sq(direction) == 2)
    {
        left = v2i(.x = -direction.x);
        right= v2i(.y = -direction.y);
    }
    
    Line_Hit result = {0};
    for (s32 index = 1; index <= distance; ++index)
    {
        V2i end = v2i_mul_i(direction, index);
        end = v2i_add(start, end);
        
        result = line_cast(start, end);
        if (result.hit == Line_Hit_Result_No_Hit)
        {
            cf_array_push(tiles, end);
        }
        
        for (s32 side = 1; side < index; ++side)
        {
            V2i left_side = v2i_mul_i(left, side);
            V2i right_side = v2i_mul_i(right, side);
            
            left_side = v2i_add(end, left_side);
            right_side = v2i_add(end, right_side);
            
            result = line_cast(start, left_side);
            if (result.hit == Line_Hit_Result_No_Hit)
            {
                cf_array_push(tiles, left_side);
            }
            
            result = line_cast(start, right_side);
            if (result.hit == Line_Hit_Result_No_Hit)
            {
                cf_array_push(tiles, right_side);
            }
        }
    }
    
    return tiles;
}

fixed V2i* circle_outline_cast(V2i start, f32 radius)
{
    fixed V2i* tiles = NULL;
    s32 count = 1;
    s32 bounds_side_length = (s32)CF_FLOORF(radius * 2 + 1);
    s32 tile_radius = (s32)(radius + 0.5f);
    count = cf_max(count, bounds_side_length * bounds_side_length);
    MAKE_SCRATCH_ARRAY(tiles, count);
    
    V2i min = v2i(.x = start.x - tile_radius, .y = start.y - tile_radius);
    V2i max = v2i(.x = start.x + tile_radius, .y = start.y + tile_radius);
    
    for (s32 y = min.y; y <= max.y; ++y)
    {
        for (s32 x = min.x; x <= max.x; ++x)
        {
            V2i tile = v2i(.x = x, .y = y);
            V2i delta = v2i_sub(tile, start);
            f32 distance = CF_SQRTF((f32)(delta.x * delta.x + delta.y * delta.y));
            if (cf_abs(distance - radius) < 0.5f)
            {
                cf_array_push(tiles, tile);
            }
        }
    }
    
    if (cf_array_count(tiles) == 0)
    {
        cf_array_push(tiles, start);
    }
    
    return tiles;
}

void world_init()
{
    World* world = cf_calloc(sizeof(World), 1);
    s_app->world = world;
    
    s32 string_size = 256 + sizeof(CF_Ahdr);
    char* buf = cf_alloc(string_size * 4);
    cf_string_static(world->level.file_name, buf, string_size);
    cf_string_static(world->level.name, buf + string_size, string_size);
    cf_string_static(world->level.music_file_name, buf + string_size * 2, string_size);
    cf_string_static(world->level.background_file_name, buf + string_size * 3, string_size);
    
    world->level.size = v2i(.x = 64, .y = 64);
    s32 size = v2i_size(LEVEL_SIZE_MAX);
    world->level.tiles = cf_calloc(sizeof(world->level.tiles[0]),  size);
    world->level.tile_ids = cf_calloc(sizeof(world->level.tile_ids[0]),  size);
    world->level.tile_elevation_offsets = cf_calloc(sizeof(world->level.tile_elevation_offsets[0]),  size);
    world->level.tile_elevation_velocity_offsets = cf_calloc(sizeof(world->level.tile_elevation_velocity_offsets[0]),  size);
    
    world->level.background = NULL;
    cf_array_fit(world->level.ai_event_queue, 1024);
    cf_array_fit(world->level.switch_links, 128);
    cf_array_fit(world->level.switch_queue, 128);
    
    CF_V2 tile_size = assets_get_tile_size();
    f32 tile_hypo = CF_SQRTF(tile_size.x * tile_size.x + tile_size.y * tile_size.y);
    f32 q_x = -tile_size.x * world->level.size.x;
    f32 q_y = -tile_hypo;
    f32 q_w = tile_size.x * world->level.size.x;
    f32 q_h = tile_hypo * world->level.size.y;
    
    world->ai_rnd = cf_rnd_seed(cf_get_tick_frequency());
    
    world->grid = entity_grid_make(LEVEL_SIZE_MAX.x, LEVEL_SIZE_MAX.y, 8);
    
    qt_rect_t bounds = qt_make_rect(q_x, q_y, q_w, q_h);
    s32 max_depth = 8;
    world->qt = qt_create(bounds, max_depth);
    
    // 16k commands
    cf_array_fit(world->draw.commands, 1024 * 8);
    cf_array_fit(world->draw.colors, 8);
    cf_array_fit(world->draw.thickness, 8);
    cf_array_fit(world->draw.layers, 8);
    
    world_clear();
    
    ecs_init();
}

typedef struct AnimationDirection
{
    const char* name;
    b32 is_flipped;
} AnimationDirection;

AnimationDirection unit_sprite_animation_direction(const char* animation, V2i direction)
{
    char buf[256];
    const char* suffix = "ne";
    b32 is_flipped = false;
    
    if (direction.x > 0)
    {
        if (direction.y < 0)
        {
            suffix = "sw";
        }
    }
    else if (direction.x < 0)
    {
        suffix = "sw";
        is_flipped = true;
    }
    else
    {
        if (direction.y < 0)
        {
            suffix = "sw";
        }
        else if (direction.y > 0)
        {
            is_flipped = true;
        }
    }
    CF_SNPRINTF(buf, sizeof(buf), "%s_%s", animation, suffix);
    AnimationDirection result = 
    {
        .name = cf_sintern(buf),
        .is_flipped = is_flipped,
    };
    return result;
}

void world_update()
{
    perf_begin("world_update");
    World* world = s_app->world;
    ecs_t* ecs = s_app->ecs;
    
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_handle_events), CF_DELTA_TIME);
    
    if (world->state == World_State_Play || world->state == World_State_Demo)
    {
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_handle_ai_events), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_pre_transform_update), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_pre_elevation_update), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_pre_health_update), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_level_elevation_offsets), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_tile_fillers), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_tile_movers), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_elevation_effectors), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_process_switch_queue), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_jumper), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_bounce_pads), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_surface_icy), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_unit_selection), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_camera_focus), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_ui_selection), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_navigation_input), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_navigation_validation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_fire_input), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_ai), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_ai_view), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_ai_view_check), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_pre_ai_navigation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_ai_navigation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_navigation_validation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_control_move_rate), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_ai_move_rate), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_prop_transforms), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_level_exits), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_switches), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_floaters), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_action_navigation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_action_fire), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_elevation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_move), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_mover_navigation), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_grid_slot), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_colliders), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_projectiles), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_projectile_hits), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_pickup_hits), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_life_times), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_healths), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_child_transforms), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_unit_sprites), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_sprites), CF_DELTA_TIME);
    }
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_demo_spectate), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_update_camera), CF_DELTA_TIME);
    
    cf_array_clear(world->control_selection_queue);
    cf_array_clear(world->control_hover_queue);
    
    perf_end("world_update");
}

void world_draw()
{
    perf_begin("world_draw");
    World* world = s_app->world;
    ecs_t* ecs = s_app->ecs;
    
    s_app->draw_tile_count = 0;
    
    cf_array_clear(world->draw.colors);
    cf_array_clear(world->draw.thickness);
    cf_array_clear(world->draw.layers);
    draw_push_color(cf_color_white());
    draw_push_thickenss(1.0f);
    draw_push_layer(0);
    
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_background), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_setup_camera), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_level_tile), CF_DELTA_TIME);
    if (world->debug.show_ai_view_cone)
    {
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_ai_view), CF_DELTA_TIME);
    }
    if (world->debug.show_pathing)
    {
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_control_preview_path), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_control_path), CF_DELTA_TIME);
        ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_unit_tile), CF_DELTA_TIME);
    }
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_control_aim), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_decals), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_unit_shadow_blobs), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_props), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_units), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_emotes), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_projectiles), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_editor), CF_DELTA_TIME);
    ecs_update_system(ecs, ECS_GET_SYSTEM_ID(system_draw_canvas_composite), CF_DELTA_TIME);
    
    draw_clear();
    
    perf_end("world_draw");
}

void world_clear()
{
    World* world = s_app->world;
    Level* level = &world->level;
    s32 size = v2i_size(LEVEL_SIZE_MAX);
    CF_MEMSET(level->tiles, 0, sizeof(level->tiles[0]) * size);
    CF_MEMSET(level->tile_ids, 0, sizeof(level->tile_ids[0]) * size);
    CF_MEMSET(level->tile_elevation_offsets, 0, sizeof(level->tile_elevation_offsets[0]) * size);
    CF_MEMSET(level->tile_elevation_velocity_offsets, 0, sizeof(level->tile_elevation_velocity_offsets[0]) * size);
    
    entity_grid_clear(world->grid);
    qt_clear(world->qt);
    qt_clean(world->qt);
    
    if (s_app->ecs)
    {
        ecs_reset(s_app->ecs);
    }
    
    audio_stop(Audio_Source_Type_Music);
    
    cf_string_clear(level->file_name);
    cf_string_clear(level->name);
    
    level->background = NULL;
    level->size = LEVEL_SIZE_MIN;
    
    cf_array_clear(level->ai_event_queue);
    cf_array_clear(level->switch_links);
    cf_array_clear(level->switch_queue);
    
    CF_MEMSET(&world->level_stats, 0, sizeof(world->level_stats));
    
    world_set_state(World_State_None);
}

void world_load_empty()
{
    world_clear();
}

void world_load_random_demo_level()
{
    static s32 level_index = 0;
    World* world = s_app->world;
    ecs_t* ecs = s_app->ecs;
    
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    
    dyna const char** levels = assets_get_resource_property_value("demo", "levels");
    if (levels && cf_array_count(levels))
    {
        level_index = cf_min(level_index, cf_array_count(levels) - 1);
        fixed ecs_id_t* spawned_entities = world_load(levels[level_index]);
        level_index = (level_index + 1) % cf_array_count(levels);
        
        for (s32 index = 0; index < cf_array_count(spawned_entities); ++index)
        {
            ecs_id_t entity = spawned_entities[index];
        }
    }
    world_set_state(World_State_Demo);
}

fixed ecs_id_t* world_load(const char* name)
{
    if (cf_string_equ(name, EDITOR_LEVEL_EMPTY_NAME))
    {
        printf("Loading editor level template\n");
        world_load_empty();
        return NULL;
    }
    
    World* world = s_app->world;
    Level* level = &world->level;
    Level_Stats* stats = &world->level_stats;
    ecs_t* ecs = s_app->ecs;
    Tile* tiles = level->tiles;
    Asset_Object_ID* tile_ids = level->tile_ids;
    
    Load_Level_Result result = load_level(name);
    
    if (!result.success)
    {
        printf("Failed to load level %s\n", name);
        world_load_empty();
        return NULL;
    }
    
    perf_begin("world_load");
    
    level->size = result.size;
    ecs_reset(ecs);
    
    fixed ecs_id_t* spawned_entities = NULL;
    MAKE_SCRATCH_ARRAY(spawned_entities, cf_array_count(result.entity_tiles));
    
    CF_MEMCPY(tiles, result.tiles, sizeof(Tile) * result.tile_count);
    
    const char* tile_layer_name = cf_sintern(EDITOR_TILE_LAYER_NAME);
    for (s32 index = 0; index < result.layer_count; ++index)
    {
        if (tile_layer_name == cf_sintern(result.layer_names[index]))
        {
            CF_MEMCPY(tile_ids, result.layers[index], sizeof(tile_ids[0]) * result.tile_count);
            break;
        }
    }
    
    if (cf_array_capacity(level->switch_links) < cf_array_count(result.switch_links))
    {
        cf_array_fit(level->switch_links, next_power_of_two(cf_array_count(result.switch_links)));
    }
    CF_MEMCPY(level->switch_links, result.switch_links, sizeof(result.switch_links[0]) * cf_array_count(result.switch_links));
    cf_array_len(level->switch_links) = cf_array_count(result.switch_links);
    
    //  @todo:  to make this faster, sort all entities that will be spawned in order
    //          of their resource id
    //          once all of the same set of entities have been spawned in
    //          grab each property then iterate through each component
    //          then set the component data
    //          afterwards run through the entity component initialization
    //          - properties
    //            - component
    //          - init
    //          this way we only hit property once then all component data
    //          that gets set are contiguous
    //          afterwards init with normal entity stuff is more trivial
    //  @note:  pico_ecs doesn't seem to have a way to grab all entities for
    //          an archetype without digging through internals
    //          maybe it's better to have an init_system() that would set up 
    //          all of these properties?
    //          or to manually create each entity here first,
    //          then through each one create the components then set each one?
    //          need to measure
    for (s32 index = 0; index < cf_array_count(result.entity_tiles); ++index)
    {
        V2i tile = result.entity_tiles[index];
        Asset_Resource* resource = assets_get_resource_from_id(result.entity_resource_ids[index]);
        ecs_id_t entity = make_entity(tile, resource->name);
        if (entity != ECS_NULL)
        {
            cf_array_push(spawned_entities, entity);
        }
    }
    
    // check to see if we have any controllable units and select first one in the
    {
        for (s32 index = 0; index < cf_array_count(spawned_entities); ++index)
        {
            ecs_id_t entity = spawned_entities[index];
            C_Control* control = ECS_GET_COMPONENT(entity, C_Control);
            if (control)
            {
                control->order = 0;
                break;
            }
        }
    }
    
    string_set(level->file_name, result.file_name);
    string_set(level->name, result.name);
    
    world_assets_load(result.music_file_name, result.background_file_name);
    world_assets_unload(level->music_file_name, level->background_file_name);
    
    string_set(level->music_file_name, result.music_file_name);
    string_set(level->background_file_name, result.background_file_name);
    if (cf_string_len(level->music_file_name))
    {
        audio_play(level->music_file_name, Audio_Source_Type_Music);
    }
    if (cf_string_len(level->background_file_name))
    {
        CF_Sprite* background = assets_get_sprite(level->background_file_name);
        if (background)
        {
            level->background = background;
        }
    }
    
    stats->level_start_time = (f32)CF_SECONDS;
    world->camera.position = v2i_to_v2_iso_center(result.camera_tile, get_tile_total_elevation(result.camera_tile));
    world->camera.next_position = world->camera.position;
    
    printf("Loaded Level %s\n", result.name);
    perf_end("world_load");
    
    return spawned_entities;
}

b32 world_is_level_loaded()
{
    return cf_string_len(s_app->world->level.name) > 0;
}

void world_reload()
{
    make_event_load_level(s_app->world->level.file_name);
}

void world_assets_load(const char* music, const char* background)
{
    if (music)
    {
        assets_load_sound(music);
    }
    if (background)
    {
        if (!assets_load_sprite(background))
        {
            assets_load_png(background);
        }
    }
}

void world_assets_unload(const char* music, const char* background)
{
    World* world = s_app->world;
    
    if (music)
    {
        assets_unload_sound(music);
    }
    if (background)
    {
        assets_unload_sprite(background);
        assets_unload_png(background);
    }
    
    cf_string_clear(world->level.music_file_name);
    cf_string_clear(world->level.background_file_name);
}

void world_set_state(World_State state)
{
    s_app->world->state = state;
}

// ---------------
// ecs
// ---------------

void ecs_init()
{
    s_app->ecs = ecs_new(MIN_ENTITIES, NULL);
    
    // setup components
    ECS_REGISTER_COMPONENT(C_Sprite, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Transform, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Child_Transform, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Unit_Transform, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Mover, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Navigation, component_navigation_constructor, component_navigation_destructor);
    ECS_REGISTER_COMPONENT(C_Action, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_AI, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_AI_Patrol, component_ai_patrol_constructor, component_ai_patrol_destructor);
    ECS_REGISTER_COMPONENT(C_AI_View, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Control, component_control_constructor, component_control_destructor);
    ECS_REGISTER_COMPONENT(C_Weapon, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Projectile, NULL, component_projectile_destructor);
    ECS_REGISTER_COMPONENT(C_Life_Time, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Camera_Focus, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Collider, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Health, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Corpse, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Elevation, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Elevation_Effector, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Decal, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Level_Exit, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Prop, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Emoter, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Emote, NULL, component_emote_destructor);
    ECS_REGISTER_COMPONENT(C_Pickup, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Sound_Source, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Asset_Resource, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Spawner, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Jumper, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_UI, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Team, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Switch, NULL, component_switch_destructor);
    ECS_REGISTER_COMPONENT(C_Tile_Filler, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Tile_Mover, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Bounce_Pad, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Surface_Icy, component_surface_icy_constructor, component_surface_icy_destructor);
    ECS_REGISTER_COMPONENT(C_Slip, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Flying, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Floater, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_Event, NULL, NULL);
    ECS_REGISTER_COMPONENT(C_AI_Event, NULL, NULL);
    
    // setup system updates
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_handle_events, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Event));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_handle_ai_events, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Team));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_pre_transform_update, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_pre_elevation_update, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_pre_health_update, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Health));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_level_elevation_offsets, system_id);
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_elevation_effectors, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation_Effector));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Life_Time));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_tile_fillers, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Tile_Filler));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_tile_movers, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Tile_Mover));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_process_switch_queue, system_id);
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_jumper, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Jumper));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_bounce_pads, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Bounce_Pad));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_surface_icy, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Surface_Icy));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_unit_selection, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_camera_focus, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Camera_Focus));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_ui_selection, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_UI));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_navigation_input, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_navigation_validation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_fire_input, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_ai, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_Patrol));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Weapon));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_ai_view, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_View));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Health));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_ai_view_check, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_Patrol));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_View));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Health));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Team));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_pre_ai_navigation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_Patrol));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_ai_navigation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_Patrol));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Weapon));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Team));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_navigation_validation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_control_move_rate, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_ai_move_rate, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_prop_transforms, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Prop));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_level_exits, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Level_Exit));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_switches, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Switch));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_floaters, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Floater));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_action_navigation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_action_fire, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Action));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Health));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Weapon));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_elevation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_move, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_mover_navigation, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        
        ecs_exclude_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Corpse));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_sprites, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Sprite));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Health));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_unit_grid_slot, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM_CB(system_update_unit_colliders, system_id, NULL, system_update_unit_colliders_on_remove);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Collider));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_projectiles, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Projectile));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Life_Time));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_projectile_hits, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Projectile));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_pickup_hits, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Pickup));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_life_times, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Life_Time));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_healths, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Health));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_child_transforms, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Child_Transform));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_sprites, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Sprite));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_demo_spectate, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Camera_Focus));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_update_camera, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Camera_Focus));
    }
    
    // setup system draws
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_background, system_id);
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_setup_camera, system_id);
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_level_tile, system_id);
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_ai_view, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_AI_View));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_control_preview_path, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_control_path, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Navigation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_control_aim, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Control));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_unit_tile, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_decals, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Sprite));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Decal));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Life_Time));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_unit_shadow_blobs, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Mover));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_props, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Sprite));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Prop));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_units, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Unit_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Sprite));
        
        ecs_exclude_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Prop));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_emotes, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Child_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Sprite));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_projectiles, system_id);
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Transform));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Projectile));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Elevation));
        ecs_require_component(s_app->ecs, system_id, ECS_GET_COMPONENT_ID(C_Life_Time));
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_editor, system_id);
    }
    {
        ecs_id_t system_id;
        ECS_REGISTER_SYSTEM(system_draw_canvas_composite, system_id);
    }
}

// ---------------
// components
// ---------------

void component_navigation_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    UNUSED(args);
    
    C_Navigation* navigation = ptr;
    cf_array_fit(navigation->path, 60);
}

void component_navigation_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    
    C_Navigation* navigation = ptr;
    cf_array_free(navigation->path);
}

void component_ai_patrol_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    UNUSED(args);
    
    C_AI_Patrol* ai_patrol = ptr;
    cf_array_fit(ai_patrol->tiles, 8);
    ai_patrol->index = 0;
}

void component_ai_patrol_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    
    C_AI_Patrol* ai_patrol = ptr;
    cf_array_free(ai_patrol->tiles);
}

void component_control_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    UNUSED(args);
    
    C_Control* control = ptr;
    game_ui_control_add(entity_id);
    control->order = CONTROL_INVALID_CONTROL;
}

void component_control_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    
    C_Control* control = ptr;
    game_ui_control_remove(entity_id);
}

void component_projectile_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    
    C_Projectile* projectile = ptr;
    cf_array_free(projectile->line);
}

void component_emote_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    World* world = s_app->world;
    C_Emote* emote = ptr;
    
    ecs_id_t component_emoter_id = ECS_GET_COMPONENT_ID(C_Emoter);
    
    if (ecs_is_ready(ecs, emote->owner))
    {
        if (ecs_get(ecs, emote->owner, component_emoter_id))
        {
            C_Emoter* emoter = ecs_get(ecs, emote->owner, component_emoter_id);
            emoter->emote_id = ECS_NULL;
        }
    }
}

void component_switch_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    UNUSED(ecs);
    
    C_Switch* c_switch = ptr;
    if (c_switch->trigger_time <= 0.0f)
    {
        make_event_on_switch(entity_id, c_switch->key);
    }
}

void component_surface_icy_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    UNUSED(args);
    
    C_Surface_Icy* surface_icy = ptr;
    pq_fit(surface_icy->touchers, 32);
    pq_set_descending(surface_icy->touchers);
}

void component_surface_icy_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr)
{
    UNUSED(ecs);
    UNUSED(entity_id);
    
    C_Surface_Icy* surface_icy = ptr;
    pq_free(surface_icy->touchers);
}

// ---------------
// system updates
// ---------------

void entity_handle_sprite_event(ecs_id_t entity, Event_Reaction_Info* event_reaction_info)
{
    C_Sprite* c_sprite = ECS_GET_COMPONENT(entity, C_Sprite);
    if (c_sprite)
    {
        cf_sprite_play(&c_sprite->sprite, event_reaction_info->sprite_reference.animation);
    }
}

void entity_handle_emoter_event(ecs_id_t entity, Event_Reaction_Info* event_reaction_info)
{
    C_Emoter* emoter = ECS_GET_COMPONENT(entity, C_Emoter);
    if (emoter)
    {
        try_emote(entity, event_reaction_info->emoter_rule);
    }
}

void entity_handle_spawner_event(ecs_id_t entity, Event_Reaction_Info* event_reaction_info)
{
    C_Unit_Transform* unit_transform = ECS_GET_COMPONENT(entity, C_Unit_Transform);
    C_Transform* transform = ECS_GET_COMPONENT(entity, C_Transform);
    C_Elevation* elevation = ECS_GET_COMPONENT(entity, C_Elevation);
    
    for (s32 index = 0; index < cf_array_count(event_reaction_info->names); ++index)
    {
        ecs_id_t spawned_entity = make_entity(unit_transform->prev_tile, event_reaction_info->names[index]);
        if (spawned_entity != ECS_NULL)
        {
            C_Transform* spawn_transform = ECS_GET_COMPONENT(spawned_entity, C_Transform);
            C_Elevation* spawn_elevation = ECS_GET_COMPONENT(spawned_entity, C_Elevation);
            
            if (transform)
            {
                spawn_transform->position = transform->position;
                spawn_transform->prev_position = transform->position;
            }
            if (elevation)
            {
                set_elevation_value(spawn_elevation, elevation->value);
            }
        }
    }
}

void entity_handle_event(ecs_id_t entity, const char* resource_name, const char* event_name)
{
    ecs_t* ecs = s_app->ecs;
    event_name = cf_sintern(event_name);
    b32 is_entity_alive = entity != ECS_NULL && ecs_is_ready(ecs, entity);
    Asset_Resource* resource = assets_get_resource(resource_name);
    if (resource == NULL)
    {
        return;
    }
    
    cf_htbl Event_Reaction_Info** event_reactions = resource_get_event_reactions(resource);
    if (event_reactions == NULL)
    {
        return;
    }
    
    // sound doesn't need a live entity since there aren't any persistent data
    // but maybe it should? maybe it needs a `can_play_while_dead` or something
    if (cf_hashtable_has(event_reactions, cf_sintern(CF_STRINGIZE(C_Sound_Source))))
    {
        C_Sound_Source* sound_source = resource_get(resource, cf_sintern(CF_STRINGIZE(C_Sound_Source)));
        if (sound_source)
        {
            cf_htbl Event_Reaction_Info* component_event_reactions = cf_hashtable_get(event_reactions, cf_sintern(CF_STRINGIZE(C_Sound_Source)));
            if (cf_hashtable_has(component_event_reactions, event_name))
            {
                Event_Reaction_Info* event_reaction_info = cf_hashtable_get_ptr(component_event_reactions, event_name);
                audio_play_random(event_reaction_info->names, sound_source->type);
            }
        }
    }
    
    // persistent sprite, animation needs to change
    if (cf_hashtable_has(event_reactions, cf_sintern(CF_STRINGIZE(C_Sprite))))
    {
        if (is_entity_alive)
        {
            cf_htbl Event_Reaction_Info* component_event_reactions = cf_hashtable_get(event_reactions, cf_sintern(CF_STRINGIZE(C_Sprite)));
            if (cf_hashtable_has(component_event_reactions, event_name))
            {
                Event_Reaction_Info* event_reaction_info = cf_hashtable_get_ptr(component_event_reactions, event_name);
                entity_handle_sprite_event(entity, event_reaction_info);
            }
        }
    }
    
    // emoter needs a transform to parent
    if (cf_hashtable_has(event_reactions, cf_sintern(CF_STRINGIZE(C_Emoter))))
    {
        if (is_entity_alive)
        {
            cf_htbl Event_Reaction_Info* component_event_reactions = cf_hashtable_get(event_reactions, cf_sintern(CF_STRINGIZE(C_Emoter)));
            if (cf_hashtable_has(component_event_reactions, event_name))
            {
                Event_Reaction_Info* event_reaction_info = cf_hashtable_get_ptr(component_event_reactions, event_name);
                entity_handle_emoter_event(entity, event_reaction_info);
            }
        }
    }
    
    if (cf_hashtable_has(event_reactions, cf_sintern(CF_STRINGIZE(C_Spawner))))
    {
        if (is_entity_alive)
        {
            cf_htbl Event_Reaction_Info* component_event_reactions = cf_hashtable_get(event_reactions, cf_sintern(CF_STRINGIZE(C_Spawner)));
            if (cf_hashtable_has(component_event_reactions, event_name))
            {
                Event_Reaction_Info* event_reaction_info = cf_hashtable_get_ptr(component_event_reactions, event_name);
                entity_handle_spawner_event(entity, event_reaction_info);
            }
        }
    }
}

ecs_ret_t system_handle_events(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    Level* level = &world->level;
    
    ecs_id_t component_event_id = ECS_GET_COMPONENT_ID(C_Event);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_corpse_id = ECS_GET_COMPONENT_ID(C_Corpse);
    ecs_id_t component_emoter_id = ECS_GET_COMPONENT_ID(C_Emoter);
    ecs_id_t component_sound_source_id = ECS_GET_COMPONENT_ID(C_Sound_Source);
    ecs_id_t component_team_id = ECS_GET_COMPONENT_ID(C_Team);
    
    const char* next_level_name = NULL;
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Event* event = ecs_get(ecs, entity, component_event_id);
        
        switch (event->type)
        {
            case Event_Type_Ground_Impact:
            {
                V2i tile = event->ground_impact.tile;
                f32 impact = event->ground_impact.impact;
                b32 ignore_start_tile = event->ground_impact.ignore_start_tile;
                f32 start_radius = 0.0f;
                f32 end_radius = impact * 0.5f;
                f32 impulse = impact * 1.0f;
                
                make_elevation_effector(tile, impulse, start_radius, end_radius, ignore_start_tile);
            }
            break;
            case Event_Type_Load_Level:
            {
                next_level_name = string_clone(event->load_level.name);
            }
            break;
            case Event_Type_On_Alert:
            {
                entity_handle_event(event->on_alert.owner, event->on_alert.owner_resource_name, "on_alert");
            }
            break;
            case Event_Type_On_Idle:
            {
                entity_handle_event(event->on_idle.owner, event->on_idle.owner_resource_name, "on_idle");
            }
            break;
            case Event_Type_On_Hit:
            {
                entity_handle_event(event->on_hit.owner, event->on_hit.owner_resource_name, "on_hit");
                entity_handle_event(event->on_hit.target, event->on_hit.target_resource_name, "on_hit_taken");
            }
            break;
            case Event_Type_On_Kill:
            {
                entity_handle_event(event->on_kill.owner, event->on_kill.owner_resource_name, "on_kill");
            }
            break;
            case Event_Type_On_Dead:
            {
                entity_handle_event(event->on_dead.owner, event->on_dead.owner_resource_name, "on_dead");
            }
            break;
            case Event_Type_On_Fire:
            {
                entity_handle_event(event->on_fire.owner, event->on_fire.owner_resource_name, "on_fire");
            }
            break;
            case Event_Type_On_Pickup:
            {
                const char* event_name = cf_sintern("on_pickup");
                
                entity_handle_event(event->on_pickup.owner, event->on_pickup.owner_resource_name, event_name);
                // since the pickup can technically be all ready destroyed at this point
                // try to just play the sound based off of the resource name
                if (event->on_pickup.asset_resource_name)
                {
                    Asset_Resource* resource = assets_get_resource(event->on_pickup.asset_resource_name);
                    C_Sound_Source* sound_source = resource_get(resource, cf_sintern(CF_STRINGIZE(C_Sound_Source)));
                    
                    cf_htbl Event_Reaction_Info** event_reactions = resource_get_event_reactions(resource);
                    if (sound_source)
                    {
                        cf_htbl Event_Reaction_Info* component_event_reactions = cf_hashtable_get(event_reactions, cf_sintern(CF_STRINGIZE(C_Sound_Source)));
                        if (cf_hashtable_has(component_event_reactions, event_name))
                        {
                            Event_Reaction_Info* event_reaction_info = cf_hashtable_get_ptr(component_event_reactions, event_name);
                            audio_play_random(event_reaction_info->names, sound_source->type);
                        }
                    }
                }
            }
            break;
            case Event_Type_On_Touch:
            {
                entity_handle_event(event->on_touch.toucher, event->on_touch.toucher_resource_name, "on_touch");
                entity_handle_event(event->on_touch.touched, event->on_touch.touched_resource_name, "on_touch");
            }
            break;
            case Event_Type_On_Switch:
            {
                entity_handle_event(event->on_switch.entity, event->on_switch.resource_name, "on_switch");
                cf_array_push(level->switch_queue, event->on_switch.tile);
            }
            break;
            case Event_Type_On_Switch_Reset:
            {
                entity_handle_event(event->on_switch_reset.entity, event->on_switch_reset.resource_name, "on_switch_reset");
            }
            break;
            case Event_Type_On_Destroy:
            {
                const char* event_name = cf_sintern("on_destroy");
                
                entity_handle_event(ECS_NULL, event->on_destroy.resource_name, event_name);
                
                if (event->on_destroy.resource_name)
                {
                    Asset_Resource* resource = assets_get_resource(event->on_destroy.resource_name);
                    C_Spawner* spawner = resource_get(resource, cf_sintern(CF_STRINGIZE(C_Spawner)));
                    
                    // handle spawner
                    cf_htbl Event_Reaction_Info** event_reactions = resource_get_event_reactions(resource);
                    if (spawner)
                    {
                        cf_htbl Event_Reaction_Info* component_event_reactions = cf_hashtable_get(event_reactions, cf_sintern(CF_STRINGIZE(C_Spawner)));
                        if (cf_hashtable_has(component_event_reactions, event_name))
                        {
                            Event_Reaction_Info* event_reaction_info = cf_hashtable_get_ptr(component_event_reactions, event_name);
                            V2i tile = event->on_destroy.tile;
                            CF_V2 position = event->on_destroy.position;
                            f32 elevation_value = event->on_destroy.elevation;
                            
                            for (s32 spawn_index = 0; spawn_index < cf_array_count(event_reaction_info->names); ++spawn_index)
                            {
                                ecs_id_t spawned_entity = make_entity(tile, event_reaction_info->names[spawn_index]);
                                if (spawned_entity != ECS_NULL)
                                {
                                    C_Transform* transform = ECS_GET_COMPONENT(spawned_entity, C_Transform);
                                    C_Elevation* elevation = ECS_GET_COMPONENT(spawned_entity, C_Elevation);
                                    
                                    if (transform)
                                    {
                                        transform->position = position;
                                        transform->prev_position = position;
                                    }
                                    if (elevation)
                                    {
                                        set_elevation_value(elevation, elevation_value);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
            case Event_Type_Do_Select_Control_Unit:
            {
                
                ecs_id_t select_entity = event->select_control_unit.entity;
                if (ecs_is_ready(ecs, select_entity) && ecs_has(ecs, select_entity, component_control_id))
                {
                    cf_array_push(world->control_selection_queue, select_entity);
                }
            }
            break;
            case Event_Type_On_Select_Control_Unit:
            {
                ecs_id_t select_entity = event->select_control_unit.entity;
                entity_handle_event(select_entity, event->select_control_unit.resource_name, "on_select");
            }
            break;
            case Event_Type_On_Deselect_Control_Unit:
            {
                ecs_id_t select_entity = event->select_control_unit.entity;
                entity_handle_event(select_entity, event->select_control_unit.resource_name, "on_deselect");
            }
            break;
            case Event_Type_On_UI_Hover_Control_Unit:
            {
                ecs_id_t select_entity = event->select_control_unit.entity;
                if (ecs_is_ready(ecs, select_entity) && ecs_has(ecs, select_entity, component_control_id))
                {
                    cf_array_push(world->control_hover_queue, select_entity);
                }
            }
            break;
            default:
            break;
        }
        
        ecs_destroy(ecs, entity);
    }
    
    if (next_level_name)
    {
        world_clear();
        world_load(next_level_name);
    }
    
    return 0;
}

ecs_ret_t system_handle_ai_events(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    dyna ecs_id_t* ai_event_queue = s_app->world->level.ai_event_queue;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    ecs_id_t component_team_id = ECS_GET_COMPONENT_ID(C_Team);
    
    ecs_id_t component_emoter_id = ECS_GET_COMPONENT_ID(C_Emoter);
    
    ecs_id_t component_ai_event_id = ECS_GET_COMPONENT_ID(C_AI_Event);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_AI* ai = ecs_get(ecs, entity, component_ai_id);
        C_Team* team = ecs_get(ecs, entity, component_team_id);
        C_Emoter* emoter = NULL;
        
        if (ecs_has(ecs, entity, component_emoter_id))
        {
            emoter = ecs_get(ecs, entity, component_emoter_id);
        }
        
        for (s32 event_index = 0; event_index < cf_array_count(ai_event_queue); ++event_index)
        {
            ecs_id_t event_entity = ai_event_queue[event_index];
            if (!ecs_is_ready(ecs, event_entity) || !ecs_has(ecs, event_entity, component_ai_event_id))
            {
                continue;
            }
            C_AI_Event* ai_event = ecs_get(ecs, event_entity, component_ai_event_id);
            
            switch (ai_event->type)
            {
                case AI_Event_Type_On_Alert:
                {
                    if (ai->target == ECS_NULL)
                    {
                        if (team->id == ai_event->on_alert.team_id)
                        {
                            if (v2i_distance(unit_transform->prev_tile, ai_event->on_alert.tile) <= ai_event->on_alert.radius)
                            {
                                ai->target = ai_event->on_alert.target;
                                make_event_on_alert(entity, ai->target);
                            }
                        }
                    }
                }
                break;
                default:
                break;
            }
        }
    }
    
    for (s32 event_index = 0; event_index < cf_array_count(ai_event_queue); ++event_index)
    {
        ecs_id_t event_entity = ai_event_queue[event_index];
        if (ecs_is_ready(ecs, event_entity))
        {
            ecs_destroy(ecs, event_entity);
        }
    }
    
    cf_array_clear(ai_event_queue);
    
    return 0;
}

ecs_ret_t system_update_pre_transform_update(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        
        transform->prev_position = transform->position;
    }
    
    return 0;
}

ecs_ret_t system_update_pre_elevation_update(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        
        elevation->prev_value = elevation->value;
    }
    
    return 0;
}

ecs_ret_t system_update_pre_health_update(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Health* health = ecs_get(ecs, entity, component_health_id);
        
        health->prev_value = health->value;
    }
    
    return 0;
}

ecs_ret_t system_update_level_elevation_offsets(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(ecs);
    UNUSED(entities);
    UNUSED(entity_count);
    UNUSED(udata);
    
    World* world = s_app->world;
    Level* level = &world->level;
    Tile* tiles = level->tiles;
    f32* offsets = level->tile_elevation_offsets;
    f32* velocities = level->tile_elevation_velocity_offsets;
    V2i level_size = level->size;
    
    f32 gravity = 10.0f;
    f32 gravity_force = gravity * dt;
    
    for (s32 y = 0; y < level_size.y; ++y)
    {
        for (s32 x = 0; x < level_size.x; ++x)
        {
            s32 index = x + y * level_size.x;
            
            if (!tiles[index].switch_is_active)
            {
                velocities[index] -= gravity_force;
            }
            
            offsets[index] = cf_max(offsets[index] + velocities[index] * dt, 0);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_tile_fillers(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    Level* level = &world->level;
    Tile* tiles = level->tiles;
    f32* offsets = level->tile_elevation_offsets;
    f32* velocities = level->tile_elevation_velocity_offsets;
    
    ecs_id_t component_tile_filler_id = ECS_GET_COMPONENT_ID(C_Tile_Filler);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Tile_Filler* tile_filler = ecs_get(ecs, entity, component_tile_filler_id);
        
        b32 is_done = false;
        
        f32 prev_delay = tile_filler->delay;
        tile_filler->delay -= dt;
        
        if (tile_filler->delay > 0)
        {
            continue;
        }
        
        if (is_tile_in_bounds(tile_filler->position))
        {
            s32 tile_index = get_tile_index(tile_filler->position);
            Tile* tile_ptr = tiles + tile_index;
            f32* velocity = velocities + tile_index;
            f32* offset = offsets + tile_index;
            
            if (prev_delay >= 0.0f && tile_filler->delay < 0)
            {
                *velocity = tile_filler->speed;
            }
            
            // only allow to fill if a switch is active
            if (tile_ptr->switch_is_active)
            {
                s32 elevation_offset = elevation_f32_to_s32(*offset);
                // going up
                if (elevation_offset > 0 && *velocity > 0)
                {
                    tile_ptr->elevation = cf_clamp_int(tile_ptr->elevation + elevation_offset, 0, cf_min(tile_filler->end_elevation, MAX_ELEVATION));
                    *offset -= elevation_s32_to_f32(elevation_offset);
                    
                    // reached goal can stop moving the tile
                    if (tile_filler->end_elevation - tile_ptr->elevation == 0)
                    {
                        tile_ptr->switch_is_active = false;
                        is_done = true;
                    }
                }
                else if (*velocity < 0)
                {
                    // going down
                    if (*offset == 0)
                    {
                        // offsets can't go below 0, so if the tile is still going down just nudge it a bit
                        s8 prev_elevation = tile_ptr->elevation;
                        tile_ptr->elevation = cf_max(tile_ptr->elevation - 1, 0);
                        if (prev_elevation != tile_ptr->elevation)
                        {
                            *offset += elevation_s32_to_f32(1);
                        }
                    }
                    
                    // reached goal can stop moving the tile
                    if (tile_filler->end_elevation - tile_ptr->elevation == 0)
                    {
                        tile_ptr->switch_is_active = false;
                        is_done = true;
                    }
                }
            }
        }
        else
        {
            is_done = true;
        }
        
        if (is_done)
        {
            destroy_entity(entity);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_tile_movers(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    Level* level = &world->level;
    Tile* tiles = level->tiles;
    f32* velocities = level->tile_elevation_velocity_offsets;
    
    ecs_id_t component_tile_mover_id = ECS_GET_COMPONENT_ID(C_Tile_Mover);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Tile_Mover* tile_mover = ecs_get(ecs, entity, component_tile_mover_id);
        
        b32 is_done = false;
        
        f32 prev_delay = tile_mover->delay;
        tile_mover->delay -= dt;
        
        if (tile_mover->delay > 0)
        {
            continue;
        }
        
        if (is_tile_in_bounds(tile_mover->position))
        {
            s32 tile_index = get_tile_index(tile_mover->position);
            Tile* tile_ptr = tiles + tile_index;
            f32* velocity = velocities + tile_index;
            
            if (prev_delay >= 0.0f && tile_mover->delay < 0)
            {
                *velocity = tile_mover->speed;
            }
            
            if (get_tile_total_elevation(tile_mover->position) >= tile_mover->end_offset)
            {
                tile_ptr->switch_is_active = false;
                is_done = true;
            }
        }
        else
        {
            is_done = true;
        }
        
        if (is_done)
        {
            destroy_entity(entity);
        }
    }
    
    return 0;
}

//  @todo:  if there ends up being too many units or things causing elevation effects to go off
//          then split this up into 2 phases
//          - solve all of the below until we reach the actual changing of velocities
//          - move all found indices to effector velocity diffs and update all velocties
ecs_ret_t system_update_elevation_effectors(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    Tile* tiles = world->level.tiles;
    f32* offsets = world->level.tile_elevation_offsets;
    f32* velocities = world->level.tile_elevation_velocity_offsets;
    
    ecs_id_t component_elevation_effector_id = ECS_GET_COMPONENT_ID(C_Elevation_Effector);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Elevation_Effector* elevation_effector = ecs_get(ecs, entity, component_elevation_effector_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Life_Time* life_time = ecs_get(ecs, entity, component_life_time_id);
        
        f32 t = 1.0f - life_time->duration / life_time->total;
        f32 radius = cf_lerp(elevation_effector->start_radius, elevation_effector->end_radius, t);
        fixed V2i* hit_tiles = circle_outline_cast(unit_transform->tile, radius);
        for (s32 tile_index = 0; tile_index < cf_array_count(hit_tiles); ++tile_index)
        {
            // only ignore starting tile if flag is set
            if (elevation_effector->ignore_start_tile && v2i_distance(unit_transform->tile, hit_tiles[tile_index]) == 0)
            {
                continue;
            }
            
            if (is_tile_in_bounds(hit_tiles[tile_index]))
            {
                // if there's no direct climbable line from effector to tile then ignore
                // this is to prevent having a elevation effector causing something like
                // a highly stacked tile to be knocked into the air when it should have
                // ignored the elevation effect
                fixed V2i* line = make_tile_line(unit_transform->tile, hit_tiles[tile_index]);
                b32 can_reach = true;
                for (s32 line_index = 1; line_index < cf_array_count(line); ++line_index)
                {
                    V2i tile_0 = line[line_index - 1];
                    V2i tile_1 = line[line_index];
                    s32 elevation_0 = get_tile_elevation(tile_0);
                    s32 elevation_1 = get_tile_elevation(tile_1);
                    
                    if ((!is_tile_empty(tile_0) && is_tile_empty(tile_1)) || 
                        cf_abs_int(elevation_0 - elevation_1) > 2)
                    {
                        can_reach = false;
                        break;
                    }
                }
                
                if (!can_reach)
                {
                    continue;
                }
                
                s32 grid_index = get_tile_index(hit_tiles[tile_index]);
                if (offsets[grid_index] == 0.0f && !tiles[grid_index].switch_is_active)
                {
                    velocities[grid_index] = velocities[grid_index] < 0 ? elevation_effector->impulse : velocities[grid_index];
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_process_switch_queue(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(entities);
    UNUSED(dt);
    UNUSED(udata);
    
    Level* level = &s_app->world->level;
    Tile* tiles = level->tiles;
    Asset_Object_ID* tile_ids = level->tile_ids;
    s32 tile_count = v2i_size(level->size);
    f32* tile_offsets = level->tile_elevation_offsets;
    f32* tile_velocities = level->tile_elevation_velocity_offsets;
    
    for (s32 index = 0; index < cf_array_count(level->switch_queue); ++index)
    {
        V2i tile = level->switch_queue[index];
        
        for (s32 switch_link_index = 0; switch_link_index < cf_array_count(level->switch_links); ++switch_link_index)
        {
            Switch_Link* switch_link = level->switch_links + switch_link_index;
            if (v2i_distance(switch_link->source, tile) == 0)
            {
                V2i region_center = aabbi_center(switch_link->region);
                
                for (s32 y = switch_link->region.min.y; y <= switch_link->region.max.y; ++y)
                {
                    for (s32 x = switch_link->region.min.x; x <= switch_link->region.max.x; ++x)
                    {
                        V2i tile = v2i(.x = x, .y = y);
                        s32 tile_index = get_tile_index(tile);
                        Tile* tile_ptr = get_tile(tile);
                        s32 elevation = switch_link->end_elevation;
                        // ignore tiles that are all ready active
                        if (tile_ptr->switch_is_active)
                        {
                            continue;
                        }
                        
                        // ignore moving tiles
                        if (tile_offsets[tile_index] != 0.0f)
                        {
                            continue;
                        }
                        
                        s32 base_elevation = elevation;
                        s32 elevation_difference = elevation - tile_ptr->elevation;
                        
                        // if tile is all ready on the same elevation ignore it
                        if (elevation_difference == 0)
                        {
                            continue;
                        }
                        
                        s32 distance_from_center = v2i_distance(tile, region_center);
                        f32 delay = 0.0f;
                        if (switch_link->state & Switch_Link_State_Bit_Cascade)
                        {
                            delay = distance_from_center * switch_link->cascade_delay;
                        }
                        
                        if (switch_link->state & Switch_Link_State_Bit_Stairs)
                        {
                            V2i delta = v2i_sub(switch_link->stairs_top, tile);
                            delta = v2i_abs(delta);
                            if (switch_link->stairs_step_rate.x)
                            {
                                delta.x = cf_max(delta.x / switch_link->stairs_step_rate.x, 0);
                            }
                            else
                            {
                                delta.x = 0;
                            }
                            if (switch_link->stairs_step_rate.y)
                            {
                                delta.y = cf_max(delta.y / switch_link->stairs_step_rate.y, 0);
                            }
                            else
                            {
                                delta.y = 0;
                            }
                            s32 elevation_offset = cf_max(delta.x, delta.y);
                            elevation = cf_max(elevation - elevation_offset, 0);
                        }
                        
                        if (switch_link->state & Switch_Link_State_Bit_Mod)
                        {
                            V2i delta = v2i_sub(region_center, tile);
                            delta = v2i_abs(delta);
                            if (switch_link->mod.x)
                            {
                                delta.x = delta.x % switch_link->mod.x;
                            }
                            if (switch_link->mod.y)
                            {
                                delta.y = delta.y % switch_link->mod.y;
                            }
                            s32 elevation_offset = cf_max(delta.x, delta.y);
                            elevation = cf_max(elevation - elevation_offset, 0);
                        }
                        
                        if (switch_link->state & Switch_Link_State_Bit_Invert)
                        {
                            b32 invertible_state = switch_link->state & Switch_Link_State_Bit_Invertible;
                            b32 invertible_cascade = switch_link->state & Switch_Link_State_Bit_Cascade;
                            
                            if (invertible_cascade)
                            {
                                s32 distance_from_min = v2i_distance(switch_link->region.min, region_center);
                                s32 distance = cf_max(distance_from_min - distance_from_center, 0);
                                delay = distance * switch_link->cascade_delay;
                            }
                            
                            // only do elevation invert if we're doing anything other than cascades
                            // if we're only doing cascades and elevation is flipped, that means elevation
                            // becomes 0 so nothing happens since all the tile mover and fillers have technically
                            // reached their goal
                            if (!(popcount(invertible_state) == 1 && invertible_cascade))
                            {
                                elevation = base_elevation - elevation;
                            }
                        }
                        
                        f32 speed = switch_link->speed * cf_sign_int(elevation_difference);
                        tile_ptr->switch_is_active = true;
                        
                        V2i position = v2i(.x = tile_index % level->size.x, .y = tile_index / level->size.x);
                        
                        if (switch_link->state & Switch_Link_State_Bit_Filler)
                        {
                            make_tile_filler(position, delay, speed, elevation);
                        }
                        else
                        {
                            make_tile_mover(position, delay, speed, elevation_s32_to_f32(elevation));
                        }
                    }
                }
            }
        }
    }
    
    cf_array_clear(level->switch_queue);
    
    return 0;
}

ecs_ret_t system_update_jumper(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    f32* offsets = world->level.tile_elevation_offsets;
    f32* velocities = world->level.tile_elevation_velocity_offsets;
    
    f32 level_start_time = world->level_stats.level_start_time;
    
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_jumper_id = ECS_GET_COMPONENT_ID(C_Jumper);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Jumper* jumper = ecs_get(ecs, entity, component_jumper_id);
        
        if (cf_on_interval(jumper->interval, level_start_time))
        {
            elevation->velocity = jumper->impulse;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_bounce_pads(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    World* world = s_app->world;
    Entity_Grid* grid = world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_bounce_pad_id = ECS_GET_COMPONENT_ID(C_Bounce_Pad);
    
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    
    ecs_id_t component_flying_id = ECS_GET_COMPONENT_ID(C_Flying);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Bounce_Pad* bounce_pad = ecs_get(ecs, entity, component_bounce_pad_id);
        
        dyna ecs_id_t* targets = entity_grid_query(grid, unit_transform->tile);
        
        for (s32 index = 0; index < cf_array_count(targets); ++index)
        {
            ecs_id_t target = targets[index];
            
            if (!ecs_is_ready(ecs, target))
            {
                continue;
            }
            
            C_Elevation* target_elevation = ecs_get(ecs, target, component_elevation_id);
            if (target_elevation->velocity <= 0 && 
                CF_FABSF(target_elevation->value - elevation->value) < ELEVATION_TOUCH_DISTANCE)
            {
                // since the only bounce the game has is along elevation, 
                // we can assume the normal to always be facing up and just
                // flip the current velocity by the bounce_pad's restitution
                f32 resitution_impulse = target_elevation->velocity * (-bounce_pad->restitution) + bounce_pad->impulse;
                target_elevation->velocity = cf_max(resitution_impulse, bounce_pad->impulse);
                
                //  @note:  due to how direction is determined below, if an entity spawns on a bounce pad
                //          it will get stuck on the bounce pad essentially until it dies
                if (ecs_has(ecs, target, component_navigation_id) && 
                    ecs_has(ecs, target, component_action_id))
                {
                    // there are 2 cases
                    //   either entity is moving pass the bounce pad then use navigation path prev->next
                    //   entity stops on bounce pad, use unit_transform->prev_tile -> unit_Transform->tile
                    C_Unit_Transform* target_unit_transform = ecs_get(ecs, target, component_unit_transform_id);
                    C_Navigation* target_navigation = ecs_get(ecs, target, component_navigation_id);
                    C_Action* target_action = ecs_get(ecs, target, component_action_id);
                    
                    V2i direction = navigation_get_direction_from_tile(target, unit_transform->prev_tile);
                    
                    cf_array_clear(target_navigation->path);
                    V2i current = unit_transform->prev_tile;
                    // target pathing stopped on the prop
                    if (v2i_distance(target_unit_transform->tile, current) == 0)
                    {
                        cf_array_push(target_navigation->path, current);
                    }
                    
                    while (true)
                    {
                        V2i next = v2i_add(current, direction);
                        if (v2i_distance(current, next) == 0)
                        {
                            break;
                        }
                        else if (!is_tile_in_bounds(next))
                        {
                            break;
                        }
                        cf_array_push(target_navigation->path, next);
                        current = next;
                    }
                    
                    target_navigation->path_index = 1;
                    target_action->apply_new_path = true;
                    
                    make_event_on_touch(target, entity);
                    
                    if (!ecs_has(ecs, target, component_flying_id))
                    {
                        ecs_add(ecs, target, component_flying_id, NULL);
                    }
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_surface_icy(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    World* world = s_app->world;
    Entity_Grid* grid = world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_surface_icy_id = ECS_GET_COMPONENT_ID(C_Surface_Icy);
    
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    
    ecs_id_t component_slip_id = ECS_GET_COMPONENT_ID(C_Slip);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Surface_Icy* surface_icy = ecs_get(ecs, entity, component_surface_icy_id);
        
        // essentially ref counting how often the surface has touched an entity, avoids repeatedly
        // setting a slide pathing over and over again causing the entity to get stuck
        for (s32 toucher_index = 0; toucher_index < pq_count(surface_icy->touchers); ++toucher_index)
        {
            pq_sub_weight_at(surface_icy->touchers, toucher_index, 1);
        }
        
        dyna ecs_id_t* targets = entity_grid_query(grid, unit_transform->tile);
        
        for (s32 index = 0; index < cf_array_count(targets); ++index)
        {
            ecs_id_t target = targets[index];
            
            if (!ecs_is_ready(ecs, target))
            {
                continue;
            }
            
            //  @todo:  wrap this up to a is_entity_touching_other_entity(), this is a repeating set of
            //          things and it's also error prone
            C_Elevation* target_elevation = ecs_get(ecs, target, component_elevation_id);
            if (target_elevation->velocity <= 0 && 
                CF_FABSF(target_elevation->value - elevation->value) < ELEVATION_TOUCH_DISTANCE)
            {
                if (ecs_has(ecs, target, component_navigation_id) && 
                    ecs_has(ecs, target, component_action_id))
                {
                    // first time interacting with entity
                    if (pq_index_of(surface_icy->touchers, target) == -1)
                    {
                        C_Unit_Transform* target_unit_transform = ecs_get(ecs, target, component_unit_transform_id);
                        C_Navigation* target_navigation = ecs_get(ecs, target, component_navigation_id);
                        C_Action* target_action = ecs_get(ecs, target, component_action_id);
                        
                        V2i direction = navigation_get_direction_from_tile(target, unit_transform->prev_tile);
                        cf_array_clear(target_navigation->path);
                        V2i current = unit_transform->prev_tile;
                        V2i next = v2i_add(current, direction);
                        
                        cf_array_push(target_navigation->path, current);
                        cf_array_push(target_navigation->path, next);
                        
                        target_navigation->path_index = 1;
                        target_action->apply_new_path = true;
                        
                        make_event_on_touch(target, entity);
                        make_event_on_slip(target, entity);
                        
                        C_Slip* slip = NULL;
                        if (!ecs_has(ecs, target, component_slip_id))
                        {
                            slip = ecs_add(ecs, target, component_slip_id, NULL);
                        }
                        else
                        { 
                            slip = ecs_get(ecs, target, component_slip_id);
                        }
                        
                        slip->duration = SLIP_DURATION;
                    }
                    
                    pq_add_weight(surface_icy->touchers, target, 1);
                }
            }
        }
        
        for (s32 toucher_index = 0; toucher_index < pq_count(surface_icy->touchers); ++toucher_index)
        {
            f32 weight = pq_weight_at(surface_icy->touchers, toucher_index);
            if (f32_is_zero(weight) || weight < 0)
            {
                pq_len(surface_icy->touchers) = toucher_index;
                break;
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_control_unit_selection(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Input* input = s_app->input;
    World* world = s_app->world;
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    
    ecs_id_t component_corpse_id = ECS_GET_COMPONENT_ID(C_Corpse);
    
    pq ecs_id_t* control_order = NULL;
    MAKE_SCRATCH_PQ(control_order, entity_count);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        
        if (control->order != CONTROL_INVALID_CONTROL)
        {
            pq_set_weight(control_order, entity, (f32)control->order);
        }
    }
    
    Aabbi aabb = make_aabbi(input->tile_select, input->tile_select);
    
    if (input->multiselect == Input_Multiselect_State_Commit)
    {
        aabb = make_aabbi(input->tile_select_start, input->tile_select_end);
    }
    
    if (input->select)
    {
        s32 control_count = pq_count(control_order);
        
        // this needs to be checked in 2 ways
        // - if we're selecting a range of units that are not in the
        //   pre-selected units, then this is an ADD command
        // - if we're selecting a range or units that are ONLY in the
        //   pre-selected units, then this is a REMOVE command
        
        // multiple unit selection
        if (input->multiselect == Input_Multiselect_State_Commit)
        {
            // check for units to add
            for (s32 index = 0; index < entity_count; ++index)
            {
                ecs_id_t entity = entities[index];
                C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
                
                if (aabbi_contains(aabb, unit_transform->prev_tile))
                {
                    pq_add_weight(control_order, entity, (f32)control_count);
                }
            }
            
            // if it's the same then we've tried to select same units
            // remove those units from selection list
            if (pq_count(control_order) == control_count)
            {
                for (s32 index = pq_count(control_order) - 1; index >= 0; --index)
                {
                    ecs_id_t entity = control_order[index];
                    
                    if (pq_weight_at(control_order, index) >= control_count)
                    {
                        pq_pop(control_order);
                        make_event_on_deselect_control_unit(entity);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                // added some units
                for (s32 index = 0; index < pq_count(control_order); ++index)
                {
                    ecs_id_t entity = control_order[index];
                    C_Control* control = ecs_get(ecs, entity, component_control_id);
                    
                    if (control->is_locked)
                    {
                        continue;
                    }
                    
                    if (control->order == CONTROL_INVALID_CONTROL)
                    {
                        make_event_on_select_control_unit(entity);
                    }
                }
            }
        }
        else if (input->multiselect == Input_Multiselect_State_None)
        {
            // single unit selection
            for (s32 index = 0; index < entity_count; ++index)
            {
                ecs_id_t entity = entities[index];
                C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
                
                if (aabbi_contains(aabb, unit_transform->prev_tile))
                {
                    pq_clear(control_order);
                    pq_add(control_order, entity, 0);
                    make_event_on_select_control_unit(entity);
                    break;
                }
            }
        }
    }
    
    if (input->is_holding_add_remove)
    {
        for (s32 index = 0; index < cf_array_count(world->control_selection_queue); ++index)
        {
            ecs_id_t entity = world->control_selection_queue[index];
            C_Control* control = ecs_get(ecs, entity, component_control_id);
            if (ecs_is_ready(ecs, entity) && !ecs_has(ecs, entity, component_corpse_id))
            {
                if (control->is_locked)
                {
                    continue;
                }
                
                if (control->order != CONTROL_INVALID_CONTROL)
                {
                    s32 control_index = pq_index_of(control_order, entity);
                    if (control_index != -1)
                    {
                        pq_remove_at(control_order, control_index);
                        make_event_on_deselect_control_unit(entity);
                    }
                }
                else
                {
                    pq_set_weight(control_order, entity, (f32)pq_count(control_order));
                    make_event_on_select_control_unit(entity);
                }
            }
        }
    }
    else
    {
        // pick first one out of the queue that's alive
        for (s32 index = 0; index < cf_array_count(world->control_selection_queue); ++index)
        {
            ecs_id_t entity = world->control_selection_queue[index];
            if (ecs_is_ready(ecs, entity) && !ecs_has(ecs, entity, component_corpse_id))
            {
                C_Control* control = ecs_get(ecs, entity, component_control_id);
                
                if (control->is_locked)
                {
                    continue;
                }
                
                pq_clear(control_order);
                pq_add(control_order, entity, 0);
                make_event_on_select_control_unit(entity);
                break;
            }
        }
    }
    
    // remove all control units first before selecting new one
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        control->order = CONTROL_INVALID_CONTROL;
    }
    
    //  @note:  this step is done outside mainly due to both single and multiselect
    //          acts similar but also incase a unit is destroyed you still want to
    //          control the next leader rather than be stuck not controlling anyone
    //          and the followers are just standing there
    for (s32 index = 0; index < pq_count(control_order); ++index)
    {
        C_Control* control = ecs_get(ecs, control_order[index], component_control_id);
        
        if (control->is_locked)
        {
            continue;
        }
        
        control->order = index;
    }
    
    return 0;
}

ecs_ret_t system_update_control_camera_focus(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_camera_focus_id = ECS_GET_COMPONENT_ID(C_Camera_Focus);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        C_Camera_Focus* camera_focus = ecs_get(ecs, entity, component_camera_focus_id);
        
        if (control->order != 0)
        {
            *camera_focus = 0;
        }
        else
        {
            *camera_focus = 1;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_control_ui_selection(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    Input* input = s_app->input;
    
    dyna ecs_id_t* control_hover_queue = world->control_hover_queue;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_ui_id = ECS_GET_COMPONENT_ID(C_UI);
    
    Aabbi aabb = make_aabbi(input->tile_select, input->tile_select);
    if (input->multiselect != Input_Multiselect_State_None)
    {
        aabb = make_aabbi(input->tile_select_start, input->tile_select_end);
    }
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        C_UI* c_ui = ecs_get(ecs, entity, component_ui_id);
        
        if (control->is_locked)
        {
            c_ui->current = &c_ui->deselect;
            continue;
        }
        
        b32 is_ui_hovered = false;
        for (s32 hover_index = 0; hover_index < cf_array_count(control_hover_queue); ++hover_index)
        {
            if (entity == control_hover_queue[hover_index])
            {
                is_ui_hovered = true;
                break;
            }
        }
        
        // do any hover to let the user know these units are highlighted
        if (aabbi_contains(aabb, unit_transform->prev_tile) || is_ui_hovered)
        {
            c_ui->current = &c_ui->hover;
        }
        else if (control->order == 0)
        {
            c_ui->current = &c_ui->leader;
        }
        else if (control->order > 0)
        {
            c_ui->current = &c_ui->select;
        }
        else
        {
            c_ui->current = &c_ui->deselect;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_control_navigation_input(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Input* input = s_app->input;
    V2i goal = input->tile_select;
    b32 is_input_tile_in_bounds = is_tile_in_bounds(goal);
    b32 was_digital_input = v2i_len_sq(input->prev_move_direction) > 0;
    b32 is_digital_input = v2i_len_sq(input->move_direction) > 0;
    
    b32 try_move = input->move;
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    
    b32 stop_moving_leader = false;
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        
        if (control->order == 0)
        {
            if (is_digital_input)
            {
                goal = v2i_add(unit_transform->tile, input->move_direction);
            }
            else if (was_digital_input)
            {
                goal = unit_transform->tile;
                stop_moving_leader = true;
            }
            
            break;
        }
    }
    
    if (is_input_tile_in_bounds)
    {
        for (s32 index = 0; index < entity_count; ++index)
        {
            ecs_id_t entity = entities[index];
            C_Control* control = ecs_get(ecs, entity, component_control_id);
            C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
            C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
            C_Action* action = ecs_get(ecs, entity, component_action_id);
            
            control->preview_path = NULL;
            
            if (control->is_locked)
            {
                cf_array_clear(navigation->path);
                control->order = CONTROL_INVALID_CONTROL;
                continue;
            }
            
            if (control->order == CONTROL_INVALID_CONTROL)
            {
                continue;
            }
            
            //  if path is going back to a unit_transform->prev_tile then it should only move back once if possible
            //  such as cases where the unit fires but doesn't fully move off of the previous tile
            //  @todo:  might include Mover here also to only allow if move_time is less than some threshold, to
            //          prevent a control from hugging a tile as a vantage point
            s32 goal_distance = v2i_distance(goal, unit_transform->prev_tile);
            if (goal_distance == 0)
            {
                MAKE_SCRATCH_ARRAY(control->preview_path, 1);
                cf_array_push(control->preview_path, unit_transform->prev_tile);
            }
            else
            {
                V2i start = unit_transform->tile;
                V2i end = goal;
                control->preview_path = astar(start, end, MAX_ASTAR_DISTANCE);
            }
            
            if ((try_move || is_digital_input) && cf_array_count(control->preview_path) > 1)
            {
                navigation_set_path(entity, control->preview_path);
            }
            
            if (control->order == 0 && stop_moving_leader)
            {
                navigation_set_path(entity, NULL);
            }
        }
    }
    else
    {
        for (s32 index = 0; index < entity_count; ++index)
        {
            ecs_id_t entity = entities[index];
            C_Control* control = ecs_get(ecs, entity, component_control_id);
            
            control->preview_path = NULL;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_control_navigation_validation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    
    pq ecs_id_t* control_order = NULL;
    MAKE_SCRATCH_PQ(control_order, entity_count);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        
        if (control->order != CONTROL_INVALID_CONTROL)
        {
            pq_set_weight(control_order, entity, (f32)control->order);
        }
    }
    
    for (s32 index = 1; index < pq_count(control_order); ++index)
    {
        ecs_id_t entity = control_order[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        C_Action* action = ecs_get(ecs, entity, component_action_id);
        
        if (action->apply_new_path)
        {
            b32 changed = true;
            while (changed)
            {
                changed = false;
                for (s32 link_index = 0; link_index < index; ++link_index)
                {
                    ecs_id_t link_entity = control_order[link_index];
                    C_Unit_Transform* link_unit_transform = ecs_get(ecs, link_entity, component_unit_transform_id);
                    C_Navigation* link_navigation = ecs_get(ecs, link_entity, component_navigation_id);
                    
                    if (cf_array_count(navigation->path) == 0)
                    {
                        break;
                    }
                    
                    V2i goal = cf_array_last(navigation->path);
                    V2i link_goal = cf_array_last(link_navigation->path);
                    
                    if (cf_array_count(link_navigation->path) == 0)
                    {
                        link_goal = link_unit_transform->prev_tile;
                    }
                    
                    if (v2i_distance(goal, link_goal) == 0)
                    {
                        cf_array_pop(navigation->path);
                        changed = true;
                    }
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_control_fire_input(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Input* input = s_app->input;
    CF_V2 world_mouse = input->world_mouse;
    V2i tile_select = input->tile_select;
    
    b32 try_fire = input->fire;
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    
    if (try_fire)
    {
        if (cf_len_sq(input->aim_direction) > 0)
        {
            for (s32 index = 0; index < entity_count; ++index)
            {
                ecs_id_t entity = entities[index];
                C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
                C_Control* control = ecs_get(ecs, entity, component_control_id);
                
                if (control->order == 0)
                {
                    world_mouse = cf_add_v2(transform->position, input->aim_direction);
                    tile_select = get_tile_from_input_cursor(world_mouse, false);
                    break;
                }
            }
        }
        
        for (s32 index = 0; index < entity_count; ++index)
        {
            ecs_id_t entity = entities[index];
            C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
            C_Control* control = ecs_get(ecs, entity, component_control_id);
            C_Action* action = ecs_get(ecs, entity, component_action_id);
            
            C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
            
            if (control->order == CONTROL_INVALID_CONTROL)
            {
                continue;
            }
            
            V2i fire_tile = get_tile_from_input_cursor(world_mouse, false);
            // sampling failed to try to find next closest one from sampling or tile select
            if (fire_tile.x == -1 && fire_tile.y == -1)
            {
                fire_tile = screen_v2_to_v2i(world_mouse);
                CF_V2 world_fire_tile = v2i_to_v2_iso_center(fire_tile, get_tile_total_elevation(fire_tile));
                CF_V2 world_tile_select = v2i_to_v2_iso_center(tile_select, get_tile_total_elevation(tile_select));
                
                if (cf_distance(world_mouse, world_fire_tile) > cf_distance(world_mouse, world_tile_select))
                {
                    // mouse is closer to tile select
                    fire_tile = tile_select;
                }
            }
            
            action->fire_tile = fire_tile;
            action->try_fire = true;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_ai(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    ecs_id_t component_ai_patrol_id = ECS_GET_COMPONENT_ID(C_AI_Patrol);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    ecs_id_t component_weapon_id = ECS_GET_COMPONENT_ID(C_Weapon);
    
    // target
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_AI* ai = ecs_get(ecs, entity, component_ai_id);
        C_AI_Patrol* ai_patrol = ecs_get(ecs, entity, component_ai_patrol_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Action* action = ecs_get(ecs, entity, component_action_id);
        C_Weapon* weapon = ecs_get(ecs, entity, component_weapon_id);
        
        ai->can_aim_at_target = false;
        
        b32 clear_target = true;
        // only fire if target is alive, otherwise signal to restart patrol
        if (ai->target != ECS_NULL)
        {
            if (ecs_is_ready(ecs, ai->target))
            {
                if (ecs_has(ecs, ai->target, component_transform_id) && 
                    ecs_has(ecs, ai->target, component_unit_transform_id) &&
                    ecs_has(ecs, ai->target, component_health_id))
                {
                    C_Unit_Transform* target_unit_transform = ecs_get(ecs, ai->target, component_unit_transform_id);
                    C_Health* target_health = ecs_get(ecs, ai->target, component_health_id);
                    
                    s32 distance = v2i_distance(unit_transform->tile, target_unit_transform->tile);
                    if (target_health->value > 0)
                    {
                        clear_target = distance >= ai->disengage_distance;
                        ai->can_aim_at_target = distance <= ai->aim_distance;
                        if (ai->can_aim_at_target)
                        {
                            Line_Hit hit_result = line_cast(unit_transform->prev_tile, target_unit_transform->prev_tile);
                            if (hit_result.hit == Line_Hit_Result_Hit_Before)
                            {
                                ai->can_aim_at_target = false;
                            }
                        }
                        
                        if (!clear_target)
                        {
                            action->try_fire = ai->can_aim_at_target;
                            if (action->try_fire)
                            {
                                action->fire_tile = target_unit_transform->prev_tile;
                            }
                        }
                    }
                }
            }
        }
        
        if (clear_target)
        {
            ai->target = ECS_NULL;
            ai_patrol->restart_path = true;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_ai_view(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_ai_view_id = ECS_GET_COMPONENT_ID(C_AI_View);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_AI_View* ai_view = ecs_get(ecs, entity, component_ai_view_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Health* health = ecs_get(ecs, entity, component_health_id);
        
        ai_view->tiles = NULL;
        
        if (health->value > 0)
        {
            V2i start = unit_transform->prev_tile;
            
            ai_view->tiles = cone_cast(start, unit_transform->direction, ai_view->distance);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_ai_view_check(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Entity_Grid* grid = s_app->world->grid;
    
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    ecs_id_t component_ai_patrol_id = ECS_GET_COMPONENT_ID(C_AI_Patrol);
    ecs_id_t component_ai_view_id = ECS_GET_COMPONENT_ID(C_AI_View);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    ecs_id_t component_team_id = ECS_GET_COMPONENT_ID(C_Team);
    
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_AI* ai = ecs_get(ecs, entity, component_ai_id);
        C_AI_Patrol* ai_patrol = ecs_get(ecs, entity, component_ai_patrol_id);
        C_AI_View* ai_view = ecs_get(ecs, entity, component_ai_view_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Health* health = ecs_get(ecs, entity, component_health_id);
        C_Team* team = ecs_get(ecs, entity, component_team_id);
        
        V2i start = unit_transform->prev_tile;
        
        s32 closest_distance = S32_MAX;
        ecs_id_t* closest_alert_target = NULL;
        
        if (health->value <= 0)
        {
            continue;
        }
        
        // only consider living control entities
        for (s32 tile_index = 0; tile_index < cf_array_count(ai_view->tiles); ++tile_index)
        {
            V2i tile = ai_view->tiles[tile_index];
            dyna ecs_id_t* targets = entity_grid_query(grid, tile);
            s32 tile_distance = v2i_distance(start, tile);
            for (s32 target_index = 0; target_index < cf_array_count(targets); ++target_index)
            {
                ecs_id_t* target = targets + target_index;
                if (*target != entity)
                {
                    if (ecs_is_ready(ecs, *target))
                    {
                        if (ecs_has(ecs, *target, component_team_id) &&
                            ecs_has(ecs, *target, component_health_id))
                        {
                            C_Team* target_team = ecs_get(ecs, *target, component_team_id);
                            C_Health* target_health = ecs_get(ecs, *target, component_health_id);
                            
                            if (team->id == target_team->id)
                            {
                                continue;
                            }
                            
                            if (target_health->value > 0 && closest_distance > tile_distance)
                            {
                                closest_distance = tile_distance;
                                closest_alert_target = target;
                            }
                        }
                    }
                }
            }
        }
        
        if (closest_alert_target)
        {
            if (ai->target == ECS_NULL)
            {
                make_ai_event_on_alert(team->id, *closest_alert_target, unit_transform->prev_tile, ai->alert_radius);
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_pre_ai_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform_Patrol);
    ecs_id_t component_ai_patrol_id = ECS_GET_COMPONENT_ID(C_AI_Patrol);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_AI_Patrol* ai_patrol = ecs_get(ecs, entity, component_ai_patrol_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        // check if path is still reachable and isn't a hazard
        if (navigation->path_index < cf_array_count(navigation->path))
        {
            V2i tile = navigation->path[navigation->path_index];
            
            if (is_tile_hazardous(tile) || !is_tile_reachable(unit_transform->prev_tile, tile))
            {
                ai_patrol->restart_path = true;
            }
        }
        else
        {
            // path is finished setup to find a new one
            ai_patrol->force_new_path = true;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_ai_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    ecs_id_t component_ai_patrol_id = ECS_GET_COMPONENT_ID(C_AI_Patrol);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    ecs_id_t component_weapon_id = ECS_GET_COMPONENT_ID(C_Weapon);
    ecs_id_t component_team_id = ECS_GET_COMPONENT_ID(C_Team);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_AI* ai = ecs_get(ecs, entity, component_ai_id);
        C_AI_Patrol* ai_patrol = ecs_get(ecs, entity, component_ai_patrol_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        C_Action* action = ecs_get(ecs, entity, component_action_id);
        C_Weapon* weapon = ecs_get(ecs, entity, component_weapon_id);
        C_Team* team = ecs_get(ecs, entity, component_team_id);
        
        V2i* target_tile = NULL;
        b32 entered_next_patrol_path = false;
        b32 try_new_path = cf_array_count(ai_patrol->tiles) > 0;
        
        // try to find target if target exists
        if (ai->target != ECS_NULL)
        {
            if (ecs_is_ready(ecs, ai->target))
            {
                if (ecs_has(ecs, ai->target, component_unit_transform_id))
                {
                    C_Unit_Transform* target_unit_transform = ecs_get(ecs, ai->target, component_unit_transform_id);
                    target_tile = &target_unit_transform->tile;
                }
            }
        }
        
        // try to path to target
        if (target_tile)
        {
            // can't see the target or not close enough, try to move closer
            if (!ai->can_aim_at_target)
            {
                V2i start = unit_transform->tile;
                V2i end = *target_tile;
                fixed V2i* path = astar(start, end, MAX_ASTAR_DISTANCE);
                
                if (navigation_set_path(entity, path))
                {
                    // only go up next to the target
                    cf_array_pop(navigation->path);
                }
            }
            else if (f32_is_zero(weapon->fire_delay - weapon->fire_rate))
            {
                if (navigation_can_set_path(entity))
                {
                    // just fired, try to wiggle
                    setup_ai_patrol(ai_patrol, unit_transform->prev_tile, 1);
                    try_new_path = cf_array_count(ai_patrol->tiles) > 0;
                    cf_array_clear(navigation->path);
                }
            }
            
            if (navigation->path_index < cf_array_count(navigation->path))
            {
                try_new_path = false;
            }
        }
        
        if (try_new_path)
        {
            // try to continue patrol
            V2i patrol_tile = ai_patrol->tiles[ai_patrol->index];
            V2i start = unit_transform->tile;
            V2i end = patrol_tile;
            fixed V2i* path = NULL;
            
            if (ai_patrol->force_new_path)
            {
                // try to continue old patrol route or new one if finished that route
                // only keep patrol tiles if it's actually reachable
                s32 iterations = 0;
                while (cf_array_count(path) == 0 && iterations < cf_array_count(ai_patrol->tiles))
                {
                    ai_patrol->index = (ai_patrol->index + 1) % cf_array_count(ai_patrol->tiles);
                    start = unit_transform->tile;
                    end = ai_patrol->tiles[ai_patrol->index];
                    path = astar(start, end, MAX_ASTAR_DISTANCE);
                    ++iterations;
                }
                
                // check if path isn't stepping back and forth
                if (cf_array_count(path) > 1)
                {
                    entered_next_patrol_path = true;
                    ai_patrol->force_new_path = false;
                }
                else
                {
                    // failed to generate any new paths to current patrol route. try to generate new ones
                    setup_ai_patrol(ai_patrol, unit_transform->prev_tile, ai_patrol->patrol_distance);
                }
            }
            else if (ai_patrol->restart_path)
            {
                // restarting path due to any other reason like patrol stopping due to attacking another unit
                path = astar(start, end, MAX_ASTAR_DISTANCE);
                // failed to restart path, try to start a new one in patrol rotation
                if (!path)
                {
                    ai_patrol->force_new_path = true;
                }
            }
            
            if (path)
            {
                if (navigation_set_path(entity, path))
                {
                    ai_patrol->restart_path = false;
                }
            }
        }
        
        if (entered_next_patrol_path)
        {
            // either aiming or moving towards a target
            if (target_tile)
            {
                make_ai_event_on_alert(team->id, ai->target, unit_transform->prev_tile, ai->alert_radius);
            }
            else
            {
                make_event_on_idle(entity);
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_unit_navigation_validation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    
    ecs_id_t component_slip_id = ECS_GET_COMPONENT_ID(C_Slip);
    ecs_id_t component_flying_id = ECS_GET_COMPONENT_ID(C_Flying);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        
        // flying validation
        if (ecs_has(ecs, entity, component_flying_id))
        {
            if (!elevation_is_grounded(elevation))
            {
                if (navigation->path_index < cf_array_count(navigation->path))
                {
                    V2i p = navigation->path[navigation->path_index];
                    Tile* tile_ptr = get_tile(p);
                    f32 p_elevation = get_tile_total_elevation(p);
                    f32 next_velocity = elevation->velocity - ELEVATION_GRAVITY * dt;
                    f32 next_elevation = elevation->value + next_velocity * dt;
                    
                    if (p_elevation > next_elevation && (p_elevation - next_elevation) > CLIMBABLE_ELEVATION)
                    {
                        // hit a wall
                        //  @todo:  removed tile->stackless check here until we can
                        //          figure out what best way to handle units passing
                        //          through stackless tiles. this was causing
                        //          units to gain immediate elevation followed by
                        //          flying off further than it should
                        cf_array_clear(navigation->path);
                    }
                    else if (next_elevation < p_elevation && elevation->value > p_elevation)
                    {
                        cf_array_len(navigation->path) = cf_min(navigation->path_index + 1, cf_array_len(navigation->path) - 1);
                    }
                }
            }
            else
            {
                ecs_remove(ecs, entity, component_flying_id);
                cf_array_clear(navigation->path);
            }
            
            continue;
        }
        
        // check if still slipping
        if (ecs_has(ecs, entity, component_slip_id))
        {
            s32 wall_test_index = -1;
            C_Slip* slip = ecs_get(ecs, entity, component_slip_id);
            slip->duration -= dt;
            
            if (slip->duration <= 0)
            {
                ecs_remove(ecs, entity, component_slip_id);
            }
            
            // finished slipping continue normally
            if (navigation->path_index < cf_array_count(navigation->path))
            {
                wall_test_index = navigation->path_index;
            }
            else
            {
                wall_test_index = cf_array_count(navigation->path) - 1;
            }
            
            if (wall_test_index >= 0)
            {
                V2i p = navigation->path[wall_test_index];
                f32 p_elevation = get_tile_total_elevation(p);
                // hit a wall
                if (p_elevation - elevation->value > CLIMBABLE_ELEVATION)
                {
                    cf_array_clear(navigation->path);
                }
            }
            
            continue;
        }
        
        // grounded validation
        fixed V2i* path = NULL;
        b32 force_restart_path = false;
        
        for (s32 path_index = navigation->path_index; path_index < cf_array_count(navigation->path) - 1; ++path_index)
        {
            V2i p0 = navigation->path[path_index];
            V2i p1 = navigation->path[path_index + 1];
            
            if (is_tile_hazardous(p0) || is_tile_hazardous(p0) || !is_tile_reachable(p0, p1))
            {
                force_restart_path = true;
                break;
            }
        }
        
        if (force_restart_path)
        {
            V2i goal = cf_array_last(navigation->path);
            if (!is_tile_empty(goal))
            {
                path = astar(unit_transform->prev_tile, goal, MAX_ASTAR_DISTANCE);
            }
            else
            {
                cf_array_len(navigation->path) = cf_min(navigation->path_index, cf_array_count(navigation->path));
            }
        }
        
        if (path)
        {
            navigation->path_index = 1;
            cf_array_set(navigation->path, path);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_control_move_rate(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    
    ecs_id_t component_slip_id = ECS_GET_COMPONENT_ID(C_Slip);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        
        if (ecs_has(ecs, entity, component_slip_id))
        {
            mover->move_rate = SLIP_MOVE_RATE;
        }
        else
        {
            mover->move_rate = control->move_rate;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_ai_move_rate(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    
    ecs_id_t component_slip_id = ECS_GET_COMPONENT_ID(C_Slip);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_AI* ai = ecs_get(ecs, entity, component_ai_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        
        if (ecs_has(ecs, entity, component_slip_id))
        {
            mover->move_rate = SLIP_MOVE_RATE;
        }
        else
        {
            
            if (ai->target != ECS_NULL)
            {
                mover->next_move_rate = ai->chase_move_rate;
                if (ai->can_aim_at_target)
                {
                    mover->next_move_rate = ai->aim_move_rate;
                }
            }
            else
            {
                mover->next_move_rate = ai->patrol_move_rate;
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_prop_transforms(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Entity_Grid* grid = s_app->world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_prop_id = ECS_GET_COMPONENT_ID(C_Prop);
    UNUSED(component_prop_id);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        
        CF_V2 position = v2i_to_v2_iso_center(unit_transform->tile, elevation->value);
        transform->position = position;
    }
    
    return 0;
}

ecs_ret_t system_update_level_exits(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    World* world = s_app->world;
    Entity_Grid* grid = world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_level_exit_id = ECS_GET_COMPONENT_ID(C_Level_Exit);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    
    pq ecs_id_t* closest_targets = NULL;
    MAKE_SCRATCH_PQ(closest_targets, 32);
    
    s32 activated_count = 0;
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Level_Exit* level_exit = ecs_get(ecs, entity, component_level_exit_id);
        
        if (level_exit->activated)
        {
            ++activated_count;
            continue;
        }
        
        dyna ecs_id_t* targets = entity_grid_query(grid, unit_transform->tile);
        
        pq_clear(closest_targets);
        get_closest_target_elevation_touches(ecs, targets, elevation->value, closest_targets);
        
        for (s32 target_index = 0; target_index < pq_count(closest_targets); ++target_index)
        {
            if (ecs_has(ecs, closest_targets[target_index], component_control_id))
            {
                ecs_id_t closest_target = closest_targets[target_index];
                
                C_Control* control = ecs_get(ecs, closest_target, component_control_id);
                control->is_locked = true;
                
                if (ecs_has(ecs, closest_target, component_health_id))
                {
                    C_Health* health = ecs_get(ecs, closest_target, component_health_id);
                    health->is_invulnerable = true;
                }
                
                level_exit->activated = true;
                
                make_event_on_touch(closest_target, entity);
                break;
            }
        }
    }
    
    world->level_stats.activated_exits = activated_count;
    world->level_stats.total_exits = entity_count;
    
    return 0;
}

ecs_ret_t system_update_switches(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    World* world = s_app->world;
    Entity_Grid* grid = world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_switch_id = ECS_GET_COMPONENT_ID(C_Switch);
    
    pq ecs_id_t* closest_targets = NULL;
    MAKE_SCRATCH_PQ(closest_targets, 32);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Switch* c_switch = ecs_get(ecs, entity, component_switch_id);
        
        c_switch->prev_activation_count = c_switch->activation_count;
        
        f32 prev_trigger_time = c_switch->trigger_time;
        c_switch->trigger_time = cf_max(c_switch->trigger_time - dt, 0.0f);
        
        // still waiting since last trigger
        if (c_switch->trigger_time > 0.0f)
        {
            continue;
        }
        
        if (prev_trigger_time > 0 && c_switch->trigger_time == 0.0f)
        {
            make_event_on_switch_reset(entity);
        }
        
        // the rest of this is a touch check
        if (!c_switch->trigger_on_touch)
        {
            continue;
        }
        
        dyna ecs_id_t* targets = entity_grid_query(grid, unit_transform->tile);
        
        pq_clear(closest_targets);
        get_closest_target_elevation_touches(ecs, targets, elevation->value, closest_targets);
        
        // only allow lever to be touched if it hasn't been touched recently or has been sat on
        if (c_switch->last_touch == ECS_NULL)
        {
            for (s32 target_index = 0; target_index < pq_count(closest_targets); ++target_index)
            {
                ecs_id_t target = closest_targets[target_index];
                if (ecs_is_ready(ecs, target))
                {
                    c_switch->last_touch = target;
                    c_switch->trigger_time = c_switch->reset_time;
                    ++c_switch->activation_count;
                    make_event_on_touch(target, entity);
                    make_event_on_switch(entity, c_switch->key);
                    break;
                }
            }
        }
        
        if (pq_count(closest_targets) == 0)
        {
            c_switch->last_touch = ECS_NULL;
        }
    }
    
    return 0;
}

ecs_ret_t system_update_floaters(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    World* world = s_app->world;
    Entity_Grid* grid = world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_floater_id = ECS_GET_COMPONENT_ID(C_Floater);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Floater* floater = ecs_get(ecs, entity, component_floater_id);
        
        // make sure to keep the air drop at the same elevation
        f32 tile_elevation = get_tile_elevation_f32(unit_transform->prev_tile);
        tile_elevation += elevation_s32_to_f32(floater->elevation_offset);
        set_elevation_value(elevation, tile_elevation);
        elevation->velocity = 0;
    }
    
    return 0;
}

ecs_ret_t system_update_unit_action_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        C_Action* action = ecs_get(ecs, entity, component_action_id);
        
        if (action->apply_new_path)
        {
            navigation->path_index = 1;
            action->apply_new_path = false;
            mover->is_pathing = true;
            // prevents unit from getting stuck on a tile during inital new pathfind since there's
            // an edge case where when a new path is set the mover->move_time would all ready be at
            // mover->move_rate which causes the unit to stall
            if (v2i_distance(unit_transform->prev_tile, unit_transform->tile) == 0)
            {
                mover->move_time = 0.0f;
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_unit_action_fire(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    CF_V2 tile_size = assets_get_tile_size();
    f32 tile_length = cf_len(tile_size);
    
    CF_Rnd *ai_rnd = &s_app->world->ai_rnd;
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    ecs_id_t component_action_id = ECS_GET_COMPONENT_ID(C_Action);
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    ecs_id_t component_weapon_id = ECS_GET_COMPONENT_ID(C_Weapon);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        C_Action* action = ecs_get(ecs, entity, component_action_id);
        C_Health* health = ecs_get(ecs, entity, component_health_id);
        C_Weapon* weapon = ecs_get(ecs, entity, component_weapon_id);
        
        weapon->fire_delay -= dt;
        b32 can_fire = weapon->ammunition > 0 && weapon->fire_delay <= 0.0f;
        
        if (action->try_fire && can_fire && health->value > 0)
        {
            // only stop pathing if grounded
            if (navigation_can_set_path(entity))
            {
                mover->is_pathing = false;
            }
            
            V2i spawn_tile = unit_transform->prev_tile;
            f32 t = mover->move_time / mover->move_rate;
            if (t < 0.5f)
            {
                spawn_tile = unit_transform->tile;
            }
            
            V2i direction = v2i_sub(action->fire_tile, spawn_tile);
            unit_transform->direction = v2i_sign(direction);
            
            // don't fire at yourself
            if (v2i_len_sq(direction) == 0)
            {
                continue;
            }
            
            for (s32 shots = 0; shots < weapon->projectiles_per_fire; ++shots)
            {
                s32 iterations = 0;
                V2i target_tile = spawn_tile;
                while (v2i_distance(target_tile, spawn_tile) == 0)
                {
                    V2i aim_offset_range = v2i(.x = -weapon->accuracy, .y = weapon->accuracy);
                    V2i offset = v2i(.x = cf_rnd_range_int(ai_rnd, aim_offset_range.x, aim_offset_range.y),
                                     .y = cf_rnd_range_int(ai_rnd, aim_offset_range.x, aim_offset_range.y));
                    target_tile = v2i_add(action->fire_tile, offset);
                    ++iterations;
                    if (iterations > 5)
                    {
                        break;
                    }
                }
                
                make_projectile(entity, spawn_tile, target_tile, elevation->value, 
                                weapon->projectile_distance, weapon->projectile_name);
            }
            weapon->ammunition = cf_max(weapon->ammunition - weapon->cost_per_fire, 0);
            
            weapon->fire_delay = weapon->fire_rate;
            
            make_event_on_fire(entity, transform->position, unit_transform->prev_tile, elevation->value);
        }
        // disable fire to avoid queued shots
        action->try_fire = false;
    }
    
    return 0;
}

ecs_ret_t system_update_unit_elevation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    World* world = s_app->world;
    
    f32 epsilon = 1e-2f;
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        
        f32 prev_velocity = elevation->velocity;
        
        elevation->velocity -= ELEVATION_GRAVITY * dt;
        
        if (prev_velocity >= 0 && elevation->velocity < 0)
        {
            elevation->initial_fall_height = elevation->value;
            if (f32_is_zero(elevation->initial_fall_height - elevation->grounded_value))
            {
                elevation->velocity -= ELEVATION_GRAVITY;
            }
        }
        
        elevation->value += elevation->velocity * dt;
        
        f32 tile_elevation = get_tile_total_elevation(unit_transform->prev_tile);
        elevation->value = cf_max(elevation->value, tile_elevation);
        
        if (cf_abs(tile_elevation - elevation->value) < epsilon)
        {
            // hit hazard tile, doing both prev_tile and tile to allow the unit to "climb" out of a hazard
            if (is_tile_hazardous(unit_transform->prev_tile) && is_tile_hazardous(unit_transform->tile))
            {
                //  @todo:  death event due to hazard tile
                make_event_on_dead(entity);
                destroy_entity(entity);
            }
            else
            {
                // always have some force pulling down, this makes 
                // it when going from tile to tile at slight elevation
                // difference short -> tall -> short -> tall
                // to look more snappy
                elevation->velocity = cf_max(elevation->velocity, 0);
                
                // hit ground
                f32 ground_impact = elevation->initial_fall_height - elevation->value;
                elevation->grounded_value = elevation->value;
                
                if (ground_impact > 1)
                {
                    make_event_ground_impact(unit_transform->prev_tile, ground_impact, false);
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_unit_move(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    f32 min_move_animation_time = 0.05f;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        
        mover->move_rate = cf_max(mover->move_rate, min_move_animation_time);
        
        f32 start_elevation = get_tile_total_elevation(unit_transform->prev_tile);
        f32 goal_elevation = get_tile_total_elevation(unit_transform->tile);
        
        start_elevation = cf_max(elevation->value, start_elevation);
        goal_elevation = cf_max(elevation->value, goal_elevation);
        
        CF_V2 start = v2i_to_v2_iso_center(unit_transform->prev_tile, start_elevation);
        CF_V2 goal = v2i_to_v2_iso_center(unit_transform->tile, goal_elevation);
        
        transform->position = cf_lerp_v2(start, goal, cf_clamp01(1.0f - mover->move_time / mover->move_rate));
    }
    
    return 0;
}

ecs_ret_t system_update_mover_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    f32 min_move_animation_time = 0.05f;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        
        b32 is_falling = (elevation->prev_value - elevation->value) > ELEVATION_FALL_THRESHOLD;
        mover->move_time = cf_clamp(mover->move_time - dt, 0.0f, mover->move_rate);
        
        if (mover->move_time == 0.0f)
        {
            unit_transform->prev_tile = unit_transform->tile;
            if (mover->is_pathing)
            {
                if (cf_array_count(navigation->path) && navigation->path_index < cf_array_count(navigation->path))
                {
                    V2i next_tile = navigation->path[navigation->path_index++];
                    unit_transform->tile = next_tile;
                    unit_transform->direction = v2i_sub(unit_transform->tile, unit_transform->prev_tile);
                    mover->move_rate = mover->next_move_rate;
                    mover->move_time = mover->move_rate;
                }
                else
                {
                    mover->is_pathing = false;
                }
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_unit_grid_slot(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Entity_Grid* grid = s_app->world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    UNUSED(component_navigation_id);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        
        entity_grid_insert(grid, unit_transform->prev_tile, entity);
    }
    
    return 0;
}

ecs_ret_t system_update_unit_colliders(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    qt_t* qt = s_app->world->qt;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_collider_id = ECS_GET_COMPONENT_ID(C_Collider);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Collider* collider = ecs_get(ecs, entity, component_collider_id);
        
        f32 elevation = get_tile_total_elevation(unit_transform->prev_tile);
        
        CF_Aabb aabb = cf_make_aabb_center_half_extents(transform->position, collider->half_extents);
        CF_V2 size = cf_extents(aabb);
        
        f32 q_x = aabb.min.x;
        f32 q_y = aabb.min.y;
        f32 q_w = size.x;
        f32 q_h = size.y;
        
        qt_rect_t bounds = qt_make_rect(q_x, q_y, q_w, q_h);
        
        qt_remove(qt, entity);
        qt_insert(qt, bounds, entity);
    }
    
    return 0;
}

void system_update_unit_colliders_on_remove(ecs_t* ecs, ecs_id_t entity_id, void* udata)
{
    UNUSED(ecs);
    UNUSED(udata);
    qt_t* qt = s_app->world->qt;
    qt_remove(qt, entity_id);
}

ecs_ret_t system_update_projectiles(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_projectile_id = ECS_GET_COMPONENT_ID(C_Projectile);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Projectile* projectile = ecs_get(ecs, entity, component_projectile_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Life_Time* life_time = ecs_get(ecs, entity, component_life_time_id);
        
        f32 t = 1.0f - life_time->duration / life_time->total;
        s32 line_index = (s32)CF_FLOORF(t * cf_array_count(projectile->line));
        
        V2i tile = projectile->line[line_index];
        Tile* tile_ptr = get_tile(tile);
        
        if (tile_ptr)
        {
            // try to climb the tile if possibe by using highest elevation
            f32 tile_elevation = get_tile_total_elevation(tile);
            f32 tile_elevation_offset = get_tile_elevation_offset(tile);
            f32 tile_total_elevation = tile_elevation + tile_elevation_offset;
            b32 is_short = tile_ptr->elevation & 1;
            
            // if stackless is close enough to climb then climb otherwise go through it
            if (tile_ptr->stackless)
            {
                f32 elevation_delta = tile_total_elevation - elevation->value;
                if (tile_total_elevation > elevation->value && elevation_delta <= CLIMBABLE_ELEVATION)
                {
                    if (is_short && tile_elevation_offset > CLIMBABLE_ELEVATION)
                    {
                        tile_elevation = elevation_s32_to_f32(tile_ptr->elevation) - 1;
                    }
                    else if (!is_short && tile_elevation_offset > CLIMBABLE_ELEVATION * 1.25f)
                    {
                        tile_elevation = elevation_s32_to_f32(tile_ptr->elevation) - 2;
                    }
                    elevation->value = cf_max(elevation->value, tile_elevation);
                }
            }
            else
            {
                // if the tile is flying then see if projectile can squeeze through
                // short tile needs CLIMBABLE_ELEVATION
                // tall tiles needs a bit more space than CLIMBABLE_ELEVATION
                if (is_short && tile_elevation_offset > CLIMBABLE_ELEVATION)
                {
                    tile_elevation = elevation_s32_to_f32(tile_ptr->elevation) - 1;
                }
                else if (!is_short && tile_elevation_offset > CLIMBABLE_ELEVATION * 1.25f)
                {
                    tile_elevation = elevation_s32_to_f32(tile_ptr->elevation) - 2;
                }
                elevation->value = cf_max(elevation->value, tile_elevation);
            }
        }
        
        V2i start_tile = projectile->line[0];
        V2i end_tile = cf_array_last(projectile->line);
        CF_V2 p0 = v2i_to_v2_iso_center(start_tile, projectile->start_elevation);
        CF_V2 p1 = v2i_to_v2_iso_center(end_tile, elevation->value);
        transform->position = cf_lerp_v2(p0, p1, t);
    }
    
    return 0;
}

ecs_ret_t system_update_projectile_hits(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    World* world = s_app->world;
    
    qt_t* qt = s_app->world->qt;
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_projectile_id = ECS_GET_COMPONENT_ID(C_Projectile);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_spawner_id = ECS_GET_COMPONENT_ID(C_Spawner);
    
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    ecs_id_t component_team_id = ECS_GET_COMPONENT_ID(C_Team);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Projectile* projectile = ecs_get(ecs, entity, component_projectile_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Spawner* spawner = ecs_get(ecs, entity, component_spawner_id);
        
        s32 hit_count = 0;
        s32 controllable_units_killed = 0;
        
        //  @todo:  probably need to add in an additional test case for slope tiles
        //          since projectiles SHOULD be able to climb a slope giving it's going
        //          either up or down
        if (elevation->value - elevation->prev_value < CLIMBABLE_ELEVATION)
        {
            // check for hits against units
            {
                CF_Aabb aabb = cf_make_aabb_center_half_extents(transform->position, cf_v2(2, 2));
                
                CF_V2 size = cf_extents(aabb);
                f32 q_x = aabb.min.x;
                f32 q_y = aabb.min.y;
                f32 q_w = size.x;
                f32 q_h = size.y;
                
                qt_rect_t bounds = qt_make_rect(q_x, q_y, q_w, q_h);
                s32 query_count = 0;
                qt_value_t* hits = qt_query(qt, bounds, &query_count);
                
                for (s32 hit_index = 0; hit_index < query_count; ++hit_index)
                {
                    ecs_id_t hit_entity = hits[hit_index];
                    if (hit_entity == projectile->owner)
                    {
                        continue;
                    }
                    
                    if (ecs_is_ready(ecs, hit_entity))
                    {
                        if (ecs_has(ecs, hit_entity, component_team_id))
                        {
                            C_Team* hit_team = ecs_get(ecs, hit_entity, component_team_id);
                            
                            if (hit_team->id == projectile->team_id && !hit_team->friendly_fire)
                            {
                                continue;
                            }
                        }
                        
                        if (ecs_has(ecs, hit_entity, component_health_id))
                        {
                            C_Health* hit_health = ecs_get(ecs, hit_entity, component_health_id);
                            if (hit_health->value > 0)
                            {
                                //  @todo:  raise event for damage
                                if (!hit_health->is_invulnerable)
                                {
                                    --hit_health->value;
                                }
                                ++hit_count;
                                make_event_on_hit(projectile->owner, hit_entity);
                                if (hit_health->value <= 0)
                                {
                                    ++controllable_units_killed;
                                    make_event_on_kill(projectile->owner, hit_entity);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // failed to climb tile due to elevation difference, hit the tile instead
            hit_count = 1;
        }
        
        if (hit_count)
        {
            destroy_entity(entity);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_pickup_hits(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    Entity_Grid* grid = s_app->world->grid;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_pickup_id = ECS_GET_COMPONENT_ID(C_Pickup);
    ecs_id_t component_asset_resource_id = ECS_GET_COMPONENT_ID(C_Asset_Resource);
    
    ecs_id_t component_weapon_id = ECS_GET_COMPONENT_ID(C_Weapon);
    
    pq ecs_id_t* closest_targets = NULL;
    MAKE_SCRATCH_PQ(closest_targets, 32);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Pickup* pickup = ecs_get(ecs, entity, component_pickup_id);
        C_Asset_Resource* asset_resource = ecs_get(ecs, entity, component_asset_resource_id);
        
        s32 touch_count = 0;
        
        pq_clear(closest_targets);
        dyna ecs_id_t* targets = entity_grid_query(grid, unit_transform->prev_tile);
        get_closest_target_elevation_touches(ecs, targets, elevation->value, closest_targets);
        targets = entity_grid_query(grid, unit_transform->tile);
        get_closest_target_elevation_touches(ecs, targets, elevation->value, closest_targets);
        
        // allow multipe entities to pickup the same pickup at the same time for now
        s32 pickup_stop_index = 0;
        f32 current_elevation = pq_count(closest_targets) ? pq_weight_at(closest_targets, 0) : 0.0f;
        for (s32 target_index = 1; target_index < pq_count(closest_targets); ++target_index)
        {
            f32 delta = current_elevation - pq_weight_at(closest_targets, target_index);
            if (f32_is_zero(delta))
            {
                ++pickup_stop_index;
            }
        }
        
        for (s32 pickup_index = 0; pickup_index < pickup_stop_index; ++pickup_index)
        {
            ecs_id_t target = closest_targets[pickup_index];
            
            if (pickup->type == PickupType_Ammunition)
            {
                if (ecs_has(ecs, target, component_weapon_id))
                {
                    C_Weapon* weapon = ecs_get(ecs, target, component_weapon_id);
                    weapon->ammunition = cf_min(weapon->ammunition + pickup->count, weapon->max_ammunition);
                    ++touch_count;
                    make_event_on_pickup(target, asset_resource->name);
                    make_event_on_touch(target, entity);
                }
            }
            
        }
        
        if (touch_count)
        {
            destroy_entity(entity);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_life_times(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    World* world = s_app->world;
    
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Life_Time* life_time = ecs_get(ecs, entity, component_life_time_id);
        
        life_time->duration -= dt;
        
        if (life_time->duration <= 0.0f)
        {
            destroy_entity(entity);
        }
    }
    
    return 0;
}

ecs_ret_t system_update_healths(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    CF_Rnd* rnd = &s_app->world->ai_rnd;
    
    f32 corpse_life_time = 1.0f;
    
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    ecs_id_t component_corpse_id = ECS_GET_COMPONENT_ID(C_Corpse);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Health* health = ecs_get(ecs, entity, component_health_id);
        
        if (health->prev_value > 0 && health->value <= 0)
        {
            if (!ecs_has(ecs, entity, component_corpse_id))
            {
                ecs_add(ecs, entity, component_life_time_id, NULL);
                ecs_add(ecs, entity, component_corpse_id, NULL);
                
                C_Life_Time* life_time = ecs_get(ecs, entity, component_life_time_id);
                set_life_time_duration(life_time, corpse_life_time);
                
                make_event_on_dead(entity);
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_update_child_transforms(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_child_transform_id = ECS_GET_COMPONENT_ID(C_Child_Transform);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Child_Transform* child_transform = ecs_get(ecs, entity, component_child_transform_id);
        
        if (ecs_is_ready(ecs, child_transform->parent))
        {
            if (ecs_has(ecs, child_transform->parent, component_transform_id))
            {
                C_Transform* parent_transform = ecs_get(ecs, child_transform->parent, component_transform_id);
                transform->position = cf_add_v2(parent_transform->position, child_transform->offset);
            }
        }
    }
    return 0;
}

ecs_ret_t system_update_unit_sprites(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_asset_resource_id = ECS_GET_COMPONENT_ID(C_Asset_Resource);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_health_id = ECS_GET_COMPONENT_ID(C_Health);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Asset_Resource* asset_resource = ecs_get(ecs, entity, component_asset_resource_id);
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Sprite* c_sprite = ecs_get(ecs, entity, component_sprite_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Health* health = ecs_get(ecs, entity, component_health_id);
        CF_Sprite* sprite = &c_sprite->sprite;
        
        const char* animation_prefix = "idle";
        if (health->value <= 0)
        {
            animation_prefix = "dead";
        }
        else if (health->prev_value > health->value)
        {
            animation_prefix = "hit";
        }
        else if ((elevation->initial_fall_height - elevation->value) > ELEVATION_FALL_THRESHOLD)
        {
            animation_prefix = "fall";
        }
        else if (mover->is_pathing)
        {
            animation_prefix = "walk";
        }
        
        if (ecs_has(ecs, entity, component_control_id))
        {
            C_Control* control = ecs_get(ecs, entity, component_control_id);
            if (control->is_locked)
            {
                animation_prefix = "jump";
            }
        }
        
        AnimationDirection animation_direction = unit_sprite_animation_direction(animation_prefix, unit_transform->direction);
        if (!cf_sprite_is_playing(sprite, animation_direction.name))
        {
            C_Sprite* lookup_sprite = assets_get_resource_property_value(asset_resource->name, cf_sintern(CF_STRINGIZE(C_Sprite)));
            if (cf_hashtable_has(lookup_sprite->animations, animation_direction.name))
            {
                const char* animation_name = cf_hashtable_get(lookup_sprite->animations, animation_direction.name);
                cf_sprite_play(sprite, animation_name);
            }
        }
        if (animation_direction.is_flipped && sprite->scale.x > 0)
        {
            cf_sprite_flip_x(sprite);
        }
        else if (!animation_direction.is_flipped && sprite->scale.x < 0)
        {
            cf_sprite_flip_x(sprite);
        }
        
        cf_sprite_update(sprite);
    }
    
    return 0;
}

ecs_ret_t system_update_sprites(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Sprite* sprite = ecs_get(ecs, entity, component_sprite_id);
        
        cf_sprite_update(&sprite->sprite);
    }
    
    return 0;
}

ecs_ret_t system_update_demo_spectate(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    
    if (world->state != World_State_Demo)
    {
        return 0;
    }
    
    static s32 spectate_index = 0;
    f32 rotate_interval = 5.0f;
    
    ecs_id_t component_camera_focus_id = ECS_GET_COMPONENT_ID(C_Camera_Focus);
    ecs_id_t component_ai_id = ECS_GET_COMPONENT_ID(C_AI);
    
    if (cf_on_interval(rotate_interval, 0))
    {
        spectate_index = (spectate_index + 1) % entity_count;
    }
    
    spectate_index = cf_clamp_int(spectate_index, 0, entity_count - 1);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Camera_Focus* camera_focus = ecs_get(ecs, entity, component_camera_focus_id);
        
        if (index != spectate_index)
        {
            *camera_focus = 0;
        }
        else
        {
            *camera_focus = 1;
        }
    }
    
    return 0;
}

//  @note:  we could instead render the game onto it's own canvas 
//          and drawn at a smaller or clipped scale comapared to UI
//          as long as there's no offsets done or resizing then that
//          should be fine, only issue is the mouse picking is dependent
//          on the projection.
//          doing the below is just a quick and dirty way to get this drawn
//          without messing with any of that.
ecs_ret_t system_update_camera(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_camera_focus_id = ECS_GET_COMPONENT_ID(C_Camera_Focus);
    
    
    fixed CF_V2* focus_positions = NULL;
    MAKE_SCRATCH_ARRAY(focus_positions, entity_count);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Camera_Focus* camera_focus = ecs_get(ecs, entity, component_camera_focus_id);
        
        if (*camera_focus != 0)
        {
            cf_array_push(focus_positions, transform->position);
        }
    }
    
    focus_camera(focus_positions, cf_array_count(focus_positions), dt);
    
    return 0;
}

// ---------------
// system draws
// ---------------

ecs_ret_t system_draw_background(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(ecs);
    UNUSED(entities);
    UNUSED(entity_count);
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    CF_Sprite* background = world->level.background;
    if (background)
    {
        cf_sprite_update(background);
        
        CF_Aabb level_aabb = get_level_aabb();
        CF_V2 extents = cf_extents(level_aabb);
        
        //  @todo:  figure out what to do with the background, if it should be parallax or if the editor
        //          needs more options on how to setup the background
        //          the block below can make the background sort of fit to the level but it also rescales
        //          the entire background to match the level. this looks okay when tiles are not moving
        //          but once they start to the background will scale up/down depending on the top most
        //          tile elevation
        CF_V2 background_size = cf_v2(background->w * background->scale.x, background->h * background->scale.y);
        if (background_size.x < extents.x || background_size.y < extents.y)
        {
            extents = cf_mul_v2_f(extents, 1.25f);
            CF_V2 scale = cf_v2(extents.x / (f32)background->w, extents.y / (f32)background->h);
            // maintain aspect ratio
            f32 largest_scale = cf_max(scale.x, scale.y);
            scale = cf_v2(largest_scale, largest_scale);
            scale = cf_max_v2(scale, cf_v2(1, 1));
            
            //background->transform.p = cf_center(level_aabb);
            background->scale = scale;
        }
        cf_draw_sprite(background);
    }
    
    return 0;
}

ecs_ret_t system_draw_setup_camera(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(ecs);
    UNUSED(entities);
    UNUSED(entity_count);
    UNUSED(dt);
    UNUSED(udata);
    
    World* world = s_app->world;
    
    CF_M3x2 projection = cf_ortho_2d(0, 0, (f32)cf_app_get_width(), (f32)cf_app_get_height());
    
    cf_draw_push();
    cf_draw_projection(projection);
    cf_draw_translate_v2(cf_neg_v2(s_app->world->camera.position));
    
    return 0;
}

ecs_ret_t system_draw_level_tile(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(ecs);
    UNUSED(entities);
    UNUSED(entity_count);
    UNUSED(dt);
    UNUSED(udata);
    
    Input* input = s_app->input;
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    V2i min = v2i();
    V2i max = world->level.size;
    
    V2i tile_select = input->tile_select;
    
    b32 is_any_edit = editor->state == Editor_State_Edit || editor->state == Editor_State_Pause;
    b32 is_any_mode_playing = (world->state == World_State_Play || 
                               editor->state == Editor_State_Edit || editor->state == Editor_State_Edit_Play);
    b32 is_demo = world->state == World_State_Demo;
    
    // draw tiles
    for (s32 y = min.y; y < max.y; ++y)
    {
        for (s32 x = min.x; x < max.x; ++x)
        {
            V2i tile = v2i(.x = x, .y = y);
            draw_tile(tile);
        }
    }
    
    //  @note:  minor
    //  @todo:  build all these up in an RLE format before system_draw_level_tile()
    //          such that each RLE means it's on the same elevation as nearby tiles
    //          so it can batch draw these outlines as a larger set of lines
    if (!is_demo && is_any_edit)
    {
        // draw grid
        draw_push_color(cf_color_grey());
        for (s32 y = min.y; y < max.y; ++y)
        {
            for (s32 x = min.x; x < max.x; ++x)
            {
                V2i tile = v2i(.x = x, .y = y);
                draw_tile_outline(tile);
            }
        }
        draw_pop_color();
    }
    
    // draw tile select
    if (!is_demo && is_any_mode_playing && is_tile_in_bounds(tile_select))
    {
        static b32 swap_color = false;
        CF_Color color = cf_color_cyan();
        if (cf_on_interval(1.0f, 0.0f))
        {
            swap_color = !swap_color;
        }
        if (swap_color)
        {
            color.g = cf_clamp01(color.g + 0.2f);
            color.b = cf_clamp01(color.b + 0.1f);
        }
        
        draw_push_color(color);
        if (v2i_distance(input->tile_select_start, input->tile_select_end) == 0)
        {
            draw_tile_fill(tile_select);
        }
        else
        {
            V2i select_min = v2i_min(input->tile_select_start, input->tile_select_end);
            V2i select_max = v2i_max(input->tile_select_start, input->tile_select_end);
            
            for (s32 y = select_min.y; y <= select_max.y; ++y)
            {
                for (s32 x = select_min.x; x <= select_max.x; ++x)
                {
                    V2i tile = v2i(.x = x, .y = y);
                    draw_tile_fill(tile);
                }
            }
            
        }
        draw_pop_color();
    }
    
    return 0;
}

ecs_ret_t system_draw_ai_view(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    CF_Color view_tile_color = cf_color_yellow();
    view_tile_color.a = 0.5f;
    
    ecs_id_t component_ai_view_id = ECS_GET_COMPONENT_ID(C_AI_View);
    
    draw_push_color(view_tile_color);
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_AI_View* ai_view = ecs_get(ecs, entity, component_ai_view_id);
        
        // since ai_view uses frame arrays to draw and do checks
        // any skips in update like pauses can cause this to be bad
        // so skip it if it does, ideally you shouldn't do this as it's
        // not gaurenteed to have this array survive from update -> draw
        // as well as interacting directly with internal DS header.
        // any scratch array that's passed from system to system should
        // be gaurenteed to reach that system.
        if (cf_array_count(ai_view->tiles))
        {
            if (CF_AHDR(ai_view->tiles)->cookie != CF_ACOOKIE || cf_array_count(ai_view->tiles) > 200)
            {
                continue;
            }
        }
        
        for (s32 tile_index = 0; tile_index < cf_array_count(ai_view->tiles); ++tile_index)
        {
            draw_tile_fill(ai_view->tiles[tile_index]);
        }
    }
    draw_pop_color();
    
    return 0;
}

ecs_ret_t system_draw_control_preview_path(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    
    draw_push_color(cf_color_yellow());
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        
        for (s32 path_index = 0; path_index < cf_array_count(control->preview_path); ++path_index)
        {
            V2i tile = control->preview_path[path_index];
            draw_tile_outline(tile);
        }
    }
    draw_pop_color();
    
    return 0;
}

ecs_ret_t system_draw_control_path(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_navigation_id = ECS_GET_COMPONENT_ID(C_Navigation);
    UNUSED(component_control_id);
    
    draw_push_color(cf_color_orange());
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Navigation* navigation = ecs_get(ecs, entity, component_navigation_id);
        
        for (s32 path_index = navigation->path_index; path_index < cf_array_count(navigation->path); ++path_index)
        {
            V2i tile = navigation->path[path_index];
            draw_tile_outline(tile);
        }
    }
    draw_pop_color();
    
    return 0;
}

ecs_ret_t system_draw_control_aim(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    Input* input = s_app->input;
    CF_V2 world_mouse = input->world_mouse;
    
    ecs_id_t component_control_id = ECS_GET_COMPONENT_ID(C_Control);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    b32 draw_line = false;
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Control* control = ecs_get(ecs, entity, component_control_id);
        
        if (control->order == 0)
        {
            if (cf_len_sq(input->aim_direction) > 0)
            {
                world_mouse = cf_add_v2(transform->position, input->aim_direction);
                draw_line = true;
            }
            break;
        }
    }
    
    // only draw aim line if currently trying to aim
    if (draw_line)
    {
        draw_push_layer(1);
        draw_push_color(cf_color_white());
        for (s32 index = 0; index < entity_count; ++index)
        {
            ecs_id_t entity = entities[index];
            C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
            C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
            C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
            C_Control* control = ecs_get(ecs, entity, component_control_id);
            
            if (control->order == CONTROL_INVALID_CONTROL)
            {
                continue;
            }
            
            draw_push_line(Draw_Sort_Key_Type_Unit, unit_transform->tile, elevation->value, transform->position, world_mouse);
        }
        draw_pop_color();
        draw_pop_layer();
    }
    
    return 0;
}

ecs_ret_t system_draw_unit_tile(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    
    draw_push_color(cf_color_cyan());
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        
        draw_tile_outline(unit_transform->tile);
    }
    draw_pop_color();
    
    return 0;
}

//  @todo:  decals needs to be fixed, it can clipped by tiles that are below where the decal gets drawn
//          maybe measure how large the sprite is and see how much it intersects nearby tiles to determine
//          the draw layer. this is mostly noticeable for flat elevation areas, tiles with higher elevation
//          should still clip on any intersections
ecs_ret_t system_draw_decals(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    ecs_id_t component_decal_id = ECS_GET_COMPONENT_ID(C_Decal);
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Sprite* c_sprite = ecs_get(ecs, entity, component_sprite_id);
        C_Decal* decal = ecs_get(ecs, entity, component_decal_id);
        C_Life_Time* life_time = ecs_get(ecs, entity, component_life_time_id);
        CF_Sprite* sprite = &c_sprite->sprite;
        
        f32 opacity = life_time->duration / (life_time->total - decal->fade_delay);
        opacity = cf_clamp01(opacity);
        
        V2i tile = get_tile_from_input_cursor(transform->position, false);
        if (tile.x == -1 && tile.y == -1)
        {
            tile = get_tile_from_world(transform->position);
        }
        
        sprite->transform.p = transform->position;
        sprite->opacity = opacity;
        draw_push_sprite(Draw_Sort_Key_Type_Decal, tile, get_tile_total_elevation(tile), sprite);
    }
    
    return 0;
}

ecs_ret_t system_draw_unit_shadow_blobs(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    CF_V2 tile_size = assets_get_tile_size();
    f32 radius = cf_min(tile_size.x, tile_size.y);
    // same level as ground to max height
    CF_V2 shadow_blob_scaling = cf_v2(0.22f, 0.05f);
    // 4 tiles high
    f32 max_shadow_elevation = elevation_s32_to_f32(8);
    CF_Color shadow_color = cf_color_black();
    shadow_color.a = 0.25f;
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_mover_id = ECS_GET_COMPONENT_ID(C_Mover);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    V2i occluding_tile_offsets[] =
    {
        v2i(.y = -1),
        v2i(.x = -1),
        v2i(.x = -1, .y = -1),
    };
    
    draw_push_color(shadow_color);
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Mover* mover = ecs_get(ecs, entity, component_mover_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        
        // draw blob
        {
            f32 t = mover->move_time / mover->move_rate;
            f32 prev_tile_elevation = get_tile_total_elevation(unit_transform->prev_tile);
            f32 tile_elevation = get_tile_total_elevation(unit_transform->tile);
            f32 ground_elevation = cf_lerp(prev_tile_elevation, tile_elevation, t);
            
            CF_V2 p0 = v2i_to_v2_iso_center(unit_transform->prev_tile, prev_tile_elevation);
            CF_V2 p1 = v2i_to_v2_iso_center(unit_transform->tile, tile_elevation);
            
            CF_V2 p = cf_lerp_v2(p1, p0, t);
            f32 shadow_scaling = cf_clamp01((elevation->value - ground_elevation) / max_shadow_elevation);
            f32 shadow_radius = cf_lerp(shadow_blob_scaling.x, shadow_blob_scaling.y, shadow_scaling);
            shadow_radius *= radius;
            
            V2i draw_tile = v2i_min(unit_transform->prev_tile, unit_transform->tile);
            draw_push_circle_fill(Draw_Sort_Key_Type_Shadow, draw_tile, ground_elevation, p, shadow_radius);
        }
    }
    draw_pop_color();
    
    return 0;
}

ecs_ret_t system_draw_props(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    ecs_id_t component_prop_id = ECS_GET_COMPONENT_ID(C_Prop);
    UNUSED(component_prop_id);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Sprite* c_sprite = ecs_get(ecs, entity, component_sprite_id);
        CF_Sprite* sprite = &c_sprite->sprite;
        
        V2i draw_tile = v2i_min(unit_transform->prev_tile, unit_transform->tile);
        sprite->transform.p = transform->position;
        f32 ground_elevation = cf_max(get_tile_total_elevation(unit_transform->prev_tile), get_tile_total_elevation(unit_transform->tile));
        ground_elevation = cf_max(ground_elevation, elevation->value);
        
        draw_push_sprite(Draw_Sort_Key_Type_Prop, draw_tile, ground_elevation, sprite);
    }
    
    return 0;
}

ecs_ret_t system_draw_units(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Unit_Transform* unit_transform = ecs_get(ecs, entity, component_unit_transform_id);
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Sprite* c_sprite = ecs_get(ecs, entity, component_sprite_id);
        CF_Sprite* sprite = &c_sprite->sprite;
        
        V2i draw_tile = v2i_min(unit_transform->prev_tile, unit_transform->tile);
        sprite->transform.p = transform->position;
        f32 ground_elevation = cf_max(get_tile_total_elevation(unit_transform->prev_tile), get_tile_total_elevation(unit_transform->tile));
        ground_elevation = cf_max(ground_elevation, elevation->value);
        
        draw_push_sprite(Draw_Sort_Key_Type_Unit, draw_tile, ground_elevation, sprite);
    }
    
    return 0;
}

ecs_ret_t system_draw_emotes(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_child_transform_id = ECS_GET_COMPONENT_ID(C_Child_Transform);
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    
    ecs_id_t component_unit_transform_id = ECS_GET_COMPONENT_ID(C_Unit_Transform);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Child_Transform* child_transform = ecs_get(ecs, entity, component_child_transform_id);
        C_Sprite* c_sprite = ecs_get(ecs, entity, component_sprite_id);
        CF_Sprite* sprite = &c_sprite->sprite;
        
        if (ecs_is_ready(ecs, child_transform->parent))
        {
            if (ecs_has(ecs, child_transform->parent, component_unit_transform_id) &&
                ecs_has(ecs, child_transform->parent, component_elevation_id))
            {
                C_Unit_Transform* parent_unit_transform = ecs_get(ecs, child_transform->parent, component_unit_transform_id);
                C_Elevation* parent_elevation = ecs_get(ecs, child_transform->parent, component_elevation_id);
                
                sprite->transform.p = transform->position;
                V2i draw_tile = v2i_min(parent_unit_transform->prev_tile, parent_unit_transform->tile);
                f32 ground_elevation = cf_max(get_tile_total_elevation(parent_unit_transform->prev_tile), get_tile_total_elevation(parent_unit_transform->tile));
                ground_elevation = cf_max(ground_elevation, parent_elevation->value);
                
                draw_push_sprite(Draw_Sort_Key_Type_Emote, draw_tile, ground_elevation, sprite);
            }
        }
    }
    
    return 0;
}

ecs_ret_t system_draw_projectiles(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(dt);
    UNUSED(udata);
    
    ecs_id_t component_transform_id = ECS_GET_COMPONENT_ID(C_Transform);
    ecs_id_t component_projectile_id = ECS_GET_COMPONENT_ID(C_Projectile);
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    ecs_id_t component_life_time_id = ECS_GET_COMPONENT_ID(C_Life_Time);
    
    draw_push_color(cf_color_purple());
    for (s32 index = 0; index < entity_count; ++index)
    {
        ecs_id_t entity = entities[index];
        C_Transform* transform = ecs_get(ecs, entity, component_transform_id);
        C_Projectile* projectile = ecs_get(ecs, entity, component_projectile_id);
        C_Elevation* elevation = ecs_get(ecs, entity, component_elevation_id);
        C_Life_Time* life_time = ecs_get(ecs, entity, component_life_time_id);
        
        f32 t = 1.0f - life_time->duration / life_time->total;
        f32 t_0 = t * cf_array_count(projectile->line);
        f32 t_1 = t_0 + 0.5f;
        s32 i_0 = (s32)CF_FLOORF(t_0);
        s32 i_1 = (s32)CF_FLOORF(t_1);
        V2i prev_tile = projectile->line[i_0];
        V2i tile = projectile->line[i_1];
        
        // use highest elvation otherwise there's clipping as projectile moves up hill
        f32 max_elevation = cf_max(get_tile_elevation_f32(prev_tile), get_tile_elevation_f32(tile));
        max_elevation = cf_max(max_elevation, elevation->value);
        
        V2i draw_tile = v2i_min(prev_tile, tile);
        draw_push_circle_fill(Draw_Sort_Key_Type_Unit, draw_tile, max_elevation, transform->position, 3.0f);
    }
    draw_pop_color();
    
    return 0;
}

ecs_ret_t system_draw_editor(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(ecs);
    UNUSED(entities);
    UNUSED(entity_count);
    UNUSED(dt);
    UNUSED(udata);
    
    editor_draw();
    return 0;
}

ecs_ret_t system_draw_canvas_composite(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata)
{
    UNUSED(ecs);
    UNUSED(entities);
    UNUSED(entity_count);
    UNUSED(dt);
    UNUSED(udata);
    
    draw_sort();
    draw_push_all();
    cf_draw_pop();
    
    cf_render_to(cf_app_get_canvas(), true);
    
    return 0;
}

// ---------------
// system utils
// ---------------

s32 get_closest_target_elevation_touches(ecs_t* ecs, dyna ecs_id_t* targets, f32 elevation, pq ecs_id_t* out_targets)
{
    s32 count = 0;
    ecs_id_t component_elevation_id = ECS_GET_COMPONENT_ID(C_Elevation);
    
    for (s32 index = 0; index < cf_array_count(targets); ++index)
    {
        ecs_id_t target = targets[index];
        if (ecs_is_ready(ecs, target) && ecs_has(ecs, target, component_elevation_id))
        {
            C_Elevation* target_elevation = ecs_get(ecs, target, component_elevation_id);
            f32 delta = cf_abs(target_elevation->value - elevation);
            if (delta <= ELEVATION_TOUCH_DISTANCE)
            {
                pq_add(out_targets, target, delta);
                ++count;
            }
        }
    }
    return count;
}

void set_elevation_value(C_Elevation* elevation, f32 value)
{
    elevation->value = value;
    elevation->prev_value = value;
    elevation->grounded_value = value;
}

void set_unit_transform_tile(C_Unit_Transform* unit_transform, V2i value)
{
    unit_transform->tile = value;
    unit_transform->prev_tile = value;
}

void set_life_time_duration(C_Life_Time* life_time, f32 duration)
{
    life_time->duration = duration;
    life_time->total = duration;
}

// worse case is when there's no ties and this thing walks for every across the map
// 100*100 entities can take up to 500ms to load up due to this (mostly astar)
// so if this is not called then we end up wtih 170ms
void setup_ai_patrol(C_AI_Patrol* ai_patrol, V2i tile, s32 patrol_distance)
{
    static s32 start_index = 0;
    
    V2i patrol_directions[] =
    {
        v2i(.x = 1),
        v2i(.y = 1),
        v2i(.x = -1),
        v2i(.y = -1),
    };
    cf_array_clear(ai_patrol->tiles);
    cf_array_push(ai_patrol->tiles, tile);
    s32 expected_patrol_tile_count = CF_ARRAY_SIZE(patrol_directions) + 1;
    s32 attempts = 0;
    s32 index = start_index;
    
    while (attempts < CF_ARRAY_SIZE(patrol_directions))
    {
        index = index % CF_ARRAY_SIZE(patrol_directions);
        s32 distance = patrol_distance;
        while (distance > 0)
        {
            V2i direction = patrol_directions[index];
            V2i patrol_tile = v2i_mul_i(direction, distance);
            patrol_tile = v2i_add(tile, patrol_tile);
            
            while (is_tile_in_bounds(patrol_tile) && 
                   is_tile_hazardous(patrol_tile))
            {
                // keep moving tile further if it's a harzard while still in bounds
                patrol_tile = v2i_add(patrol_tile, direction);
            }
            if (is_tile_in_bounds(patrol_tile))
            {
                // at this point tile is in bounds and not a harzard, try to see if there's a path
                // before adding it to patrol list
                fixed V2i* path = astar(tile, patrol_tile, MAX_ASTAR_DISTANCE);
                if (cf_array_count(path))
                {
                    cf_array_push(ai_patrol->tiles, patrol_tile);
                    break;
                }
            }
            --distance;
        }
        ++attempts;
        ++index;
    }
}

b32 elevation_is_grounded(C_Elevation* elevation)
{
    return f32_is_zero(elevation->grounded_value - elevation->value) && elevation->velocity <= 0.0f;
}

b32 navigation_set_path(ecs_id_t entity, dyna V2i* path)
{
    C_Navigation* navigation = ECS_GET_COMPONENT(entity, C_Navigation);
    C_Action* action = ECS_GET_COMPONENT(entity, C_Action);
    C_Elevation* elevation = ECS_GET_COMPONENT(entity, C_Elevation);
    C_Slip* slip = ECS_GET_COMPONENT(entity, C_Slip);
    C_Flying* flying = ECS_GET_COMPONENT(entity, C_Flying);
    
    b32 path_set = false;
    
    if (navigation && action)
    {
        b32 can_set_path = true;
        if (elevation)
        {
            if (!f32_is_zero(elevation->grounded_value - elevation->value))
            {
                can_set_path = false;
            }
        }
        if (slip)
        {
            if (slip->duration > 0)
            {
                can_set_path = false;
            }
        }
        if (flying)
        {
            can_set_path = false;
        }
        
        if (can_set_path)
        {
            cf_array_set(navigation->path, path);
            action->apply_new_path = true;
            path_set = true;
        }
    }
    
    return path_set;
}

b32 navigation_can_set_path(ecs_id_t entity)
{
    C_Elevation* elevation = ECS_GET_COMPONENT(entity, C_Elevation);
    C_Slip* slip = ECS_GET_COMPONENT(entity, C_Slip);
    
    b32 can_set_path = true;
    if (elevation)
    {
        if (!f32_is_zero(elevation->grounded_value - elevation->value))
        {
            can_set_path = false;
        }
    }
    if (slip)
    {
        if (slip->duration > 0)
        {
            can_set_path = false;
        }
    }
    
    return can_set_path;
}

V2i navigation_get_direction_from_tile(ecs_id_t entity, V2i tile)
{
    C_Unit_Transform* unit_transform = ECS_GET_COMPONENT(entity, C_Unit_Transform);
    C_Navigation* navigation = ECS_GET_COMPONENT(entity, C_Navigation);
    V2i direction = v2i_sub(unit_transform->tile, unit_transform->prev_tile);
    
    // unit has stopped on the prop
    if (v2i_len_sq(direction) == 0)
    {
        s32 prev_index = 0;
        for (s32 index = 0; index < cf_array_count(navigation->path); ++index)
        {
            if (v2i_distance(tile, navigation->path[index]) == 0)
            {
                direction = v2i_sub(tile, navigation->path[prev_index]);
                break;
            }
            prev_index = index;
        }
    }
    
    direction = v2i_norm(direction);
    
    return direction;
}

// ---------------
// entity spawning
// ---------------

ecs_id_t make_entity(V2i tile, const char* name)
{
    Asset_Resource* resource = assets_get_resource(name);
    if (!resource)
    {
        return ECS_NULL;
    }
    
    ecs_id_t entity = ecs_create(s_app->ecs);
    
    for (s32 index = 0; index < cf_array_count(resource->properties); ++index)
    {
        Property* property = resource->properties + index;
        // if a component isn't registered and you try to add a component, pico_ecs will 
        // return some garbage and can mess up the all of the component arrays
        if (ECS_IS_COMPONENT_REGISTERED(property->key))
        {
            void* component = ECS_ADD_PROPERTY_COMPONENT(entity, property->key);
            if (component && property->value)
            {
                property_copy_to(property, component);
            }
        }
    }
    
    C_Sprite* sprite = ECS_GET_COMPONENT(entity, C_Sprite);
    C_Collider* collider = ECS_GET_COMPONENT(entity, C_Collider);
    C_Transform* transform = ECS_GET_COMPONENT(entity, C_Transform);
    C_Unit_Transform* unit_transform = ECS_GET_COMPONENT(entity, C_Unit_Transform);
    C_Elevation* elevation = ECS_GET_COMPONENT(entity, C_Elevation);
    C_AI_Patrol* ai_patrol = ECS_GET_COMPONENT(entity, C_AI_Patrol);
    C_Control* control = ECS_GET_COMPONENT(entity, C_Control);
    C_Mover* mover = ECS_GET_COMPONENT(entity, C_Mover);
    C_Switch* c_switch = ECS_GET_COMPONENT(entity, C_Switch);
    C_Decal* decal = ECS_GET_COMPONENT(entity, C_Decal);
    
    if (sprite)
    {
        sprite->sprite = cf_make_sprite(sprite->name);
        sprite->sprite.scale = sprite->scale;
        
        const char* anim_default = NULL;
        if (cf_hashtable_has(sprite->animations, cf_sintern("default")))
        {
            anim_default = cf_hashtable_get(sprite->animations, cf_sintern("default"));
            cf_sprite_play(&sprite->sprite, anim_default);
        }
    }
    
    if (sprite && collider)
    {
        CF_V2 collider_half_extents = cf_v2(16, 16);
        sprite->sprite = cf_make_sprite(sprite->name);
        sprite->sprite.scale = sprite->scale;
        if (sprite->sprite.name)
        {
            CF_Aabb collider_aabb = cf_sprite_get_slice(&sprite->sprite, "collider");
            collider_half_extents = cf_half_extents(collider_aabb);
            collider_half_extents = cf_mul_v2(collider_half_extents, sprite->scale);
        }
        
        collider->half_extents = collider_half_extents;
    }
    
    if (transform)
    {
        transform->position = v2i_to_v2_iso_center(tile, 0);
    }
    
    if (elevation)
    {
        set_elevation_value(elevation, get_tile_total_elevation(tile));
    }
    
    if (unit_transform)
    {
        set_unit_transform_tile(unit_transform, tile);
    }
    
    if (ai_patrol)
    {
        // default to 5 for now
        ai_patrol->patrol_distance = cf_max(5, ai_patrol->patrol_distance);
        setup_ai_patrol(ai_patrol, tile, ai_patrol->patrol_distance);
    }
    
    if (control && mover)
    {
        control->move_rate = mover->move_rate;
    }
    
    if (c_switch)
    {
        c_switch->key = tile;
    }
    
    if (decal && sprite)
    {
        if (decal->is_random_animation)
        {
            s32 count = cf_hashtable_count(sprite->animations);
            if (count)
            {
                s32 animation_index = cf_rnd_range_int(&s_app->assets->rnd, 0, count - 1);
                const char** animation_names = cf_hashtable_items(sprite->animations);
                cf_sprite_play(&sprite->sprite, animation_names[animation_index]);
            }
        }
    }
    
    return entity;
}

ecs_id_t make_emote(ecs_id_t owner, const char* sprite_name, const char* emote_name, CF_V2 offset)
{
    ecs_t* ecs = s_app->ecs;
    C_Emoter* emoter = NULL;
    
    ecs_id_t component_emoter_id = ECS_GET_COMPONENT_ID(C_Emoter);
    
    if (ecs_is_ready(ecs, owner))
    {
        if (ecs_has(ecs, owner, component_emoter_id))
        {
            emoter = ecs_get(ecs, owner, component_emoter_id);
            if (emoter->emote_id != ECS_NULL && ecs_is_ready(ecs, emoter->emote_id))
            {
                // emoter is still making a face, don't spawn an emote
                if (ecs_has(ecs, emoter->emote_id, ECS_GET_COMPONENT_ID(C_Emote)))
                {
                    emoter = NULL;
                }
            }
        }
    }
    
    if (emoter == NULL)
    {
        return ECS_NULL;
    }
    
    ecs_id_t entity = ecs_create(ecs);
    
    ECS_ADD_COMPONENT(entity, C_Transform);
    C_Child_Transform* child_transform = ECS_ADD_COMPONENT(entity, C_Child_Transform);
    ECS_ADD_COMPONENT(entity, C_Elevation);
    C_Sprite* sprite = ECS_ADD_COMPONENT(entity, C_Sprite);
    C_Emote* emote = ECS_ADD_COMPONENT(entity, C_Emote);
    
    child_transform->parent = owner;
    child_transform->offset = offset;
    
    
    sprite->name = sprite_name;
    sprite->sprite = cf_make_sprite(sprite_name);
    sprite->sprite.scale = emoter->scale;
    cf_sprite_play(&sprite->sprite, emote_name);
    
    emote->owner = owner;
    
    f32 life_time = 0.0f;
    
    if (sprite->sprite.animation)
    {
        for (s32 index = 0; index < cf_array_count(sprite->sprite.animation->frames); ++index)
        {
            life_time += sprite->sprite.animation->frames[index].delay;
        }
    }
    
    C_Life_Time* c_life_time = ECS_ADD_COMPONENT(entity, C_Life_Time);
    set_life_time_duration(c_life_time, life_time);
    
    // successfully made an emote so make sure emoter knows about it
    emoter->emote_id = entity;
    
    return entity;
}

ecs_id_t make_projectile(ecs_id_t owner, V2i start, V2i end, f32 owner_elevation, s32 distance, const char* name)
{
    ecs_t* ecs = s_app->ecs;
    ecs_id_t entity = ecs_create(ecs);
    
    Asset_Resource* resource = assets_get_resource(name);
    for (s32 index = 0; index < cf_array_count(resource->properties); ++index)
    {
        Property* property = resource->properties + index;
        void* component = ECS_ADD_PROPERTY_COMPONENT(entity, property->key);
        if (property->value)
        {
            property_copy_to(property, component);
        }
    }
    
    C_Transform* transform = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Transform));
    C_Elevation* elevation = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Elevation));
    C_Projectile* projectile = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Projectile));
    C_Life_Time* life_time = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Life_Time));
    
    set_elevation_value(elevation, cf_max(get_tile_total_elevation(start), owner_elevation));
    transform->position = v2i_to_v2_iso_center(start, elevation->value);
    projectile->start_elevation = elevation->value;
    
    projectile->owner = owner;
    distance = cf_max(distance, 10);
    fixed V2i* line = make_tile_ray(start, end, distance);
    projectile->line = NULL;
    cf_array_set(projectile->line, line);
    
    if (ecs_is_ready(ecs, owner))
    {
        C_Team* owner_team = ECS_GET_COMPONENT(owner, C_Team);
        if (owner_team)
        {
            projectile->team_id = owner_team->id;
        }
    }
    
    return entity;
}

ecs_id_t make_elevation_effector(V2i tile, f32 impulse, f32 start_radius, f32 end_radius, b32 ignore_start_tile)
{
    ecs_t* ecs = s_app->ecs;
    
    ecs_id_t entity = ecs_create(ecs);
    C_Unit_Transform* unit_transform = ECS_ADD_COMPONENT(entity, C_Unit_Transform);
    C_Elevation_Effector* elevation_effector = ECS_ADD_COMPONENT(entity, C_Elevation_Effector);
    C_Life_Time* life_time = ECS_ADD_COMPONENT(entity, C_Life_Time);
    
    set_unit_transform_tile(unit_transform, tile);
    
    elevation_effector->impulse = impulse;
    elevation_effector->start_radius = start_radius;
    elevation_effector->end_radius = end_radius;
    elevation_effector->ignore_start_tile = ignore_start_tile;
    
    set_life_time_duration(life_time, 0.5f);
    
    return entity;
}

ecs_id_t make_tile_filler(V2i tile, f32 delay, f32 speed, s8 end_elevation)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    
    C_Tile_Filler* tile_filler = ECS_ADD_COMPONENT(entity, C_Tile_Filler);
    
    tile_filler->position = tile;
    tile_filler->delay = delay;
    tile_filler->speed = speed;
    tile_filler->end_elevation = end_elevation;
    
    return entity;
}

ecs_id_t make_tile_mover(V2i tile, f32 delay, f32 speed, f32 end_offset)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    
    C_Tile_Mover* tile_mover = ECS_ADD_COMPONENT(entity, C_Tile_Mover);
    
    tile_mover->position = tile;
    tile_mover->delay = delay;
    tile_mover->speed = speed;
    tile_mover->end_offset = end_offset;
    
    return entity;
}

void destroy_entity(ecs_id_t entity)
{
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(entity, C_Asset_Resource);
    C_Unit_Transform* unit_transform = ECS_GET_COMPONENT(entity, C_Unit_Transform);
    C_Transform* transform = ECS_GET_COMPONENT(entity, C_Transform);
    C_Elevation* elevation = ECS_GET_COMPONENT(entity, C_Elevation);
    
    V2i* tile = NULL;
    if (unit_transform)
    {
        tile = &unit_transform->prev_tile;
    }
    else
    {
        C_Projectile* projectile = ECS_GET_COMPONENT(entity, C_Projectile);
        C_Life_Time* life_time = ECS_GET_COMPONENT(entity, C_Life_Time);
        
        if (projectile && life_time)
        {
            f32 t = 1.0f - life_time->duration / life_time->total;
            s32 line_index = (s32)CF_FLOORF(t * cf_array_count(projectile->line));
            line_index = cf_clamp_int(line_index, 0, cf_array_count(projectile->line) - 1);
            
            tile = projectile->line + line_index;
        }
    }
    
    if (tile && transform && elevation)
    {
        make_event_on_destroy(*tile, transform->prev_position, elevation->prev_value, asset_resource->name);
    }
    ecs_destroy(s_app->ecs, entity);
}

ecs_id_t make_pickup(V2i tile, Pickup_Params params)
{
    ecs_t* ecs = s_app->ecs;
    ecs_id_t entity = ecs_create(ecs);
    
    Asset_Resource* resource = assets_get_resource("pickup_ammunition");
    for (s32 index = 0; index < cf_array_count(resource->properties); ++index)
    {
        Property* property = resource->properties + index;
        void* component = ECS_ADD_PROPERTY_COMPONENT(entity, property->key);
        if (property->value)
        {
            CF_MEMCPY(component, property->value, property->size);
        }
    }
    
    C_Sprite* sprite = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Sprite));
    C_Transform* transform = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Transform));
    C_Unit_Transform* unit_transform = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Unit_Transform));
    C_Elevation* elevation = ecs_get(ecs, entity, ECS_GET_COMPONENT_ID(C_Elevation));
    
    set_elevation_value(elevation, get_tile_total_elevation(tile));
    set_unit_transform_tile(unit_transform, tile);
    
    transform->position = v2i_to_v2_iso_center(tile, elevation->value);
    
    sprite->sprite = cf_make_sprite(sprite->name);
    sprite->sprite.scale = sprite->scale;
    
    const char* anim_default = NULL;
    if (cf_hashtable_has(sprite->animations, cf_sintern("default")))
    {
        anim_default = cf_hashtable_get(sprite->animations, cf_sintern("default"));
        cf_sprite_play(&sprite->sprite, anim_default);
    }
    
    return entity;
}

ecs_id_t make_pickup_ammunition(V2i tile, s32 count)
{
    Pickup_Params params = 
    {
        .type = PickupType_Ammunition,
        .count = count,
    };
    
    return make_pickup(tile, params);
}

CF_V2 get_unit_emote_offset()
{
    CF_V2 offset = assets_get_tile_size();
    offset.x = 0;
    return offset;
}

void do_emote(ecs_id_t owner, Emoter_Rule rule)
{
    if (cf_array_count(rule.emotes))
    {
        s32 index = cf_rnd_range_int(&s_app->world->ai_rnd, 0, cf_array_count(rule.emotes) - 1);
        CF_V2 offset = get_unit_emote_offset();
        make_emote(owner, rule.emotes[index].sprite, rule.emotes[index].animation, offset);
    }
}

void try_emote(ecs_id_t owner, Emoter_Rule rule)
{
    f32 emote_chance = cf_rnd_float(&s_app->world->ai_rnd);
    if (emote_chance < rule.chance)
    {
        do_emote(owner, rule);
    }
}

// ---------------
// entity events
// ---------------

ecs_id_t make_event_ground_impact(V2i tile, f32 impact, b32 ignore_start_tile)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_Ground_Impact;
    event->ground_impact.tile = tile;
    event->ground_impact.impact = impact;
    event->ground_impact.ignore_start_tile = ignore_start_tile;
    
    return entity;
}

ecs_id_t make_event_load_level(const char* name)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_Load_Level;
    event->load_level.name = string_clone(name);
    
    return entity;
}

ecs_id_t make_event_on_alert(ecs_id_t owner, ecs_id_t target)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Alert;
    event->on_alert.owner = owner;
    event->on_alert.target = target;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    C_Asset_Resource* target_resource = ECS_GET_COMPONENT(target, C_Asset_Resource);
    event->on_alert.owner_resource_name = owner_resource->name;
    event->on_alert.target_resource_name = target_resource->name;
    return entity;
}

ecs_id_t make_event_on_idle(ecs_id_t owner)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Idle;
    event->on_idle.owner = owner;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    event->on_idle.owner_resource_name = owner_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_hit(ecs_id_t owner, ecs_id_t target)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Hit;
    event->on_hit.owner = owner;
    event->on_hit.target = target;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    C_Asset_Resource* target_resource = ECS_GET_COMPONENT(target, C_Asset_Resource);
    event->on_hit.owner_resource_name = owner_resource->name;
    event->on_hit.target_resource_name = target_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_kill(ecs_id_t owner, ecs_id_t target)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Kill;
    event->on_kill.owner = owner;
    event->on_kill.target = target;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    C_Asset_Resource* target_resource = ECS_GET_COMPONENT(target, C_Asset_Resource);
    event->on_kill.owner_resource_name = owner_resource->name;
    event->on_kill.target_resource_name = target_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_dead(ecs_id_t owner)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Dead;
    event->on_dead.owner = owner;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    event->on_dead.owner_resource_name = owner_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_fire(ecs_id_t owner, CF_V2 position, V2i tile, f32 elevation)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Fire;
    event->on_fire.owner = owner;
    event->on_fire.position = position;
    event->on_fire.tile = tile;
    event->on_fire.elevation = elevation;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    event->on_fire.owner_resource_name = owner_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_pickup(ecs_id_t owner, const char* asset_resource_name)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Pickup;
    event->on_pickup.owner = owner;
    event->on_pickup.asset_resource_name = asset_resource_name;
    
    C_Asset_Resource* owner_resource = ECS_GET_COMPONENT(owner, C_Asset_Resource);
    event->on_pickup.owner_resource_name = owner_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_touch(ecs_id_t toucher, ecs_id_t touched)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Touch;
    event->on_touch.toucher = toucher;
    event->on_touch.touched = touched;
    
    C_Asset_Resource* toucher_resource = ECS_GET_COMPONENT(toucher, C_Asset_Resource);
    C_Asset_Resource* touched_resource = ECS_GET_COMPONENT(touched, C_Asset_Resource);
    event->on_touch.toucher_resource_name = touched_resource->name;
    event->on_touch.touched_resource_name = toucher_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_slip(ecs_id_t toucher, ecs_id_t touched)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Slip;
    event->on_slip.toucher = toucher;
    event->on_slip.touched = touched;
    
    C_Asset_Resource* toucher_resource = ECS_GET_COMPONENT(toucher, C_Asset_Resource);
    C_Asset_Resource* touched_resource = ECS_GET_COMPONENT(touched, C_Asset_Resource);
    event->on_slip.toucher_resource_name = touched_resource->name;
    event->on_slip.touched_resource_name = toucher_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_switch(ecs_id_t switch_entity, V2i tile)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Switch;
    event->on_switch.entity = switch_entity;
    event->on_switch.tile = tile;
    
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(switch_entity, C_Asset_Resource);
    event->on_switch.resource_name = asset_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_switch_reset(ecs_id_t switch_entity)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Switch_Reset;
    event->on_switch_reset.entity = switch_entity;
    
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(switch_entity, C_Asset_Resource);
    event->on_switch_reset.resource_name = asset_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_destroy(V2i tile, CF_V2 position, f32 elevation, const char* asset_resource_name)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Destroy;
    event->on_destroy.tile = tile;
    event->on_destroy.position = position;
    event->on_destroy.elevation = elevation;
    event->on_destroy.resource_name = asset_resource_name;
    
    return entity;
}

ecs_id_t make_event_do_select_control_unit(ecs_id_t select_entity)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_Do_Select_Control_Unit;
    event->select_control_unit.entity = select_entity;
    
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(select_entity, C_Asset_Resource);
    event->select_control_unit.resource_name = asset_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_select_control_unit(ecs_id_t select_entity)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Select_Control_Unit;
    event->select_control_unit.entity = select_entity;
    
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(select_entity, C_Asset_Resource);
    event->select_control_unit.resource_name = asset_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_deselect_control_unit(ecs_id_t select_entity)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_Deselect_Control_Unit;
    event->select_control_unit.entity = select_entity;
    
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(select_entity, C_Asset_Resource);
    event->select_control_unit.resource_name = asset_resource->name;
    
    return entity;
}

ecs_id_t make_event_on_ui_hover_control_unit(ecs_id_t select_entity)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_Event* event = ECS_ADD_COMPONENT(entity, C_Event);
    event->type = Event_Type_On_UI_Hover_Control_Unit;
    event->select_control_unit.entity = select_entity;
    
    C_Asset_Resource* asset_resource = ECS_GET_COMPONENT(select_entity, C_Asset_Resource);
    event->select_control_unit.resource_name = asset_resource->name;
    
    return entity;
}


// ---------------
// ai events
// ---------------

ecs_id_t make_ai_event_on_alert(s32 team_id, ecs_id_t target, V2i tile, s32 radius)
{
    ecs_id_t entity = ecs_create(s_app->ecs);
    C_AI_Event* event = ECS_ADD_COMPONENT(entity, C_AI_Event);
    event->type = AI_Event_Type_On_Alert;
    event->on_alert.team_id = team_id;
    event->on_alert.target = target;
    event->on_alert.tile = tile;
    event->on_alert.radius = radius;
    
    cf_array_push(s_app->world->level.ai_event_queue, entity);
    
    return entity;
}

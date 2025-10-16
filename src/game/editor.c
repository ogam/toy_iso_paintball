#include "game/editor.h"

void editor_init()
{
    Assets* assets = s_app->assets;
    Editor* editor = cf_calloc(sizeof(Editor), 1);
    s_app->editor = editor;
    
    s32 string_size = 256 + sizeof(CF_Ahdr);
    char* buf = cf_alloc(string_size * 4);
    cf_string_static(editor->file_name, buf, string_size);
    cf_string_static(editor->name, buf + string_size, string_size);
    cf_string_static(editor->music_file_name, buf + string_size * 2, string_size);
    cf_string_static(editor->background_file_name, buf + string_size * 3, string_size);
    
    cf_array_fit(editor->auto_tile_queue, 1024);
    
    s32 count = v2i_size(LEVEL_SIZE_MAX);
    
    // setup layers
    {
        s32 resources_count = cf_hashtable_count(assets->resources);
        Asset_Resource* resources = cf_hashtable_items(assets->resources);
        
        static dyna b32* show_categories = NULL;
        
        // setup categories
        pq const char** categories = NULL;
        MAKE_SCRATCH_PQ(categories, resources_count);
        
        // there is always a tile layer, it should in most cases be first
        pq_add_weight(categories, cf_sintern(EDITOR_TILE_LAYER_NAME), -1);
        
        for (s32 index = 0; index < resources_count; ++index)
        {
            if (resources[index].editor.category)
            {
                pq_add_weight(categories, cf_sintern(resources[index].editor.category), 1);
            }
        }
        
        editor->layer_names = cf_calloc(sizeof(const char*), pq_count(categories));
        editor->layers = cf_calloc(sizeof(Asset_Object_ID*), pq_count(categories));
        for (s32 index = 0; index < pq_count(categories); ++index)
        {
            editor->layer_names[index] = cf_sintern(categories[index]);
            editor->layers[index] = cf_calloc(sizeof(Asset_Object_ID), count);
        }
        
        editor->layer_count = pq_count(categories);
    }
    
    cf_array_fit(editor->redos, 512);
    cf_array_fit(editor->undos, 512);
    
    editor_reset();
    editor_set_state(Editor_State_None);
}

void editor_input_update()
{
    Input* input = s_app->input;
    Editor* editor = s_app->editor;
    Editor_Input* editor_input = &editor->input;
    
    b32 redo = (cf_key_just_pressed(CF_KEY_Y) || cf_key_repeating(CF_KEY_Y)) && cf_key_ctrl();
    b32 undo = (cf_key_just_pressed(CF_KEY_Z) || cf_key_repeating(CF_KEY_Z)) && cf_key_ctrl();
    b32 place = cf_mouse_down(CF_MOUSE_BUTTON_LEFT);
    b32 remove = cf_mouse_down(CF_MOUSE_BUTTON_RIGHT);
    b32 do_next_brush = cf_key_just_pressed(CF_KEY_TAB);
    b32 do_prev_brush = cf_key_just_pressed(CF_KEY_TAB) && cf_key_shift();
    b32 any_brush_pressed = cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT);
    b32 switch_floodfill_mode = cf_key_just_pressed(CF_KEY_F);
    b32 switch_brush_mode = cf_key_just_pressed(CF_KEY_B);
    b32 switch_auto_tiling = cf_key_just_pressed(CF_KEY_T);
    b32 pan_up = cf_key_down(CF_KEY_W) || cf_key_down(CF_KEY_UP);
    b32 pan_down = cf_key_down(CF_KEY_S) || cf_key_down(CF_KEY_DOWN);
    b32 pan_left = cf_key_down(CF_KEY_A) || cf_key_down(CF_KEY_LEFT);
    b32 pan_right = cf_key_down(CF_KEY_D) || cf_key_down(CF_KEY_RIGHT);
    
    float pan_speed = 10.0f;
    
    CF_V2 motion = cf_v2(0, 0);
    if (cf_mouse_down(CF_MOUSE_BUTTON_MIDDLE) || cf_key_down(CF_KEY_SPACE))
    {
        motion = cf_v2(cf_mouse_motion_x(), cf_mouse_motion_y());
    }
    if (pan_up)
    {
        motion.y -= 1.0f * pan_speed;
    }
    if (pan_down)
    {
        motion.y += 1.0f * pan_speed;
    }
    if (pan_right)
    {
        motion.x -= 1.0f * pan_speed;
    }
    if (pan_left)
    {
        motion.x += 1.0f * pan_speed;
    }
    
    if (input->multiselect == Input_Multiselect_State_Finish)
    {
        place = false;
        remove = false;
    }
    else if (input->multiselect == Input_Multiselect_State_Commit)
    {
        place = cf_mouse_just_released(CF_MOUSE_BUTTON_LEFT);
        remove = cf_mouse_just_released(CF_MOUSE_BUTTON_RIGHT);
    }
    
    if (game_ui_is_hovering_over_any_layouts() || ui_is_any_selected())
    {
        place = false;
        remove = false;
        switch_floodfill_mode = false;
        switch_brush_mode = false;
        any_brush_pressed = false;
    }
    
    s32 next_brush_direction = 0;
    if (do_prev_brush)
    {
        next_brush_direction = -1;
    }
    else if (do_next_brush)
    {
        next_brush_direction = 1;
    }
    
    editor_input->redo = redo;
    editor_input->undo = undo;
    editor_input->place = place;
    editor_input->remove = remove;
    editor_input->move_direction = motion;
    editor_input->next_brush_direction = next_brush_direction;
    editor_input->any_brush_pressed = any_brush_pressed;
    editor_input->switch_floodfill_mode = switch_floodfill_mode;
    editor_input->switch_brush_mode = switch_brush_mode;
    if (switch_auto_tiling)
    {
        editor->is_auto_tiling = !editor->is_auto_tiling;
    }
}

void editor_update()
{
    Editor* editor = s_app->editor;
    Editor_Input* editor_input = &editor->input;
    Input* input = s_app->input;
    if (editor->state != Editor_State_Edit)
    {
        return;
    }
    
    
    editor->time += CF_DELTA_TIME;
    
    editor_input_update();
    editor->position = cf_sub_v2(editor->position, editor_input->move_direction);
    editor->position = focus_camera(&editor->position, 1, 1.0f);
    
    if (editor_input->any_brush_pressed)
    {
        editor->command_id++;
    }
    
    editor->tile_change_delay -= CF_DELTA_TIME;
    b32 changed = false;
    
    if (editor_input->switch_floodfill_mode)
    {
        editor_brush_mode_set_floodfill();
    }
    if (editor_input->switch_brush_mode)
    {
        editor_brush_mode_set_brush();
    }
    
    if (editor->tile_change_delay <= 0.0f)
    {
        if (input->multiselect == Input_Multiselect_State_Commit)
        {
            if (editor_input->place)
            {
                changed = editor_do_brush_place_range(input->tile_select_start, input->tile_select_end);
            }
            else if (editor_input->remove)
            {
                changed = editor_do_brush_remove_range(input->tile_select_start, input->tile_select_end);
            }
        }
        else if (input->multiselect == Input_Multiselect_State_None)
        {
            if (editor_input->place)
            {
                changed = editor_do_brush_place(input->tile_select);
            }
            else if (editor_input->remove)
            {
                changed = editor_do_brush_remove(input->tile_select);
            }
        }
    }
    
    if (changed)
    {
        // initial click delay
        if (editor_input->any_brush_pressed)
        {
            editor->tile_change_delay = 0.20f;
        }
        else
        {
            editor->tile_change_delay = 0.05f;
        }
    }
    
    if (editor_input->redo)
    {
        editor_redo();
    }
    if (editor_input->undo)
    {
        editor_undo();
    }
    
    editor_process_auto_tiling();
}

void editor_draw()
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    V2i level_size = s_app->world->level.size;
    
    b32 can_draw = editor->state == Editor_State_Edit || editor->state == Editor_State_Pause;
    can_draw = can_draw && world->state != World_State_Demo;
    
    if (!can_draw)
    {
        return;
    }
    
    V2i min = v2i();
    V2i max = level_size;
    
    f32 other_layer_opacity = 0.1f;
    
    for (s32 layer_index = 0; layer_index < editor->layer_count; ++layer_index)
    {
        Asset_Object_ID* layer = editor->layers[layer_index];
        for (s32 y = min.x; y < max.y; ++y)
        {
            for (s32 x = min.x; x < max.x; ++x)
            {
                V2i tile = v2i(.x = x, .y = y);
                f32 elevation = get_tile_total_elevation(tile);
                CF_V2 position = v2i_to_v2_iso_center(tile, elevation);
                if (!is_culled(position))
                {
                    s32 index = x + y * level_size.x;
                    Asset_Resource* resource = assets_get_resource_from_id(layer[index]);
                    if (resource && resource->type != Asset_Resource_Type_Tile)
                    {
                        const char* sprite_name = resource->editor.reference.sprite;
                        const char* animation = resource->editor.reference.animation;
                        CF_V2 scale = resource->editor.scale;
                        
                        CF_Sprite sprite = cf_make_sprite(sprite_name);;
                        
                        cf_sprite_play(&sprite, animation);
                        sprite.scale = scale;
                        sprite.transform.p = position;
                        
                        draw_push_sprite(Draw_Sort_Key_Type_Unit, tile, elevation, &sprite);
                    }
                }
            }
        }
    }
}

void editor_reset()
{
    Editor* editor = s_app->editor;
    editor->position = cf_v2(0, 0);
    editor->brush = 0;
    editor->command_id = 0;
    editor->time = 0;
    cf_array_clear(editor->redos);
    cf_array_clear(editor->undos);
    editor->stack_index = -1;
    
    cf_string_clear(editor->file_name);
    cf_string_clear(editor->name);
    cf_string_clear(editor->music_file_name);
    cf_string_clear(editor->background_file_name);
    
    editor->level_size = LEVEL_SIZE_MIN;
    editor->elevation_min = EDITOR_DEFAULT_ELEVATION_MIN;
    editor->elevation_max = EDITOR_DEFAULT_ELEVATION_MAX;
    
    editor_set_state(Editor_State_None);
    s32 count = v2i_size(LEVEL_SIZE_MAX);
    for (s32 layer_index = 0; layer_index < editor->layer_count; ++layer_index)
    {
        CF_MEMSET(editor->layers[layer_index], 0, sizeof(Asset_Object_ID) * count);
    }
}

b32 editor_is_active()
{
    return s_app->editor->time > 0.0f;
}

void editor_set_state(Editor_State state)
{
    s_app->editor->state = state;
}

// soft reset, only flattens all dynamic elevations, clears out ecs and any collision trees
void editor_resume_from_play()
{
    Editor* editor = s_app->editor;
    const char* actual_file_name = string_clone(editor->file_name);
    editor_load_level(EDITOR_LEVEL_NAME, false);
    string_set(editor->file_name, actual_file_name);
}

void editor_apply_command(Editor_Command* command)
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    if (command->type == Editor_Command_Type_Tile)
    {
        Tile* tile = get_tile(command->tile.p);
        Asset_Object_ID* tile_id = get_tile_id(command->tile.p);
        *tile = command->tile.state.v;
        *tile_id = command->tile.state.id;
    }
    else if (command->type == Editor_Command_Type_Object)
    {
        s32 slot_index = get_tile_index(command->object.p);
        s32 layer_index = command->object.layer;
        editor->layers[layer_index][slot_index] = command->object.v;
    }
    else if (command->type == Editor_Command_Type_Brush)
    {
        editor_brush_set(command->brush);
    }
    else if (command->type == Editor_Command_Type_Level_Size)
    {
        editor_adjust_level_stride(world->level.size, command->level_size);
        world->level.size = command->level_size;
    }
    else if (command->type == Editor_Command_Type_Elevation)
    {
        editor->elevation_min = command->elevation_range.x;
        editor->elevation_max = command->elevation_range.y;
    }
}

void editor_redo()
{
    Editor* editor = s_app->editor;
    
    Editor_Command* command = editor->redos + editor->stack_index + 1;
    s32 id = command->id;
    while (command->id == id && editor->stack_index + 1 < cf_array_count(editor->redos))
    {
        editor_apply_command(command);
        
        editor->stack_index++;
        command++;
    }
    
    editor->stack_index = cf_min(editor->stack_index, cf_array_count(editor->redos) - 1);
}

void editor_undo()
{
    Editor* editor = s_app->editor;
    
    Editor_Command* command = editor->undos + editor->stack_index;
    s32 id = command->id;
    while (command->id == id && editor->stack_index >= 0)
    {
        editor_apply_command(command);
        editor->stack_index--;
        command--;
    }
    
    editor->stack_index = cf_max(editor->stack_index, -1);
}

void editor_push_command(Editor_Command redo, Editor_Command undo)
{
    Editor* editor = s_app->editor;
    redo.id = editor->command_id;
    undo.id = editor->command_id;
    
    if (cf_array_count(editor->redos) > editor->stack_index + 1)
    {
        cf_array_len(editor->redos) = cf_max(editor->stack_index + 1, 0);
        cf_array_len(editor->undos) = cf_max(editor->stack_index + 1, 0);
    }
    
    cf_array_push(editor->redos, redo);
    cf_array_push(editor->undos, undo);
    CF_ASSERT(cf_array_count(editor->redos) == cf_array_count(editor->undos));
    editor->stack_index = cf_min(editor->stack_index + 1, cf_array_count(editor->redos) - 1);
}

void editor_push_command_tile_change(V2i position, Tile_State before, Tile_State after)
{
    Editor_Command redo_command = 
    {
        .type = Editor_Command_Type_Tile,
        .tile = 
        {
            .p = position,
            .state = after,
        }
    };
    
    Editor_Command undo_command = 
    {
        .type = Editor_Command_Type_Tile,
        .tile = 
        {
            .p = position,
            .state = before,
        }
    };
    
    editor_push_command(redo_command, undo_command);
}

void editor_push_command_object_change(V2i position, Asset_Object_ID before, Asset_Object_ID after, s32 layer)
{
    Editor_Command redo_command = 
    {
        .type = Editor_Command_Type_Object,
        .object = 
        {
            .p = position,
            .v = after,
            .layer = layer,
        }
    };
    
    Editor_Command undo_command = 
    {
        .type = Editor_Command_Type_Object,
        .object = 
        {
            .p = position,
            .v = before,
            .layer = layer,
        }
    };
    
    editor_push_command(redo_command, undo_command);
}

void editor_push_command_brush_change(Asset_Object_ID before, Asset_Object_ID after)
{
    Editor_Command redo_command = 
    {
        .type = Editor_Command_Type_Brush,
        .brush = after,
    };
    
    Editor_Command undo_command = 
    {
        .type = Editor_Command_Type_Brush,
        .brush = before,
    };
    
    editor_push_command(redo_command, undo_command);
}

void editor_push_command_level_size_change(V2i before, V2i after)
{
    Editor_Command redo_command = 
    {
        .type = Editor_Command_Type_Level_Size,
        .level_size = after,
    };
    
    Editor_Command undo_command = 
    {
        .type = Editor_Command_Type_Level_Size,
        .level_size = before,
    };
    
    editor_push_command(redo_command, undo_command);
}

void editor_push_command_elevation_change(V2i before, V2i after)
{
    Editor_Command redo_command = 
    {
        .type = Editor_Command_Type_Elevation,
        .elevation_range = after,
    };
    
    Editor_Command undo_command = 
    {
        .type = Editor_Command_Type_Elevation,
        .elevation_range = before,
    };
    
    editor_push_command(redo_command, undo_command);
}

void editor_brush_mode_set_floodfill()
{
    s_app->editor->brush_mode |= Editor_Brush_Mode_Floodfill;
}

void editor_brush_mode_set_brush()
{
    s_app->editor->brush_mode &= ~Editor_Brush_Mode_Floodfill;
}

b32 editor_brush_mode_is_floodfill()
{
    return (s_app->editor->brush_mode & Editor_Brush_Mode_Floodfill) == Editor_Brush_Mode_Floodfill;
}

b32 editor_brush_mode_is_brush()
{
    return (s_app->editor->brush_mode & Editor_Brush_Mode_Floodfill) != Editor_Brush_Mode_Floodfill;
}

void editor_brush_mode_set_auto_tiling(b32 true_to_auto_tile)
{
    s_app->editor->is_auto_tiling = true_to_auto_tile;
}

b32 editor_brush_mode_is_auto_tiling()
{
    return s_app->editor->is_auto_tiling;
}

void editor_process_auto_tiling()
{
    Editor* editor = s_app->editor;
    
    // ordered specifically to match tile bit order 
    V2i offsets[] = 
    {
        v2i(         .y =  1), // w
        v2i(.x =  1         ), // n
        v2i(         .y = -1), // e
        v2i(.x = -1         ), // s
    };
    
    for (s32 index = 0; index < cf_array_count(editor->auto_tile_queue); ++index)
    {
        V2i tile = editor->auto_tile_queue[index];
        Tile* tile_ptr = get_tile(tile);
        Asset_Object_ID* tile_id = get_tile_id(tile);
        if (tile_ptr && tile_id && *tile_id)
        {
            Tile_State before = {
                .v = *tile_ptr,
                .id = *tile_id,
            };
            
            // set this to be connectable to all sides before being filtered down
            tile_ptr->tiling = 0xFF;
            for (s32 offset_index = 0; offset_index < CF_ARRAY_SIZE(offsets); ++offset_index)
            {
                V2i offset = v2i_add(tile, offsets[offset_index]);
                Tile* offset_ptr = get_tile(offset);
                Asset_Object_ID* offset_id = get_tile_id(offset);
                // only do auto tiling against same ids
                if (offset_ptr && offset_id && *offset_id && *offset_id == *tile_id)
                {
                    // if tile is below or same elevation as offset then close that connection
                    // otherwise if tile is above leave that connection open
                    if (tile_ptr->elevation <= offset_ptr->elevation)
                    {
                        tile_ptr->tiling &= ~(1 << offset_index);
                    }
                }
            }
            
            Tile_State after = {
                .v = *tile_ptr,
                .id = *tile_id,
            };
            
            editor_push_command_tile_change(tile, before, after);
        }
    }
    
    cf_array_clear(editor->auto_tile_queue);
}

void editor_add_auto_tile(V2i tile)
{
    Editor* editor = s_app->editor;
    V2i offsets[] = 
    {
        v2i(         .y =  1), // w
        v2i(.x =  1         ), // n
        v2i(         .y = -1), // e
        v2i(.x = -1         ), // s
    };
    
    cf_array_push(editor->auto_tile_queue, tile);
    
    for (s32 offset_index = 0; offset_index < CF_ARRAY_SIZE(offsets); ++offset_index)
    {
        V2i offset = v2i_add(tile, offsets[offset_index]);
        if (is_tile_in_bounds(offset))
        {
            //  @todo:  ideally this should be a set so the tile only gets added once
            //          for now it's fine   
            cf_array_push(editor->auto_tile_queue, offset);
        }
    }
}

void editor_brush_set(Asset_Object_ID id)
{
    Editor* editor = s_app->editor;
    Asset_Resource* resource = assets_get_resource_from_id(id);
    if (resource)
    {
        for (s32 index = 0; index < editor->layer_count; ++index)
        {
            if (editor->layer_names[index] == cf_sintern(resource->editor.category))
            {
                editor->layer_index = index;
                break;
            }
        }
        
        editor->brush = id;
    }
}

b32 editor_brush_is_selected(Asset_Resource* resource)
{
    Editor* editor = s_app->editor;
    b32 is_selected = editor->brush == resource->id;
    return is_selected;
}

b32 editor_brush_select(Asset_Resource* resource)
{
    Editor* editor = s_app->editor;
    b32 brush_changed = editor->brush != resource->id;
    
    if (brush_changed)
    {
        editor_push_command_brush_change(editor->brush, resource->id);
    }
    editor_brush_set(resource->id);
    
    return brush_changed;
}

void editor_brush_select_next(fixed Asset_Resource** resources)
{
    Assets* assets = s_app->assets;
    Editor* editor = s_app->editor;
    Editor_Input* editor_input = &editor->input;
    
    if (editor_input->next_brush_direction)
    {
        editor->command_id++;
        
        s32 found_index = -1;
        
        for (s32 resource_index = 0; resource_index < cf_array_size(resources); ++resource_index)
        {
            Asset_Resource* resource = resources[resource_index];
            if (editor_brush_is_selected(resource))
            {
                found_index = resource_index;
                break;
            }
        }
        
        if (cf_array_count(resources))
        {
            s32 next_index = (found_index + editor_input->next_brush_direction + cf_array_count(resources)) % cf_array_count(resources);
            if (found_index < 0)
            {
                next_index = 0;
            }
            editor_brush_select(resources[next_index]);
        }
    }
}

b32 editor_do_brush_place(V2i tile)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    Asset_Resource* resource = assets_get_resource_from_id(editor->brush);
    if (resource)
    {
        if (is_tile_in_bounds(tile))
        {
            if (editor->brush_mode & Editor_Brush_Mode_Floodfill)
            {
                changed = editor_do_brush_floodfill(tile, resource->id);
            }
            else
            {
                // update tile
                if (resource->type == Asset_Resource_Type_Tile)
                {
                    changed = editor_do_brush_tile_place(tile, (u8)resource->id);
                }
                
                // update editor layer
                {
                    Asset_Object_ID* layer = editor->layers[editor->layer_index];
                    s32 index = get_tile_index(tile);
                    
                    Asset_Object_ID before = layer[index];
                    layer[index] = resource->id;
                    Asset_Object_ID after = layer[index];
                    editor_push_command_object_change(tile, before, after, editor->layer_index);
                    
                    changed = changed || before != after;;
                }
            }
        }
    }
    
    return changed;
}

b32 editor_do_brush_remove(V2i tile)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    Asset_Resource* resource = NULL;
    
    // update editor layer
    if (is_tile_in_bounds(tile))
    {
        Asset_Object_ID* layer = editor->layers[editor->layer_index];
        s32 index = get_tile_index(tile);
        resource = assets_get_resource_from_id(layer[index]);
        
        if (editor->brush_mode & Editor_Brush_Mode_Floodfill)
        {
            changed = editor_do_brush_floodfill(tile, 0);
        }
        else
        {
            b32 can_remove_object = false;
            if (resource)
            {
                if (resource->type == Asset_Resource_Type_Tile)
                {
                    changed = editor_do_brush_tile_remove(tile, Editor_Brush_Mode_Default);
                    can_remove_object = is_tile_empty(tile);
                }
                else
                {
                    can_remove_object = true;
                }
            }
            
            if (can_remove_object)
            {
                Asset_Object_ID before = layer[index];
                layer[index] = 0;
                Asset_Object_ID after = layer[index];
                editor_push_command_object_change(tile, before, after, editor->layer_index);
                changed = before != after || changed;
            }
        }
    }
    
    return changed;
}

b32 editor_do_brush_place_range(V2i start, V2i end)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    Asset_Object_ID* layer = editor->layers[editor->layer_index];
    
    Asset_Resource* resource = assets_get_resource_from_id(editor->brush);
    if (resource)
    {
        if (resource->type == Asset_Resource_Type_Tile)
        {
            // walk through all tiles in range and update
            editor_do_brush_tile_place_range(start, end, (u8)resource->id);
        }
        
        V2i min = v2i_min(start, end);
        V2i max = v2i_max(start, end);
        
        for (s32 y = min.y; y <= max.y; ++y)
        {
            for (s32 x = min.x; x <= max.x; ++x)
            {
                V2i tile = v2i(.x = x, .y = y);
                if (is_tile_in_bounds(tile))
                {
                    s32 index = get_tile_index(tile);
                    
                    Asset_Object_ID before = layer[index];
                    layer[index] = resource->id;
                    Asset_Object_ID after = layer[index];
                    editor_push_command_object_change(tile, before, after, editor->layer_index);
                }
            }
        }
    }
    
    return changed;
}

b32 editor_do_brush_remove_range(V2i start, V2i end)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    V2i min = v2i_min(start, end);
    V2i max = v2i_max(start, end);
    
    Asset_Object_ID* layer = editor->layers[editor->layer_index];
    
    for (s32 y = min.y; y <= max.y; ++y)
    {
        for (s32 x = min.x; x <= max.x; ++x)
        {
            V2i tile = v2i(.x = x, .y = y);
            if (is_tile_in_bounds(tile))
            {
                s32 index = get_tile_index(tile);
                Asset_Resource* resource = assets_get_resource_from_id(layer[index]);
                if (resource)
                {
                    b32 can_remove_object = true;
                    if (resource->type == Asset_Resource_Type_Tile)
                    {
                        editor_do_brush_tile_remove(tile, Editor_Brush_Mode_Remove);
                        can_remove_object = is_tile_empty(tile);
                    }
                    
                    if (can_remove_object)
                    {
                        Asset_Object_ID before = layer[index];
                        layer[index] = 0;
                        Asset_Object_ID after = layer[index];
                        editor_push_command_object_change(tile, before, after, editor->layer_index);
                    }
                    
                    changed = true;
                }
            }
        }
    }
    
    return changed;
}

void editor_do_brush_tile_floodfill_impl(V2i tile, Asset_Object_ID* layer, Asset_Object_ID id, Asset_Object_ID next_id, s32 elevation)
{
    if (is_tile_in_bounds(tile))
    {
        s32 index = get_tile_index(tile);
        Tile* tile_ptr = get_tile(tile);
        if (layer[index] == id && elevation == tile_ptr->elevation)
        {
            Tile_State before = {
                .v = *tile_ptr,
                .id = id,
            };
            
            tile_set_minimum_elevation(tile, next_id, elevation);
            
            Tile_State after = {
                .v = *tile_ptr,
                .id = next_id,
            };
            
            editor_push_command_tile_change(tile, before, after);
            
            if (s_app->editor->is_auto_tiling)
            {
                editor_add_auto_tile(tile);
            }
            
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x + 1, .y = tile.y    ), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x + 1, .y = tile.y + 1), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x    , .y = tile.y + 1), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x - 1, .y = tile.y + 1), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x - 1, .y = tile.y    ), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x - 1, .y = tile.y - 1), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x    , .y = tile.y - 1), layer, id, next_id, elevation);
            editor_do_brush_tile_floodfill_impl(v2i(.x = tile.x + 1, .y = tile.y - 1), layer, id, next_id, elevation);
        }
    }
}

void editor_do_brush_floodfill_impl(V2i tile, Asset_Object_ID* layer, s32 layer_index, Asset_Object_ID id, Asset_Object_ID next_id, s32 elevation)
{
    if (is_tile_in_bounds(tile))
    {
        s32 index = get_tile_index(tile);
        s32 tile_elevation = get_tile_elevation(tile);
        if (layer[index] == id && elevation == tile_elevation)
        {
            editor_push_command_object_change(tile, id, next_id, layer_index);
            layer[index] = next_id;
            
            editor_do_brush_floodfill_impl(v2i(.x = tile.x + 1, .y = tile.y    ), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x + 1, .y = tile.y + 1), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x    , .y = tile.y + 1), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x - 1, .y = tile.y + 1), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x - 1, .y = tile.y    ), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x - 1, .y = tile.y - 1), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x    , .y = tile.y - 1), layer, layer_index, id, next_id, elevation);
            editor_do_brush_floodfill_impl(v2i(.x = tile.x + 1, .y = tile.y - 1), layer, layer_index, id, next_id, elevation);
        }
    }
}

b32 editor_do_brush_floodfill(V2i tile, Asset_Object_ID id)
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    if (is_tile_in_bounds(tile))
    {
        s32 index = get_tile_index(tile);
        s32 elevation = get_tile_elevation(tile);
        Asset_Object_ID* layer = editor->layers[editor->layer_index];
        s32 old_id = layer[index];
        if (old_id != id)
        {
            editor_do_brush_floodfill_impl(tile, layer, editor->layer_index, old_id, id, elevation);
            if (editor->layer_names[editor->layer_index] == cf_sintern(EDITOR_TILE_LAYER_NAME))
            {
                editor_do_brush_tile_floodfill_impl(tile, world->level.tile_ids, old_id, id, elevation);
            }
            changed = true;
        }
    }
    
    return changed;
}

b32 editor_do_brush_tile_place(V2i tile, Asset_Object_ID id)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    Asset_Resource* resource = assets_get_resource_from_id(id);
    
    if (tile_ptr && resource && resource->type == Asset_Resource_Type_Tile)
    {
        Tile_State before = {
            .v = *tile_ptr,
            .id = *tile_id,
        };
        
        if (tile_place_or_raise(tile, id, 1, editor->elevation_max))
        {
            tile_set_minimum_elevation(tile, id, editor->elevation_min);
            Tile_State after = {
                .v = *tile_ptr,
                .id = *tile_id,
            };
            
            editor_push_command_tile_change(tile, before, after);
            changed = true;
            
            if (editor->is_auto_tiling)
            {
                editor_add_auto_tile(tile);
            }
        }
    }
    
    return changed;
}

b32 editor_do_brush_tile_remove(V2i tile, Editor_Brush_Mode mode)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    Tile* tile_ptr = get_tile(tile);
    Asset_Object_ID* tile_id = get_tile_id(tile);
    if (tile_ptr && tile_id)
    {
        Tile_State before = {
            .v = *tile_ptr,
            .id = *tile_id,
        };
        
        if (mode & Editor_Brush_Mode_Remove)
        {
            changed = tile_remove(tile);
        }
        else
        {
            changed = tile_remove_or_lower(tile, 1, editor->elevation_min);
        }
        
        if (changed)
        {
            Tile_State after = {
                .v = *tile_ptr,
                .id = *tile_id,
            };
            
            editor_push_command_tile_change(tile, before, after);
            
            if (editor->is_auto_tiling)
            {
                editor_add_auto_tile(tile);
            }
        }
    }
    return changed;
}

b32 editor_do_brush_tile_place_range(V2i start, V2i end, Asset_Object_ID id)
{
    Editor* editor = s_app->editor;
    b32 changed = false;
    
    V2i min = v2i_min(start, end);
    V2i max = v2i_max(start, end);
    
    s32 minimum_elevation = S32_MAX;
    b32 all_tiles_same_type = true;
    
    Asset_Resource* resource = assets_get_resource_from_id((Asset_Object_ID)id);
    if (resource && resource->type != Asset_Resource_Type_Tile)
    {
        return false;
    }
    
    s8 walkable = *(s8*)resource_get(resource, "walkable");
    
    for (s32 y = min.y; y <= max.y; ++y)
    {
        for (s32 x = min.x; x <= max.x; ++x)
        {
            V2i tile = v2i(.x = x, .y = y);
            Tile* tile_ptr = get_tile(tile);
            Asset_Object_ID* tile_id = get_tile_id(tile);
            if (tile_ptr && tile_id)
            {
                if (*tile_id == id)
                {
                    minimum_elevation = cf_min(minimum_elevation, tile_ptr->elevation);
                }
                
                if (*tile_id != id)
                {
                    all_tiles_same_type = false;
                }
            }
        }
    }
    
    // failed to find any tiles in range, so we're going to be placing down new tiles
    if (minimum_elevation == S32_MAX)
    {
        minimum_elevation = 0;
    } 
    else if (all_tiles_same_type)
    {
        // we're trying to place tiles on top of same tile type, move these up 1 elevation
        minimum_elevation++;
    }
    
    minimum_elevation = cf_max(minimum_elevation, editor->elevation_min);
    
    for (s32 y = min.y; y <= max.y; ++y)
    {
        for (s32 x = min.x; x <= max.x; ++x)
        {
            V2i tile = v2i(.x = x, .y = y);
            Tile* tile_ptr = get_tile(tile);
            Asset_Object_ID* tile_id = get_tile_id(tile);
            if (tile_ptr && tile_id)
            {
                Tile_State before = {
                    .v = *tile_ptr,
                    .id = *tile_id,
                };
                
                if (tile_set_minimum_elevation(tile, id, minimum_elevation))
                {
                    changed = true;
                    Tile_State after = {
                        .v = *tile_ptr,
                        .id = *tile_id,
                    };
                    
                    editor_push_command_tile_change(tile, before, after);
                    
                    if (editor->is_auto_tiling)
                    {
                        editor_add_auto_tile(tile);
                    }
                }
            }
        }
    }
    
    return changed;
}

void editor_adjust_level_stride(V2i before, V2i after)
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    Tile* tiles = world->level.tiles;
    
    s32 old_stride = before.x;
    s32 new_stride = after.x;
    
    fixed Asset_Object_ID** layers = NULL;
    MAKE_SCRATCH_ARRAY(layers, editor->layer_count + 1);
    
    cf_array_push(layers, world->level.tile_ids);
    for (s32 layer_index = 0; layer_index < editor->layer_count; ++layer_index)
    {
        cf_array_push(layers, editor->layers[layer_index]);
    }
    
    if (old_stride != new_stride)
    {
        fixed RLE* lines = NULL;
        MAKE_SCRATCH_ARRAY(lines, before.y);
        s32 count = v2i_size(LEVEL_SIZE_MAX);
        u64 layer_size = sizeof(Asset_Object_ID) * count;
        
        // layers
        for (s32 layer_index = 0; layer_index < cf_array_count(layers); ++layer_index)
        {
            Asset_Object_ID* layer = layers[layer_index];
            
            grid_to_rle(before, (s32*)layer, lines);
            CF_MEMSET(layer, 0, layer_size);
            
            rle_to_grid(after, (s32*)layer, lines);
        }
        
        // tiles
        {
            cf_array_clear(lines);
            grid_to_rle(before, (s32*)tiles, lines);
            
            CF_MEMSET(tiles, 0, sizeof(tiles[0]) * count);
            
            rle_to_grid(after, (s32*)tiles, lines);
        }
    }
}

void editor_set_level_size(V2i size)
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    
    V2i before = world->level.size;
    world->level.size = size;
    V2i after = world->level.size;
    
    editor_adjust_level_stride(before, after);
    
    if (v2i_distance(before, after) > 0)
    {
        editor->command_id++;
        editor_push_command_level_size_change(before, after);
    }
}

void editor_set_elevation(s32 min, s32 max)
{
    Editor* editor = s_app->editor;
    
    V2i before = v2i(.x = editor->elevation_min, .y = editor->elevation_max);
    V2i after = v2i(.x = min, .y = max);
    
    editor->elevation_min = min;
    editor->elevation_max = max;
    
    if (v2i_distance(before, after) > 0)
    {
        editor->command_id++;
        editor_push_command_elevation_change(before, after);
    }
}

b32 editor_save_level()
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    Save_Level_Params params = 
    {
        .file_name = editor->file_name,
        .name = editor->name,
        .music_file_name = editor->music_file_name,
        .background_file_name = editor->background_file_name,
        .size = world->level.size,
        .tiles = world->level.tiles,
        .layer_names = editor->layer_names,
        .layers = editor->layers,
        .layer_count = editor->layer_count,
    };
    
    return save_level(params);
}

b32 editor_save_temp_level()
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    Save_Level_Params params = 
    {
        .file_name = EDITOR_LEVEL_NAME,
        .name = editor->name,
        .music_file_name = editor->music_file_name,
        .background_file_name = editor->background_file_name,
        .size = world->level.size,
        .tiles = world->level.tiles,
        .layer_names = editor->layer_names,
        .layers = editor->layers,
        .layer_count = editor->layer_count,
    };
    
    return save_level(params);
}

b32 editor_load_level(const char* file_name, b32 flush_undos)
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    
    Load_Level_Result result = load_level(file_name);
    
    if (result.success)
    {
        if (flush_undos)
        {
            editor_reset();
        }
        
        world_clear();
        
        string_set(editor->file_name, result.file_name);
        string_set(editor->name, result.name);
        string_set(editor->music_file_name, result.music_file_name);
        string_set(editor->background_file_name, result.background_file_name);
        
        world->level.size = result.size;
        
        CF_MEMCPY(world->level.tiles, result.tiles, sizeof(Tile) * result.tile_count);
        const char* tile_layer_name = cf_sintern(EDITOR_TILE_LAYER_NAME);
        for (s32 index = 0; index < result.layer_count; ++index)
        {
            if (tile_layer_name == cf_sintern(result.layer_names[index]))
            {
                CF_MEMCPY(world->level.tile_ids, result.layers[index], sizeof(world->level.tile_ids[0]) * result.tile_count);
                break;
            }
        }
        
        
        for (s32 index = 0; index < result.layer_count; ++index)
        {
            for (s32 layer_index = 0; layer_index < editor->layer_count; ++layer_index)
            {
                if (editor->layer_names[layer_index] == cf_sintern(result.layer_names[index]))
                {
                    CF_MEMCPY(editor->layers[layer_index], result.layers[index], sizeof(result.layers[index][0]) * result.tile_count);
                    break;
                }
            }
        }
    }
    
    return result.success;
}

void editor_new_level(const char* name)
{
    Editor* editor = s_app->editor;
    world_clear();
    editor_reset();
    string_set(editor->name, name);
}

void editor_cleanup_temp_files()
{
    mount_write_directory();
    const char* path = EDITOR_LEVEL_NAME;
    if (cf_fs_file_exists(path))
    {
        cf_fs_remove(path);
    }
}
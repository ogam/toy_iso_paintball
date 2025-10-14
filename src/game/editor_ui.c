#include "game/editor_ui.h"

void editor_ui_help_co(mco_coro* co)
{
    b32 done = false;
    
    while (!done)
    {
        CLAY(CLAY_ID("EditorHelp_Container"), {
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .border = {
                     .color = { 255, 255, 255, 255 },
                     .width = {
                         .bottom = 1,
                     },
                 },
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_GROW(0),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_CENTER,
                         .y = CLAY_ALIGN_Y_CENTER,
                     },
                     .childGap = 8,
                 },
             })
        {
            ui_do_text("Help");
            ui_do_text("PANNING: WASD / DIRECTIONAL KEYS or hold MIDDLE MOUSE BUTTON");
            ui_do_text("Place tiles, objects and units with LEFT MOUSE BUTTON");
            ui_do_text("Remove tiles, objects and units with RIGHT MOUSE BUTTON");
            ui_do_text("Hold SHIFT while placing and removing to make a change over an area");
            ui_do_text("Press B for brush mode, F for floodfill mode");
            ui_do_text("TAB and SHIFT+TAB will allow you to scroll through different tiles, objects and units");
            ui_do_text("Music and Background image can be set in the SETTINGS tab");
            ui_do_text("All saved levels will include an `.ipl` extension if one isn't provided");
            
            ui_push_corner_radius(2.0f);
            if (game_ui_do_button("Close"))
            {
                done = true;
            }
            ui_pop_corner_radius();
        }
        
        mco_yield(co);
    }
}

Editor_Tab_Type editor_ui_do_tabs()
{
    static Editor_Tab_Type current_tab = Editor_Tab_Type_Settings;
    CF_V2 image_size = cf_v2(64.0f, 64.0f);
    
    CLAY(CLAY_ID("EditorTabs_Container"), {
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .border = {
                 .color = { 255, 255, 255, 255 },
                 .width = {
                     .bottom = 1,
                 },
             },
             .layout = {
                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                 },
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 },
                 .childGap = 8,
             },
         })
    {
        {
            Sprite_Reference* reference = assets_get_resource_property_value("editor", "help");
            CF_ASSERT(reference);
            if (game_ui_do_image_button(reference->sprite, reference->animation, image_size))
            {
                ui_start_modal(editor_ui_help_co, NULL);
            }
        }
        {
            if (current_tab == Editor_Tab_Type_Settings)
            {
                ui_push_idle_color(ui_peek_hover_color());
            }
            
            Sprite_Reference* reference = assets_get_resource_property_value("editor", "settings");
            CF_ASSERT(reference);
            if (game_ui_do_image_button(reference->sprite, reference->animation, image_size))
            {
                current_tab = Editor_Tab_Type_Settings;
            }
            
            if (current_tab == Editor_Tab_Type_Settings)
            {
                ui_pop_idle_color();
            }
        }
        {
            if (current_tab == Editor_Tab_Type_Brushes)
            {
                ui_push_idle_color(ui_peek_hover_color());
            }
            
            Sprite_Reference* reference = assets_get_resource_property_value("editor", "brushes");
            CF_ASSERT(reference);
            if (game_ui_do_image_button(reference->sprite, reference->animation, image_size))
            {
                current_tab = Editor_Tab_Type_Brushes;
            }
            
            if (current_tab == Editor_Tab_Type_Brushes)
            {
                ui_pop_idle_color();
            }
        }
    }
    
    return current_tab;
}

void editor_ui_do_settings()
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    Game_UI* game_ui = s_app->game_ui;
    
    V2i min = LEVEL_SIZE_MIN;
    V2i max = LEVEL_SIZE_MAX;
    
    CF_V2 image_size = cf_v2(32, 32);
    Sprite_Reference* open_reference = assets_get_resource_property_value("editor", "file_open");
    Sprite_Reference* cancel_reference = assets_get_resource_property_value("editor", "cross");
    CF_ASSERT(open_reference);
    CF_ASSERT(cancel_reference);
    
    CLAY(CLAY_ID("EditorSettings_Container"), {
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                 },
                 .padding = CLAY_PADDING_ALL(8),
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_LEFT,
                     .y = CLAY_ALIGN_Y_CENTER,
                 },
                 .childGap = 8,
             },
         })
    {
        ui_do_text("Name");
        ui_do_input_text(editor->name, UI_Input_Text_Mode_Default);
        
        ui_do_text("File");
        ui_do_input_text(editor->file_name, UI_Input_Text_Mode_Default);
        
        ui_do_text("Level Size");
        CLAY(CLAY_ID("EditorSettingsLevelSize_Container"), {
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_CENTER,
                     },
                     .childGap = 8,
                 },
             })
        {
            if (ui_do_input_text(game_ui->input_text_x, UI_Input_Text_Mode_Sets_On_Return))
            {
                s32 x = world->level.size.x;
                sscanf(game_ui->input_text_x, "%d", &x);
                if (x >= min.x && x <= max.x)
                {
                    V2i next_size = v2i(.x = x, .y = world->level.size.y);
                    editor_set_level_size(next_size);
                }
                cf_string_fmt(game_ui->input_text_x, "%d", world->level.size.x);
            }
            if (!ui_is_item_selected())
            {
                cf_string_fmt(game_ui->input_text_x, "%d", world->level.size.x);
            }
            game_ui_set_item_tooltip("Size X\n%d - %d", min.x, max.x);
            
            if (ui_do_input_text(game_ui->input_text_y, UI_Input_Text_Mode_Sets_On_Return))
            {
                s32 y = world->level.size.y;
                sscanf(game_ui->input_text_y, "%d", &y);
                if (y >= min.y && y <= max.y)
                {
                    V2i next_size = v2i(.x = world->level.size.x, .y = y);
                    editor_set_level_size(next_size);
                }
                cf_string_fmt(game_ui->input_text_y, "%d", world->level.size.y);
            }
            if (!ui_is_item_selected())
            {
                cf_string_fmt(game_ui->input_text_y, "%d", world->level.size.y);
            }
            game_ui_set_item_tooltip("Size Y\n%d - %d", min.y, max.y);
        }
        
        //  @todo:  these should be modals
        //          a file explorer window should come up
        ui_do_text("Music");
        CLAY(CLAY_ID("EditorSettingsMusicSelect_Container"), {
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(1.0f),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_CENTER,
                     },
                     .childGap = 8,
                 },
             })
        {
            ui_push_font_size(18);
            ui_do_text(editor->music_file_name);
            ui_pop_font_size();
            if (game_ui_do_image_button(open_reference->sprite, open_reference->animation, image_size))
            {
                File_Dialog_Params* params = scratch_alloc(sizeof(File_Dialog_Params));
                fixed const char** filters = NULL;
                MAKE_SCRATCH_ARRAY(filters, 8);
                cf_array_push(filters, ".ogg");
                
                *params = (File_Dialog_Params){
                    .path = editor->music_file_name,
                    .filters = filters,
                    .filter_count = cf_array_count(filters),
                };
                
                ui_start_modal(game_ui_do_file_dialog_co, params);
            }
            if (game_ui_do_image_button(cancel_reference->sprite, cancel_reference->animation, image_size))
            {
                cf_string_clear(editor->music_file_name);
            }
        }
        
        ui_do_text("Background");
        CLAY(CLAY_ID("EditorSettingsBackgroundSelect_Container"), {
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(1.0f),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_CENTER,
                     },
                     .childGap = 8,
                 },
             })
        {
            ui_push_font_size(18);
            ui_do_text(editor->background_file_name);
            ui_pop_font_size();
            if (game_ui_do_image_button(open_reference->sprite, open_reference->animation, image_size))
            {
                File_Dialog_Params* params = scratch_alloc(sizeof(File_Dialog_Params));
                fixed const char** filters = NULL;
                MAKE_SCRATCH_ARRAY(filters, 8);
                cf_array_push(filters, ".ase");
                cf_array_push(filters, ".png");
                
                *params = (File_Dialog_Params){
                    .path = editor->background_file_name,
                    .filters = filters,
                    .filter_count = cf_array_count(filters),
                };
                
                ui_start_modal(game_ui_do_file_dialog_co, params);
            }
            if (game_ui_do_image_button(cancel_reference->sprite, cancel_reference->animation, image_size))
            {
                cf_string_clear(editor->background_file_name);
            }
        }
    }
}

void editor_ui_do_brushes()
{
    Assets* assets = s_app->assets;
    s32 resources_count = cf_hashtable_count(assets->resources);
    Asset_Resource* resources = cf_hashtable_items(assets->resources);
    
    static dyna b32* show_categories = NULL;
    
    // setup categories
    pq const char** categories = NULL;
    MAKE_SCRATCH_PQ(categories, resources_count);
    
    for (s32 index = 0; index < resources_count; ++index)
    {
        if (resources[index].editor.category)
        {
            pq_add_weight(categories, resources[index].editor.category, 1);
        }
    }
    
    // setup sets for each category
    fixed Asset_Resource*** resource_sets = NULL;
    MAKE_SCRATCH_ARRAY(resource_sets, pq_count(categories));
    
    for (s32 category_index = 0; category_index < pq_count(categories); ++category_index)
    {
        const char* category = categories[category_index];
        fixed Asset_Resource** resource_set = NULL;
        MAKE_SCRATCH_ARRAY(resource_set, resources_count);
        
        for (s32 index = 0; index < resources_count; ++index)
        {
            if (resources[index].editor.category == category)
            {
                cf_array_push(resource_set, resources + index);
            }
        }
        
        cf_array_push(resource_sets, resource_set);
    }
    
    // setup `sorted` list of resources for editor select prev/next
    fixed Asset_Resource** sorted_resources = NULL;
    MAKE_SCRATCH_ARRAY(sorted_resources, resources_count);
    
    for (s32 set_index = 0; set_index < cf_array_count(resource_sets); ++set_index)
    {
        fixed Asset_Resource** resource_set = resource_sets[set_index];
        
        for (s32 index = 0; index < cf_array_count(resource_set); ++index)
        {
            cf_array_push(sorted_resources, resource_set[index]);
        }
    }
    
    editor_brush_select_next(sorted_resources);
    
    {
        cf_array_fit(show_categories, pq_count(categories));
        while (cf_array_count(show_categories) < pq_count(categories))
        {
            cf_array_push(show_categories, true);
        }
    }
    
    CF_V2 button_size = cf_v2(64, 64);
    // do the actual ui for each category / set
    for (s32 category_index = 0; category_index < pq_count(categories); ++category_index)
    {
        const char* category = categories[category_index];
        fixed Asset_Resource** resource_set = resource_sets[category_index];
        
        Clay_ElementId category_id = CLAY_IDI_LOCAL("EditorCategory_Container", category_index);
        
        CLAY(category_id, {
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .border = {
                     .color = { 255, 255, 255, 255 },
                     .width = {
                         .top = 1,
                         .bottom = 1,
                     },
                 },
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_FIT(0),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_TOP,
                     },
                     .childGap = 8,
                 },
             })
        {
            if (game_ui_do_button_wide(category))
            {
                show_categories[category_index] = !show_categories[category_index];
            }
            
            if (!show_categories[category_index])
            {
                continue;
            }
            
            Clay_ElementData container_data = Clay_GetElementData(category_id);
            f32 container_width = container_data.boundingBox.width;
            s32 index = 0;
            ui_push_corner_radius(2.0f);
            
            while (index < cf_array_count(resource_set))
            {
                f32 x_offset = 0;
                // initial container width hasn't been setup yet so don't draw anything and wait for
                // next layout change
                if (f32_is_zero(container_width))
                {
                    break;
                }
                
                // do this per row
                CLAY(CLAY_IDI_LOCAL("EditorCategoryButton_Container", category_index + index), {
                         .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                         .layout = {
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = {
                                 .width = CLAY_SIZING_GROW(0),
                                 .height = CLAY_SIZING_FIT(button_size.y),
                             },
                             .childAlignment = {
                                 .x = CLAY_ALIGN_X_LEFT,
                                 .y = CLAY_ALIGN_Y_TOP,
                             },
                             .childGap = 8,
                         },
                         .clip = {
                             .horizontal = true,
                         },
                     })
                {
                    while (index < cf_array_count(resource_set))
                    {
                        Asset_Resource* resource = resource_set[index];
                        if (editor_brush_is_selected(resource))
                        {
                            ui_push_idle_color(ui_peek_hover_color());
                        }
                        
                        if (game_ui_do_image_button(resource->editor.reference.sprite, resource->editor.reference.animation, button_size))
                        {
                            editor_brush_select(resource);
                        }
                        if (resource->editor.display_name)
                        {
                            if (resource->editor.description)
                            {
                                ui_set_item_tooltip("%s\n%s", resource->editor.display_name, resource->editor.description);
                            }
                            else
                            {
                                ui_set_item_tooltip(resource->editor.display_name);
                            }
                        }
                        else
                        {
                            ui_set_item_tooltip(resource->name);
                        }
                        
                        if (editor_brush_is_selected(resource))
                        {
                            ui_pop_idle_color();
                        }
                        
                        ++index;
                        x_offset += button_size.x;
                        if (x_offset + button_size.x >= container_width)
                        {
                            break;
                        }
                    }
                }
            }
            ui_pop_corner_radius();
        }
    }
}

void editor_ui_do_footer()
{
    Input* input = s_app->input;
    Editor* editor = s_app->editor;
    Game_UI* game_ui = s_app->game_ui;
    
    CF_V2 image_size = cf_v2(64, 64);
    CF_V2 elevation_image_size = cf_v2(32, 32);
    s16 child_gap = 8;
    
    Sprite_Reference* floodfill_reference = assets_get_resource_property_value("editor", "floodfill");
    Sprite_Reference* brush_reference = assets_get_resource_property_value("editor", "brush");
    Sprite_Reference* decrease_reference = assets_get_resource_property_value("editor", "decrease");
    Sprite_Reference* increase_reference = assets_get_resource_property_value("editor", "increase");
    Sprite_Reference* save_reference = assets_get_resource_property_value("editor", "save");
    Sprite_Reference* play_reference = assets_get_resource_property_value("editor", "play");
    Sprite_Reference* auto_tiling_reference = assets_get_resource_property_value("editor", "auto_tiling");
    CF_ASSERT(floodfill_reference);
    CF_ASSERT(brush_reference);
    CF_ASSERT(increase_reference);
    CF_ASSERT(decrease_reference);
    CF_ASSERT(save_reference);
    CF_ASSERT(play_reference);
    CF_ASSERT(auto_tiling_reference);
    
    CLAY(CLAY_ID("EditorFooter_Container"), {
             .floating = {
                 .attachTo = CLAY_ATTACH_TO_PARENT,
                 .attachPoints = {
                     .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                 },
                 .offset = {
                     .y = -(image_size.y + child_gap) * 2,
                 },
             },
             .border = {
                 .color = { 255, 255, 255, 255 },
                 .width = CLAY_BORDER_ALL(1),
             },
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                 },
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_TOP,
                 },
                 .childGap = 8,
             },
         })
    {
        CLAY(CLAY_ID("EditorBrushSettings_Container"), {
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_BOTTOM,
                     },
                     .childGap = child_gap,
                 },
             })
        {
            if (editor_brush_mode_is_floodfill())
            {
                ui_push_idle_color(ui_peek_hover_color());
            }
            if (game_ui_do_image_button(floodfill_reference->sprite, floodfill_reference->animation, image_size))
            {
                editor_brush_mode_set_floodfill();
            }
            game_ui_set_item_tooltip("(F)ill");
            if (editor_brush_mode_is_floodfill())
            {
                ui_pop_idle_color();
            }
            
            if (editor_brush_mode_is_brush())
            {
                ui_push_idle_color(ui_peek_hover_color());
            }
            if (game_ui_do_image_button(brush_reference->sprite, brush_reference->animation, image_size))
            {
                editor_brush_mode_set_brush();
            }
            game_ui_set_item_tooltip("(B)rush");
            if (editor_brush_mode_is_brush())
            {
                ui_pop_idle_color();
            }
            
            CLAY(CLAY_ID("EditorBrushElevationSettings_Container"), {
                     .layout = {
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = {
                             .width = CLAY_SIZING_GROW(0),
                         },
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_LEFT,
                             .y = CLAY_ALIGN_Y_TOP,
                         },
                         .childGap = child_gap,
                     },
                 })
            {
                CLAY(CLAY_ID("EditorBrushMaxElevationSettings_Container"), {
                         .layout = {
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = {
                                 .width = CLAY_SIZING_GROW(0),
                             },
                             .childAlignment = {
                                 .x = CLAY_ALIGN_X_LEFT,
                                 .y = CLAY_ALIGN_Y_CENTER,
                             },
                             .childGap = child_gap,
                         },
                     })
                {
                    if (ui_do_input_text(game_ui->input_text_elevation_max, UI_Input_Text_Mode_Sets_On_Return))
                    {
                        s32 elevation_max = editor->elevation_max;
                        sscanf(game_ui->input_text_elevation_max, "%d", &elevation_max);
                        if (elevation_max >= 0 && elevation_max <= MAX_ELEVATION)
                        {
                            editor_set_elevation(editor->elevation_min, elevation_max);
                        }
                        cf_string_fmt(game_ui->input_text_elevation_max, "%d", editor->elevation_max);
                    }
                    if (!ui_is_item_selected())
                    {
                        cf_string_fmt(game_ui->input_text_elevation_max, "%d", editor->elevation_max);
                    }
                    game_ui_set_item_tooltip("Elevation Max\n%d - %d", 0, MAX_ELEVATION);
                    if (game_ui_do_image_button(decrease_reference->sprite, decrease_reference->animation, elevation_image_size))
                    {
                        editor_set_elevation(editor->elevation_min, cf_max(editor->elevation_max - 1, 0));
                    }
                    if (game_ui_do_image_button(increase_reference->sprite, increase_reference->animation, elevation_image_size))
                    {
                        editor_set_elevation(editor->elevation_min, cf_min(editor->elevation_max + 1, MAX_ELEVATION));
                    }
                }
                
                CLAY(CLAY_ID("EditorBrushMinElevationSettings_Container"), {
                         .layout = {
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = {
                                 .width = CLAY_SIZING_GROW(0),
                             },
                             .childAlignment = {
                                 .x = CLAY_ALIGN_X_LEFT,
                                 .y = CLAY_ALIGN_Y_CENTER,
                             },
                             .childGap = child_gap,
                         },
                     })
                {
                    if (ui_do_input_text(game_ui->input_text_elevation_min, UI_Input_Text_Mode_Sets_On_Return))
                    {
                        s32 elevation_min = editor->elevation_min;
                        sscanf(game_ui->input_text_elevation_min, "%d", &elevation_min);
                        if (elevation_min >= 0 && elevation_min <= MAX_ELEVATION)
                        {
                            editor_set_elevation(elevation_min, editor->elevation_max);
                        }
                        cf_string_fmt(game_ui->input_text_elevation_min, "%d", editor->elevation_min);
                    }
                    if (!ui_is_item_selected())
                    {
                        cf_string_fmt(game_ui->input_text_elevation_min, "%d", editor->elevation_min);
                    }
                    game_ui_set_item_tooltip("Elevation Min\n%d - %d", 0, MAX_ELEVATION);
                    if (game_ui_do_image_button(decrease_reference->sprite, decrease_reference->animation, elevation_image_size))
                    {
                        editor_set_elevation(cf_max(editor->elevation_min - 1, 0), editor->elevation_max);
                    }
                    if (game_ui_do_image_button(increase_reference->sprite, increase_reference->animation, elevation_image_size))
                    {
                        editor_set_elevation(cf_min(editor->elevation_min + 1, MAX_ELEVATION), editor->elevation_max);
                    }
                }
            }
        }
        
        CLAY(CLAY_ID("EditorSaveLoadButtons_Container"), {
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_BOTTOM,
                     },
                     .childGap = 8,
                 },
             })
        {
            if (game_ui_do_image_button(save_reference->sprite, save_reference->animation, image_size))
            {
                editor_save_level();
            }
            game_ui_set_item_tooltip("Save");
            
            if (game_ui_do_image_button(play_reference->sprite, play_reference->animation, image_size))
            {
                if (editor_save_temp_level())
                {
                    make_event_load_level(EDITOR_LEVEL_NAME);
                    game_ui_set_state(Game_UI_State_Main_Menu);
                    game_ui_push_state(Game_UI_State_Play);
                    editor_set_state(Editor_State_Edit_Play);
                }
            }
            
            CLAY(CLAY_ID("EditorBrushInfo_Container"), {
                     .layout = {
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .sizing = {
                             .width = CLAY_SIZING_GROW(0),
                         },
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_LEFT,
                             .y = CLAY_ALIGN_Y_TOP,
                         },
                         .childGap = 8,
                     },
                 })
            {
                if (editor_brush_mode_is_auto_tiling())
                {
                    ui_push_idle_color(ui_peek_hover_color());
                }
                if (game_ui_do_image_button(auto_tiling_reference->sprite, auto_tiling_reference->animation, cf_v2(32, 32)))
                {
                    editor_brush_mode_set_auto_tiling(!editor_brush_mode_is_auto_tiling());
                }
                game_ui_set_item_tooltip("Auto (T)iling");
                if (editor_brush_mode_is_auto_tiling())
                {
                    ui_pop_idle_color();
                }
                
                game_ui_set_item_tooltip("Play");
                {
                    V2i tile = v2i(.x = -1, .y = -1);
                    s32 elevation = 0;
                    
                    Tile* tile_ptr = get_tile(input->tile_select);
                    if (tile_ptr)
                    {
                        tile = input->tile_select;
                        elevation = tile_ptr->elevation;
                    }
                    ui_do_text("%d, %d, %d", tile.x, tile.y, elevation);
                }
            }
        }
    }
}

void editor_ui_do_main()
{
    CLAY(CLAY_ID("Editor_OuterContainer"), {
             .backgroundColor = { 128, 128, 128, 255 },
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_PERCENT(EDITOR_X_PERCENT),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_LEFT,
                     .y = CLAY_ALIGN_Y_TOP,
                 }
             },
         })
    {
        Clay_OnHover(clay_on_hover, 0);
        
        Editor_Tab_Type tab_type = editor_ui_do_tabs();
        
        switch (tab_type)
        {
            case Editor_Tab_Type_Brushes:
            {
                editor_ui_do_brushes();
            }
            break;
            case Editor_Tab_Type_Settings:
            default:
            {
                editor_ui_do_settings();
            }
            break;
        }
        
        editor_ui_do_footer();
    }
}
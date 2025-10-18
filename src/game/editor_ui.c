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
            ui_push_font_size(32);
            ui_do_text("Help");
            ui_pop_font_size();
            
            ui_push_font_size(32);
            ui_do_text("Settings");
            ui_pop_font_size();
            ui_do_text("Music and Background image can be set in the SETTINGS tab");
            
            ui_push_font_size(32);
            ui_do_text("Brush Mode");
            ui_pop_font_size();
            
            ui_do_text("PANNING: WASD / DIRECTIONAL KEYS or hold MIDDLE MOUSE BUTTON");
            ui_do_text("Place tiles, objects and units with LEFT MOUSE BUTTON");
            ui_do_text("Remove tiles, objects and units with RIGHT MOUSE BUTTON");
            ui_do_text("Hold SHIFT while placing and removing to make a change over an area");
            ui_do_text("Press B for brush mode, F for floodfill mode");
            ui_do_text("TAB and SHIFT+TAB will allow you to scroll through different tiles, objects and units");
            
            ui_push_font_size(32);
            ui_do_text("Switch Mode");
            ui_pop_font_size();
            
            ui_do_text("Place trigger sources with LEFT MOUSE BUTTON");
            ui_do_text("Place triggered region with SHIFT + LEFT MOUSE BUTTON");
            ui_do_text("Place the top of Stairs with T");
            
            ui_push_font_size(32);
            ui_do_text("Other");
            ui_pop_font_size();
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
    static Editor_Tab_Type current_tab = Editor_Tab_Type_Brushes;
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
        {
            if (current_tab == Editor_Tab_Type_Switch_Link)
            {
                ui_push_idle_color(ui_peek_hover_color());
            }
            
            Sprite_Reference* reference = assets_get_resource_property_value("editor", "switch_link");
            CF_ASSERT(reference);
            if (game_ui_do_image_button(reference->sprite, reference->animation, image_size))
            {
                current_tab = Editor_Tab_Type_Switch_Link;
            }
            
            if (current_tab == Editor_Tab_Type_Switch_Link)
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
            if (ui_do_input_s32(&world->level.size.x, min.x, max.x))
            {
                editor_set_level_size(world->level.size);
            }
            game_ui_set_item_tooltip("Size X\n%d - %d", min.x, max.x);
            
            if (ui_do_input_s32(&world->level.size.y, min.y, max.y))
            {
                editor_set_level_size(world->level.size);
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

void editor_ui_do_switch_links()
{
    Editor* editor = s_app->editor;
    UI_Input* ui_input = &s_app->ui->input;
    V2i level_size = s_app->world->level.size;
    
    dyna Switch_Link* switch_links = s_app->world->level.switch_links;
    
    fixed char* buf = make_scratch_string(256);
    s32 next_select = -1;
    static s32 drop_down_index = -1;
    
    CLAY(CLAY_ID("EditorSwitchLinks_OuterContainer"), {
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
                     // hard code for now, footer has 2 * 64 and tabs has 64, footer has 2 child gaps of 8 each
                     .height = CLAY_SIZING_FIXED(s_app->h - 64.0f * 4 + 16.0f),
                 },
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_LEFT,
                     .y = CLAY_ALIGN_Y_TOP,
                 },
                 .childGap = 8,
             },
         })
    {
        if (ui_do_button("Add"))
        {
            editor_add_switch_link((Switch_Link){ 0 });
        }
        
        CLAY(CLAY_ID("EditorSwitchLinks_Container"), {
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
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_TOP,
                     },
                     .childGap = 8,
                 },
                 .clip = {
                     .vertical = true,
                     .horizontal = true,
                     .childOffset = Clay_GetScrollOffset(),
                 }
             })
        {
            Clay_Color selected_color = {128, 128, 0, 255};
            Clay_Color idle_color = { 0, 0, 0, 0 };
            
            for (s32 index = 0; index < cf_array_count(switch_links);)
            {
                Switch_Link* link = switch_links + index;
                Switch_Link copy_link = *link;
                b32 is_selected = link->state & Switch_Link_State_Bit_Editor_Select;
                b32 skip = false;
                
                cf_string_fmt(buf, "%d", index);
                if (game_ui_do_button_wide(buf))
                {
                    link->state ^= Switch_Link_State_Bit_Editor_Visible;
                }
                
                if ((link->state & Switch_Link_State_Bit_Editor_Visible) == 0)
                {
                    ++index;
                    continue;
                }
                
                CLAY(CLAY_IDI_LOCAL("EditorSwitchLink_Container", index), {
                         .backgroundColor = is_selected ? selected_color : idle_color,
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
                    if (Clay_Hovered())
                    {
                        if (ui_input->mouse_press)
                        {
                            next_select = index;
                        }
                    }
                    
                    CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkMode_Container", index), {
                             .layout = {
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                        
                        cf_push_font_size(ui_peek_font_size());
                        f32 width = cf_text_width("Mode", -1);
                        cf_pop_font_size();
                        ui_do_text("Mode");
                        
                        //  @todo:  warp this up into a function
                        //          not sure how it should look or if a drop down should be a macro due
                        //          to how Clay works?
                        //          other option is to have the following. feels pretty restrictive still..
                        //          ui_do_drop_down_bits("name", ["fields"], [&values], [bits])
                        CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkModeDropDown_Container", index), {
                                 .floating = {
                                     .attachTo = CLAY_ATTACH_TO_PARENT,
                                     .attachPoints = {
                                         .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                                     },
                                     .offset = {
                                         .x = width + 8,
                                     },
                                     .clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
                                 },
                                 .backgroundColor = { 0, 0, 0, 255 },
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
                            if (drop_down_index == index)
                            {
                                if (!Clay_Hovered())
                                {
                                    drop_down_index = -1;
                                }
                            }
                            
                            //  @todo:  enum to string macro
                            if (link->state & Switch_Link_State_Bit_Filler)
                            {
                                cf_string_fmt(buf, "%s", "Filler");
                            }
                            else
                            {
                                cf_string_fmt(buf, "%s", "Mover");
                            }
                            if (link->state & Switch_Link_State_Bit_Cascade)
                            {
                                cf_string_fmt_append(buf, "|%s", "Cascade");
                            }
                            if (link->state & Switch_Link_State_Bit_Stairs)
                            {
                                cf_string_fmt_append(buf, "|%s", "Stairs");
                            }
                            if (link->state & Switch_Link_State_Bit_Mod)
                            {
                                cf_string_fmt_append(buf, "|%s", "Mod");
                            }
                            
                            if (game_ui_do_button_wide(buf))
                            {
                                if (drop_down_index == index)
                                {
                                    drop_down_index = -1;
                                }
                                else
                                {
                                    drop_down_index = index;
                                }
                            }
                            game_ui_set_item_tooltip(buf);
                            
                            if (drop_down_index == index)
                            {
                                //  @todo:  wrap all these into a function
                                //          ui_do_drop_down_checkbox_bit("name", &value, bit)
                                CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkModeDropDownItem_Filler_Container", index), {
                                         .layout = {
                                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                                    if (game_ui_do_button("Filler"))
                                    {
                                        link->state ^= Switch_Link_State_Bit_Filler;
                                    }
                                    game_ui_set_item_tooltip("Filler is persistent tile change\nturn off for Mover mode that is temporary");
                                    ui_do_checkbox_bit(&link->state, Switch_Link_State_Bit_Filler);
                                }
                                
                                CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkModeDropDownItem_Cascade_Container", index), {
                                         .layout = {
                                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                                    if (game_ui_do_button("Cascade"))
                                    {
                                        link->state ^= Switch_Link_State_Bit_Cascade;
                                    }
                                    game_ui_set_item_tooltip("Cascade is persistent tile change\nturn off for Mover mode that is temporary");
                                    ui_do_checkbox_bit(&link->state, Switch_Link_State_Bit_Cascade);
                                }
                                
                                CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkModeDropDownItem_Stairs_Container", index), {
                                         .layout = {
                                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                                    if (game_ui_do_button("Stairs"))
                                    {
                                        link->state ^= Switch_Link_State_Bit_Stairs;
                                    }
                                    game_ui_set_item_tooltip("Stairs, elevation offsets depends on distance\nAdjust Steps");
                                    ui_do_checkbox_bit(&link->state, Switch_Link_State_Bit_Stairs);
                                }
                                
                                CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkModeDropDownItem_Mod_Container", index), {
                                         .layout = {
                                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                                    if (game_ui_do_button("Mod"))
                                    {
                                        link->state ^= Switch_Link_State_Bit_Mod;
                                    }
                                    game_ui_set_item_tooltip("Mod by x and y from a the region center tile");
                                    ui_do_checkbox_bit(&link->state, Switch_Link_State_Bit_Mod);
                                }
                            }
                        }
                    }
                    
                    CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkSwitchPosition_Container", index), {
                             .layout = {
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                        ui_do_text("Switch");
                        ui_do_input_s32(&link->source.x, 0, level_size.x - 1);
                        game_ui_set_item_tooltip("X");
                        ui_do_input_s32(&link->source.y, 0, level_size.x - 1);
                        game_ui_set_item_tooltip("Y");
                    }
                    
                    CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkRegion_Container", index), {
                             .layout = {
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                        ui_do_text("Region");
                        ui_do_input_s32(&link->region.min.x, 0, level_size.x - 1);
                        game_ui_set_item_tooltip("Min X");
                        ui_do_input_s32(&link->region.min.y, 0, level_size.y - 1);
                        game_ui_set_item_tooltip("Min Y");
                        ui_do_input_s32(&link->region.max.x, 0, level_size.x - 1);
                        game_ui_set_item_tooltip("Max X");
                        ui_do_input_s32(&link->region.max.y, 0, level_size.y - 1);
                        game_ui_set_item_tooltip("Max X");
                    }
                    
                    CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkElevation_Container", index), {
                             .layout = {
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                        ui_do_text("Elevation");
                        ui_do_input_s32((s32*)&link->end_elevation, 0, MAX_ELEVATION);
                    }
                    
                    CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkSpeed_Container", index), {
                             .layout = {
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                        ui_do_text("Speed");
                        ui_do_input_f32(&link->speed, SWITCH_LINK_MIN_SPEED, SWITCH_LINK_MAX_SPEED);
                    }
                    
                    if (link->state & Switch_Link_State_Bit_Cascade)
                    {
                        CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkCascade_Container", index), {
                                 .layout = {
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                            ui_do_text("Delay");
                            ui_do_input_f32(&link->cascade_delay, 0.0f, 2.0f);
                        }
                    }
                    
                    if (link->state & Switch_Link_State_Bit_Stairs)
                    {
                        ui_do_text("Stairs");
                        CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkStairsTopPosition_Container", index), {
                                 .layout = {
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                            ui_do_text("(T)op");
                            ui_do_input_s32(&link->stairs_top.x, 0, level_size.x - 1);
                            ui_do_input_s32(&link->stairs_top.y, 0, level_size.y - 1);
                        }
                        
                        CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkStairsStepRate_Container", index), {
                                 .layout = {
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                            ui_do_text("Steps");
                            ui_do_input_s32(&link->stairs_step_rate.x, 0, 20);
                            ui_do_input_s32(&link->stairs_step_rate.y, 0, 20);
                        }
                    }
                    
                    if (link->state & Switch_Link_State_Bit_Mod)
                    {
                        CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkMod_Container", index), {
                                 .layout = {
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                            ui_do_text("Mod");
                            ui_do_input_s32(&link->mod.x, 0, 20);
                            ui_do_input_s32(&link->mod.y, 0, 20);
                        }
                    }
                    
                    if (link->state & Switch_Link_State_Bit_Invertible)
                    {
                        CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkInvert_Container", index), {
                                 .layout = {
                                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                            ui_do_text("Invert");
                            ui_do_checkbox_bit(&link->state, Switch_Link_State_Bit_Invert);
                        }
                    }
                    
                    CLAY(CLAY_IDI_LOCAL("EditorSwitchLinkButtons_Container", index), {
                             .layout = {
                                 .layoutDirection = CLAY_LEFT_TO_RIGHT,
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
                        if (ui_do_button("Remove"))
                        {
                            editor_remove_switch_link(index);
                            skip = true;;
                            if (drop_down_index == index)
                            {
                                drop_down_index = -1;
                            }
                        }
                        
                        if (ui_do_button("Clone"))
                        {
                            editor_clone_switch_link(index);
                        }
                    }
                    
                    if (skip)
                    {
                        continue;
                    }
                    
                    if (CF_MEMCMP(link, &copy_link, sizeof(Switch_Link)) != 0)
                    {
                        editor_update_switch_link(copy_link, *link, index);
                    }
                    
                    ++index;
                }
            }
        }
    }
    
    // new selection made
    if (next_select != -1)
    {
        for (s32 index = 0; index < cf_array_count(switch_links); ++index)
        {
            switch_links[index].state &= ~Switch_Link_State_Bit_Editor_Select;
            if (index == next_select)
            {
                switch_links[index].state |= Switch_Link_State_Bit_Editor_Select;
            }
        }
    }
    
    // deselect anything that's not visible
    for (s32 index = 0; index < cf_array_count(switch_links); ++index)
    {
        if ((switch_links[index].state & Switch_Link_State_Bit_Editor_Visible) == 0)
        {
            switch_links[index].state &= ~Switch_Link_State_Bit_Editor_Select;
            if (drop_down_index == index)
            {
                drop_down_index = -1;
            }
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
                 .childGap = child_gap,
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
                    if (ui_do_input_s32(&editor->elevation_max, 0, MAX_ELEVATION))
                    {
                        editor_set_elevation(editor->elevation_min, editor->elevation_max);
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
                    if (ui_do_input_s32(&editor->elevation_min, 0, MAX_ELEVATION))
                    {
                        editor_set_elevation(editor->elevation_min, editor->elevation_max);
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
                     .childGap = child_gap,
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
            game_ui_set_item_tooltip("Play");
            
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
                         .childGap = child_gap,
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
                // tile coordinates
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
                editor_brush_mode_set_switch_link(false);
                editor_ui_do_brushes();
            }
            break;
            case Editor_Tab_Type_Switch_Link:
            {
                editor_brush_mode_set_switch_link(true);
                editor_ui_do_switch_links();
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
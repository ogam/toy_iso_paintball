#include "game/game_ui.h"

void game_ui_handle_state(Game_UI_State state, Game_UI_State next_state)
{
    World* world = s_app->world;
    Game_UI* game_ui = s_app->game_ui;
    
    if (state == Game_UI_State_Play && next_state != Game_UI_State_Play)
    {
        world_set_state(World_State_Pause);
        audio_pause(Audio_Source_Type_SFX);
    }
    
    if (next_state == Game_UI_State_Main_Menu)
    {
        if (world->state != World_State_Demo)
        {
            game_ui->demo_reload_delay = 0.0f;
            cf_string_clear(game_ui->level_name);
        }
    }
}

void game_ui_set_state(Game_UI_State state)
{
    game_ui_handle_state(game_ui_peek_state(), state);
    cf_array_clear(s_app->game_ui->states);
    game_ui_push_state(state);
}

void game_ui_push_state(Game_UI_State state)
{
    game_ui_handle_state(game_ui_peek_state(), state);
    cf_array_push(s_app->game_ui->states, state);
}

Game_UI_State game_ui_peek_state()
{
    return cf_array_last(s_app->game_ui->states);
}

Game_UI_State game_ui_pop_state()
{
    Game_UI_State state = game_ui_peek_state();
    if (cf_array_count(s_app->game_ui->states) > 1)
    {
        cf_array_pop(s_app->game_ui->states);
        game_ui_handle_state(state, game_ui_peek_state());
    }
    return state;
}

void game_ui_control_add(ecs_id_t id)
{
    Game_UI* game_ui = s_app->game_ui;
    s32 index = 0;
    for (;index < cf_array_count(game_ui->control_ids); ++index)
    {
        if (game_ui->control_ids[index] == id)
        {
            break;
        }
    }
    
    if (index >= cf_array_count(game_ui->control_ids))
    {
        cf_array_push(game_ui->control_ids, id);
    }
}

void game_ui_control_remove(ecs_id_t id)
{
    Game_UI* game_ui = s_app->game_ui;
    s32 index = 0;
    for (;index < cf_array_count(game_ui->control_ids); ++index)
    {
        if (game_ui->control_ids[index] == id)
        {
            cf_array_del(game_ui->control_ids, index);
            break;
        }
    }
}

void game_ui_control_clear(ecs_id_t id)
{
    cf_array_clear(s_app->game_ui->control_ids);
}

void clay_on_hover(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
{
    UNUSED(pointerData);
    UNUSED(userData);
    s_app->game_ui->hover_id = elementId;
}

b32 game_ui_do_button(const char* text)
{
    b32 clicked = ui_do_button(text);
    if (clicked)
    {
        audio_play_random(s_app->game_ui->button_sounds, Audio_Source_Type_UI);
    }
    
    return clicked;
}

b32 game_ui_do_button_wide(const char* text)
{
    b32 clicked = ui_do_button_wide(text);
    if (clicked)
    {
        audio_play_random(s_app->game_ui->button_sounds, Audio_Source_Type_UI);
    }
    
    return clicked;
}

b32 game_ui_do_image_button(const char* name, const char* animation, CF_V2 size)
{
    b32 clicked = ui_do_image_button(name, animation, size);
    if (clicked)
    {
        audio_play_random(s_app->game_ui->button_sounds, Audio_Source_Type_UI);
    }
    
    return clicked;
}

b32 game_ui_do_sprite_button(CF_Sprite* sprite, CF_V2 size)
{
    b32 clicked = ui_do_sprite_button(sprite, size);
    if (clicked)
    {
        audio_play_random(s_app->game_ui->button_sounds, Audio_Source_Type_UI);
    }
    
    return clicked;
}

b32 game_ui_is_hovering_over_any_layouts()
{
    // comparing against prev since every frame this is cleared out
    return s_app->game_ui->prev_hover_id.id != 0 || s_app->ui->hover_id.id != 0 || s_app->ui->next_hover_id.id != 0;
}

void game_ui_set_item_tooltip(const char* fmt, ...)
{
    ui_push_corner_radius(2.0f);
    va_list args;
    va_start(args, fmt);
    ui_set_item_vtooltip(fmt, args);
    va_end(args);
    ui_pop_corner_radius();
}

b32 game_ui_do_file_dialog_co_ex(mco_coro* co)
{
    typedef struct File_Info
    {
        const char* name;
        CF_Stat stat;
    } File_Info;
    
    File_Dialog_Params* params = mco_get_user_data(co);
    fixed char* out_path = params->path;
    dyna char* temp_out_path = NULL;
    cf_string_fit(temp_out_path, 1024);
    string_set(temp_out_path, out_path);
    dyna const char** filters = NULL;
    cf_array_fit(filters, params->filter_count);
    
    for (s32 index = 0; index < params->filter_count; ++index)
    {
        cf_array_push(filters, params->filters[index]);
    }
    
    s32 directory_depth = 0;
    char* path = NULL;
    cf_string_fit(path, 1024);
    cf_string_append(path, "");
    
    b32 accept = false;
    
    while (true)
    {
        b32 is_done = false;
        ui_push_corner_radius(2.0f);
        Clay_ElementId modal_container_id = CLAY_ID("EditorFileDialogModal_OuterContainer");
        
        CLAY(modal_container_id, {
                 .backgroundColor = { 0, 0, 0, 255 },
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .padding = CLAY_PADDING_ALL(8),
                     .childGap = 8,
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_CENTER,
                         .y = CLAY_ALIGN_Y_TOP,
                     },
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(0.5f),
                         .height = CLAY_SIZING_PERCENT(0.5f),
                     },
                 },
             })
        {
            ui_push_font_size(18);
            const char** file_list = cf_fs_enumerate_directory(path);
            const char** file_it = file_list;
            fixed File_Info* files = NULL;
            MAKE_SCRATCH_ARRAY(files, 32);
            
            if (directory_depth > 0)
            {
                File_Info info = {
                    .name = "..",
                    .stat = {
                        .type = CF_FILE_TYPE_DIRECTORY,
                    },
                };
                cf_array_push(files, info);
            }
            
            while (*file_it)
            {
                b32 is_directory = false;
                b32 is_correct_extension = false;
                CF_Stat file_stat;
                
                {
                    CF_Result result = cf_fs_stat(*file_it, &file_stat);
                    if (result.code == CF_RESULT_SUCCESS)
                    {
                        is_directory = file_stat.type == CF_FILE_TYPE_DIRECTORY;
                    }
                }
                if (!is_directory)
                {
                    for (s32 filter_index = 0; filter_index < cf_array_count(filters); ++filter_index)
                    {
                        if (cf_path_ext_equ(*file_it, filters[filter_index]))
                        {
                            is_correct_extension = true;
                            break;
                        }
                    }
                }
                
                if (is_directory || is_correct_extension)
                {
                    if (cf_array_count(files) >= cf_array_capacity(files))
                    {
                        GROW_SCRATCH_ARRAY(files);
                    }
                    
                    File_Info info = {
                        .name = *file_it,
                        .stat = file_stat,
                    };
                    cf_array_push(files, info);
                }
                
                ++file_it;
            }
            
            CLAY(CLAY_ID("EditorFileDialogListVerticalModal_InnerContainer"), {
                     .backgroundColor = { 0, 0, 0, 255 },
                     .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                     .layout = {
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         .padding = CLAY_PADDING_ALL(8),
                         .childGap = 8,
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_LEFT,
                             .y = CLAY_ALIGN_Y_TOP,
                         },
                         .sizing = {
                             .width = CLAY_SIZING_PERCENT(1.0f),
                             .height = CLAY_SIZING_PERCENT(0.8f),
                         },
                     },
                     .clip = { .horizontal = true,  .vertical = true, .childOffset = Clay_GetScrollOffset() },
                 })
            {
                for (s32 index = 0; index < cf_array_count(files); ++index)
                {
                    File_Info* info = files + index;
                    if (ui_do_button(info->name))
                    {
                        if (info->stat.type == CF_FILE_TYPE_DIRECTORY)
                        {
                            if (cf_string_equ(info->name, ".."))
                            {
                                cf_path_pop(path);
                                directory_depth = cf_max(directory_depth - 1, 0);
                                
                                if (directory_depth == 0)
                                {
                                    cf_string_clear(path);
                                }
                            }
                            else
                            {
                                if (cf_string_len(path))
                                {
                                    cf_string_fmt_append(path, "/%s", info->name);
                                }
                                else
                                {
                                    cf_string_fmt_append(path, "%s", info->name);
                                }
                                directory_depth++;
                            }
                        }
                        else 
                        {
                            cf_string_fmt(temp_out_path, "%s/%s", path, info->name);
                        }
                    }
                }
            }
            
            cf_fs_free_enumerated_directory(file_list);
            ui_pop_font_size();
            
            ui_do_input_text(temp_out_path, UI_Input_Text_Mode_Default);
            CLAY(CLAY_ID("EditorFileDialogControlsModal_InnerContainer"), {
                     .floating = {
                         .attachTo = CLAY_ATTACH_TO_PARENT,
                         .attachPoints = {
                             .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                         },
                         .offset = {
                             .y = -(ui_peek_font_size() * 2),
                         },
                     },
                     .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                     .layout = {
                         .layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .padding = CLAY_PADDING_ALL(8),
                         .childGap = 16,
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_LEFT,
                             .y = CLAY_ALIGN_Y_TOP,
                         },
                         .sizing = {
                             .height = CLAY_SIZING_FIT(0),
                         },
                     },
                 })
            {
                
                if (ui_do_button("Cancel"))
                {
                    is_done = true;
                }
                
                if (ui_do_button("Accept"))
                {
                    if (cf_fs_file_exists(temp_out_path))
                    {
                        is_done = true;
                        accept = true;
                    }
                }
            }
        }
        
        ui_pop_corner_radius();
        
        if (is_done)
        {
            break;
        }
        
        mco_yield(co);
    }
    
    if (accept)
    {
        string_set(out_path, temp_out_path);
    }
    
    cf_string_free(temp_out_path);
    cf_string_free(path);
    cf_array_free(filters);
    
    return accept;
}

void game_ui_do_file_dialog_co(mco_coro* co)
{
    game_ui_do_file_dialog_co_ex(co);
}

void game_ui_do_credits_co(mco_coro* co)
{
    dyna Credits_Command* commands = mco_get_user_data(co);
    if (!commands)
    {
        return;
    }
    
    s32 child_gap = 16;
    f32 speed = 300.0f;
    b32 is_done = false;
    f32 y_offset = 0;
    
    dyna CF_Sprite* sprites = NULL;
    s32 text_size_change_count = 0;
    
    for (s32 index = 0; index < cf_array_count(commands); ++index)
    {
        Credits_Command* command = commands + index;
        
        if (command->type == Credits_Command_Type_Font_Size)
        {
            cf_push_font_size(command->font_size);
            text_size_change_count++;
        }
        else if (command->type == Credits_Command_Type_Text)
        {
            y_offset += cf_text_height(command->text, -1) + child_gap;
        }
        else if (command->type == Credits_Command_Type_Sprite)
        {
            CF_Sprite* asset_sprite = assets_get_sprite(command->sprite_name);
            if (asset_sprite)
            {
                CF_Sprite sprite = *asset_sprite;
                const char* animation_name = command->animation_name;
                if (animation_name)
                {
                    if (cf_hashtable_has(sprite.animations, cf_sintern(animation_name)))
                    {
                        cf_sprite_play(&sprite, animation_name);
                    }
                }
                cf_array_push(sprites, sprite);
                command->sprite = &cf_array_last(sprites);
                y_offset += sprite.h * sprite.scale.y + child_gap;
            }
        }
    }
    
    while (text_size_change_count--)
    {
        cf_pop_font_size();
    }
    
    f32 reset_y_offset = y_offset * 1.25f;
    y_offset = (f32)-s_app->h;
    
    while (!is_done)
    {
        for (s32 index = 0; index < cf_array_count(sprites); ++index)
        {
            cf_sprite_update(sprites + index);
        }
        
        s32 w = s_app->w;
        s32 h = s_app->h;
        
        y_offset += CF_DELTA_TIME * speed;
        if (y_offset > reset_y_offset)
        {
            y_offset = (f32)-h;
        }
        text_size_change_count = 0;
        //  @note:  due to how clay is removing images that have been clipped
        //          instead of drawing it within the scissored region we're going
        //          to manually do the credits instead on a separate canvas
        //          that way we don't have to deal with this
        //          also noticed tiny jitters during the auto scrolling? unsure
        //          if this related to any hitches or if it's the auto layout
        //          that's being applied to a bunch of UI elements that may
        //          or may not be outside of the scissor region
        CF_Rect scissor = 
        {
            .x = 0,
            .y = 0,
            .w = w,
            .h = (s32)(h * 0.8f),
        };
        cf_draw_push_scissor(scissor);
        
        CF_V2 position = cf_v2(0, y_offset);
        for (s32 index = 0; index < cf_array_count(commands); ++index)
        {
            Credits_Command* command = commands + index;
            switch (command->type)
            {
                case Credits_Command_Type_Font_Size:
                {
                    cf_push_font_size(command->font_size);
                    text_size_change_count++;
                }
                break;
                case Credits_Command_Type_Text:
                {
                    CF_V2 text_size = cf_text_size(command->text, -1);
                    CF_V2 text_position = position;
                    text_position.x -= text_size.x * 0.5f;
                    cf_draw_text(command->text, text_position, -1);
                    position.y -= text_size.y + child_gap;
                }
                break;
                case Credits_Command_Type_Sprite:
                {
                    command->sprite->transform.p = position;
                    cf_draw_sprite(command->sprite);
                    position.y -= command->sprite->w * command->sprite->scale.y * 0.5f + child_gap;
                }
                break;
                default:
                break;
            }
        }
        
        cf_draw_pop_scissor();
        cf_clear_color(0, 0, 0, 0);
        cf_render_to(s_app->credits_canvas, true);
        cf_clear_color(0, 0, 0, 1);
        s_app->credits_canvas_dirty = true;
        
        while (text_size_change_count--)
        {
            cf_pop_font_size();
        }
        
        CLAY(CLAY_ID("CreditsButton_Container"), {
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_GROW(0),
                     },
                     .childGap = child_gap,
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_CENTER,
                         .y = CLAY_ALIGN_Y_BOTTOM,
                     }
                 },
                 .clip = {
                     .vertical = true,
                     .childOffset = { .y = -ui_peek_font_size() - child_gap }
                 },
             })
        {
            ui_push_corner_radius(2.0f);
            if (game_ui_do_button("Close"))
            {
                is_done = true;
            }
            ui_pop_corner_radius();
        }
        
        mco_yield(co);
    }
    
    cf_array_free(commands);
    cf_array_free(sprites);
}

void game_ui_do_credits(const char* campaign_name)
{
    fixed Asset_Resource** campaign_resources = assets_get_resources_of_type(Asset_Resource_Type_Campaign);
    dyna Credits_Command* app_commands = assets_get_resource_property_value("app", "credits");
    dyna Credits_Command* commands = NULL;
    cf_array_fit(commands, 128);
    campaign_name = cf_sintern(campaign_name);
    
    Credits_Command font_size_command = 
    {
        .type = Credits_Command_Type_Font_Size,
        .font_size = 128.0f,
    };
    
    Credits_Command empty_command = 
    { 
        .type = Credits_Command_Type_Text,
        .text = " ",
    };
    
    for (s32 index = 0; index < cf_array_count(campaign_resources); ++index)
    {
        Asset_Resource* resource = campaign_resources[index];
        if (campaign_name && resource->name != campaign_name)
        {
            continue;
        }
        
        dyna Credits_Command* campaign_commands = resource_get(resource, "credits");
        if (cf_array_count(campaign_commands))
        {
            if (cf_array_count(commands))
            {
                // add a single empty line between campaigns so it doesn't looka too cluttered
                cf_array_push(commands, font_size_command);
                cf_array_push(commands, empty_command);
            }
            
            s32 expected_next_count = cf_array_count(commands) + cf_array_count(campaign_commands);
            if (expected_next_count > cf_array_capacity(commands))
            {
                cf_array_fit(commands, next_power_of_two(expected_next_count));
            }
            
            CF_MEMCPY(commands + cf_array_len(commands), campaign_commands, sizeof(campaign_commands[0]) * cf_array_len(campaign_commands));
            cf_array_len(commands) = expected_next_count;
        }
        else
        {
            printf("Failed to get campaign credits - %s\n", resource->name);
        }
    }
    
    {
        if (cf_array_count(app_commands))
        {
            if (cf_array_count(commands))
            {
                // add a couple empty lines to space out between campaign credits to general app credeits
                cf_array_push(commands, font_size_command);
                cf_array_push(commands, empty_command);
                cf_array_push(commands, empty_command);
            }
            
            s32 expected_next_count = cf_array_count(commands) + cf_array_count(app_commands);
            if (expected_next_count > cf_array_capacity(commands))
            {
                cf_array_fit(commands, next_power_of_two(expected_next_count));
            }
            
            CF_MEMCPY(commands + cf_array_len(commands), app_commands, sizeof(app_commands[0]) * cf_array_len(app_commands));
            cf_array_len(commands) = expected_next_count;
        }
        else
        {
            printf("Failed to get app credits\n");
        }
    }
    
    if (cf_array_count(commands))
    {
        ui_start_modal(game_ui_do_credits_co, commands);
    }
    else
    {
        cf_array_free(commands);
    }
}

void game_ui_start_demo_mode()
{
    if (s_app->world->state != World_State_Demo)
    {
        world_clear();
        world_load_random_demo_level();
    }
}

void game_ui_handle_demo_mode()
{
    World* world = s_app->world;
    Game_UI* game_ui = s_app->game_ui;
    game_ui->demo_reload_delay -= CF_DELTA_TIME;
    
    if (world->state == World_State_Demo)
    {
        if (game_ui->demo_reload_delay < 0)
        {
            world_clear();
            game_ui->demo_reload_delay = 60.0f;
            world_load_random_demo_level();
        }
    }
}

void game_ui_init()
{
    Game_UI* game_ui = cf_calloc(sizeof(Game_UI), 1);
    s_app->game_ui = game_ui;
    
    // don't do any event game ui state handling at startup
    cf_array_push(game_ui->states, Game_UI_State_Splash);
    
    {
        Asset_Resource* resource = assets_get_resource("ui");
        dyna const char** fonts = resource_get(resource, "fonts");
        if (fonts)
        {
            ui_set_fonts(fonts, cf_array_count(fonts));
        }
        
        cf_htbl const char*** button_data = resource_get(resource, "button");
        if (cf_hashtable_has(button_data, cf_sintern("sounds")))
        {
            dyna const char** button_sounds = cf_hashtable_get(button_data, cf_sintern("sounds"));
            cf_array_set(game_ui->button_sounds, button_sounds);
        }
    }
    
    s32 string_size = 256 + sizeof(CF_Ahdr);
    char* buf = cf_alloc(string_size);
    cf_string_static(game_ui->level_name, buf, string_size);
}

void game_ui_update()
{
    static show_perf = 0;
    
    World* world = s_app->world;
    Game_UI* game_ui = s_app->game_ui;
    game_ui->prev_hover_id = game_ui->hover_id;
    game_ui->hover_id = (Clay_ElementId){ 0 };
    
    ui_begin();
    
    game_ui_handle_demo_mode();
    
    Game_UI_State state = game_ui_peek_state();
    switch (state)
    {
        case Game_UI_State_Splash:
        {
            game_ui_do_splash();
        }
        break;
        case Game_UI_State_Campaign_Select:
        {
            game_ui_do_campaign_select();
        }
        break;
        case Game_UI_State_Play:
        {
            world_set_state(World_State_Play);
            audio_unpause(Audio_Source_Type_SFX);
            game_ui_do_play();
        }
        break;
        case Game_UI_State_Level_Finish:
        {
            world_set_state(World_State_Play);
            audio_unpause(Audio_Source_Type_SFX);
            game_ui_do_level_finish();
        }
        break;
        case Game_UI_State_Pause:
        {
            game_ui_do_pause();
        }
        break;
        case Game_UI_State_Options:
        {
            game_ui_do_options();
        }
        break;
        case Game_UI_State_Editor:
        {
            editor_set_state(Editor_State_Edit);
            editor_ui_do_main();
            if (s_app->ui->input.back_pressed)
            {
                game_ui_push_state(Game_UI_State_Editor_Pause);
            }
        }
        break;
        case Game_UI_State_Editor_Pause:
        {
            editor_set_state(Editor_State_Pause);
            game_ui_do_editor_pause();
        }
        break;
        case Game_UI_State_Main_Menu:
        default:
        {
            game_ui_do_main_menu();
        }
        break;
    }
    
    if (cf_key_just_pressed(CF_KEY_F1))
    {
        show_perf = !show_perf;
    }
    if (cf_key_just_pressed(CF_KEY_F2))
    {
        Clay_SetDebugModeEnabled(!Clay_IsDebugModeEnabled());
    }
    if (cf_key_just_pressed(CF_KEY_F5))
    {
        world->debug.show_ai_view_cone = !world->debug.show_ai_view_cone;
    }
    if (cf_key_just_pressed(CF_KEY_F6))
    {
        world->debug.show_pathing = !world->debug.show_pathing;
    }
    
    if (show_perf)
    {
        game_ui_do_perf();
    }
    
    ui_end();
}

void game_ui_do_splash_co(mco_coro* co)
{
    f32 delay = 3.0f;
    f32 t = 0.0f;
    
    // begin
    while (t <= 1.0f)
    {
        t += CF_DELTA_TIME;
        if (cf_key_just_pressed(CF_KEY_ESCAPE) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT))
        {
            t = 1.0f;
        }
        
        s_app->game_ui->splash_opacity = cf_lerp(0.0f, 1.0f, cf_clamp01(t));
        mco_yield(co);
    }
    
    // wait
    while (delay > 0)
    {
        delay -= CF_DELTA_TIME;
        if (cf_key_just_pressed(CF_KEY_ESCAPE) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT))
        {
            delay = 0.0f;
        }
        mco_yield(co);
    }
    
    // end
    t = 1.0f;
    while (t >= 0.0f)
    {
        t -= CF_DELTA_TIME;
        if (cf_key_just_pressed(CF_KEY_ESCAPE) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT) || cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT))
        {
            t = 0.0f;
        }
        
        s_app->game_ui->splash_opacity = cf_lerp(0.0f, 1.0f, cf_clamp01(t));
        mco_yield(co);
    }
    
    // finish
    s_app->game_ui->splash_index++;
}

void game_ui_do_splash()
{
    Game_UI* game_ui = s_app->game_ui;
    world_set_state(World_State_None);
    
    cf_htbl const char*** splash_table = assets_get_resource_property_value("ui", "splash");
    if (splash_table == NULL)
    {
        game_ui_set_state(Game_UI_State_Main_Menu);
        game_ui_start_demo_mode();
        return;
    }
    
    dyna const char** splash_sprites = cf_hashtable_get(splash_table, cf_sintern("sprites"));
    dyna const char** splash_animations = cf_hashtable_get(splash_table, cf_sintern("animations"));
    dyna const char** splash_titles = cf_hashtable_get(splash_table, cf_sintern("titles"));
    
    if (!ui_is_modal_active())
    {
        if (game_ui->splash_index < cf_array_count(splash_sprites))
        {
            ui_start_modal_with_color(game_ui_do_splash_co, (Clay_Color){ 0, 0, 0, 10 }, NULL);
        }
        else
        {
            game_ui_set_state(Game_UI_State_Main_Menu);
            game_ui_start_demo_mode();
            world_unload_campaign();
        }
    }
    
    CLAY(CLAY_ID("Play_OuterContainer"), {
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        s32 splash_index = cf_clamp_int(s_app->game_ui->splash_index, 0, cf_array_count(splash_sprites) - 1);
        const char* splash_sprite_name = splash_sprites[splash_index];
        const char* splash_animation_name = splash_animations[splash_index];
        const char* splash_title = splash_titles[splash_index];
        
        if (CF_STRLEN(splash_sprite_name) == 0)
        {
            splash_sprite_name = NULL;
        }
        if (CF_STRLEN(splash_animation_name) == 0)
        {
            splash_animation_name = NULL;
        }
        if (CF_STRLEN(splash_title) == 0)
        {
            splash_title = NULL;
        }
        
        if (splash_sprite_name)
        {
            CF_Sprite* splash_sprite = assets_get_sprite(splash_sprite_name);
            if (splash_sprite)
            {
                CF_Sprite sprite = *splash_sprite;
                sprite.opacity = game_ui->splash_opacity;
                if (splash_animation_name)
                {
                    cf_sprite_play(&sprite, splash_animation_name);
                }
                
                CF_V2 size = cf_v2(200, 200);
                if (sprite.w < size.x || sprite.h < size.y)
                {
                    f32 multiplier = cf_max(size.x / sprite.w, size.y / sprite.h);
                    size.x = sprite.w * multiplier;
                    size.y = sprite.h * multiplier;
                }
                else
                {
                    size.x = (f32)sprite.w;
                    size.y = (f32)sprite.h;
                }
                ui_do_sprite(&sprite, size);
            }
        }
        
        if (splash_title)
        {
            Clay_Color text_color = ui_peek_text_color();
            text_color.a = 255.0f * game_ui->splash_opacity;
            ui_push_text_color(text_color);
            ui_do_text(splash_title);
            ui_pop_text_color();
        }
    }
}

void game_ui_do_play()
{
    World* world = s_app->world;
    Game_UI* game_ui = s_app->game_ui;
    UI_Input* input = &s_app->ui->input;
    Level_Stats* stats = &world->level_stats;
    
    s32 w = s_app->w;
    s32 h = s_app->h;
    
    f32 unit_footer_max_height = h * GAME_UI_CONTROL_FOOTER_PERCENT;
    
    ecs_t* ecs = s_app->ecs;
    ecs_id_t component_sprite_id = ECS_GET_COMPONENT_ID(C_Sprite);
    ecs_id_t component_weapon_id = ECS_GET_COMPONENT_ID(C_Weapon);
    ecs_id_t component_corpse_id = ECS_GET_COMPONENT_ID(C_Corpse);
    
    if (input->back_pressed)
    {
        game_ui_push_state(Game_UI_State_Pause);
    }
    
    if (stats->activated_exits && stats->activated_exits >= stats->total_exits)
    {
        game_ui_push_state(Game_UI_State_Level_Finish);
    }
    
    CLAY(CLAY_ID("Play_OuterContainer"), {
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_LEFT,
                     .y = CLAY_ALIGN_Y_BOTTOM,
                 }
             },
         })
    {
        ecs_id_t component_ui_id = ECS_GET_COMPONENT_ID(C_UI);
        
        CLAY(CLAY_ID("FooterContainer_Controllable_Units"), {
                 .backgroundColor = { 100, 80, 120, 255 },
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_FIT(128.0f, unit_footer_max_height),
                     },
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_BOTTOM,
                     }
                 },
                 .clip = {
                     .horizontal = true,
                     .childOffset = Clay_GetScrollOffset(),
                 }
             })
        {
            Clay_OnHover(clay_on_hover, 0);
            //  @todo:  9 slice sprite for background
            f32 unit_width = w * 0.25f;
            for (s32 index = 0; index < cf_array_count(game_ui->control_ids); ++index)
            {
                b32 is_dead = false;
                f32 ammunition_ratio = 0.0f;
                C_Sprite* sprite = NULL;
                ecs_id_t entity = game_ui->control_ids[index];
                C_UI* c_ui = NULL;
                if (ecs_has(s_app->ecs, entity, component_ui_id))
                {
                    c_ui = ecs_get(s_app->ecs, entity, component_ui_id);
                }
                
                if (ecs_is_ready(ecs, entity))
                {
                    if (ecs_has(ecs, entity, component_sprite_id))
                    {
                        sprite = ecs_get(ecs, entity, component_sprite_id);
                    }
                    if (ecs_has(ecs, entity, component_weapon_id))
                    {
                        C_Weapon* weapon = ecs_get(ecs, entity, component_weapon_id);
                        ammunition_ratio = cf_clamp01((f32)weapon->ammunition / weapon->max_ammunition);
                    }
                    is_dead = ecs_has(ecs, entity, component_corpse_id);
                }
                
                Clay_ElementId unit_container_id = CLAY_IDI_LOCAL("UnitContainer", index);
                CLAY(unit_container_id, {
                         .backgroundColor = { 100, 100, 110, 255 },
                         .layout = {
                             .layoutDirection = CLAY_LEFT_TO_RIGHT,
                             .sizing = {
                                 .width = CLAY_SIZING_FIXED(unit_width),
                                 .height = CLAY_SIZING_GROW(0),
                             },
                             .padding = CLAY_PADDING_ALL(16),
                             .childGap = 8,
                             .childAlignment = {
                                 .x = CLAY_ALIGN_X_LEFT,
                                 .y = CLAY_ALIGN_Y_TOP,
                             }
                         },
                     })
                {
                    ui_push_idle_color((Clay_Color){ 0 });
                    ui_push_down_color((Clay_Color){ 0 });
                    ui_push_hover_color((Clay_Color){ 0 });
                    ui_push_select_color((Clay_Color){ 0 });
                    
                    b32 do_select = false;
                    
                    if (Clay_PointerOver(unit_container_id))
                    {
                        make_event_on_ui_hover_control_unit(entity);
                        
                        if (input->mouse_press)
                        {
                            do_select = true;
                        }
                    }
                    
                    if (c_ui && c_ui->current)
                    {
                        ui_push_background_sprite((UI_Layer_Sprite){
                                                      .type = UI_Sprite_Params_Type_Image,
                                                      .name = c_ui->current->sprite,
                                                      .animation = c_ui->current->animation,
                                                      .is_9_slice = true,
                                                  });
                    }
                    
                    
                    if (is_dead)
                    {
                        ui_push_foreground_sprite((UI_Layer_Sprite){
                                                      .type = UI_Sprite_Params_Type_Image, 
                                                      .name = c_ui->dead.sprite,
                                                      .animation = c_ui->dead.animation,
                                                  });
                    }
                    
                    if (sprite)
                    {
                        ui_do_sprite(&sprite->sprite, cf_v2(128, 128));
                    }
                    else
                    {
                        C_Asset_Resource* lookup_resource = ECS_GET_COMPONENT(entity, C_Asset_Resource);
                        CF_ASSERT(lookup_resource);
                        Asset_Resource* resource = assets_get_resource(lookup_resource->name);
                        
                        ui_do_text(resource->name);
                    }
                    
                    if (is_dead)
                    {
                        ui_pop_foreground_sprite();
                    }
                    
                    if (do_select)
                    {
                        make_event_do_select_control_unit(entity);
                    }
                    
                    ui_push_idle_color((Clay_Color){ 0 });
                    ui_push_down_color((Clay_Color){ 0 });
                    ui_push_hover_color((Clay_Color){ 0 });
                    ui_push_select_color((Clay_Color){ 0 });
                    
                    CLAY(ui_make_clay_id_index("UnitContainer_Resources", index), {
                             .backgroundColor = { 100, 100, 110, 255 },
                             .layout = {
                                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                 .sizing = {
                                     .width = CLAY_SIZING_GROW(0),
                                     .height = CLAY_SIZING_GROW(0),
                                 },
                                 .padding = CLAY_PADDING_ALL(8),
                                 .childGap = 8,
                                 .childAlignment = {
                                     .x = CLAY_ALIGN_X_LEFT,
                                     .y = CLAY_ALIGN_Y_TOP,
                                 }
                             },
                         })
                    {
                        ui_push_corner_radius(2.0f);
                        ui_push_interactable(false);
                        ui_push_idle_color(color_to_clay_color(cf_color_purple()));
                        ui_do_slider(&ammunition_ratio, 0, 1);
                        ui_pop_interactable();
                        ui_pop_idle_color();
                        ui_pop_corner_radius();
                    }
                }
                
                if (c_ui && c_ui->current)
                {
                    ui_pop_background_sprite();
                }
            }
            
        }
    }
}

void game_ui_do_level_finish()
{
    static b32 play_campaign_credits = false;
    
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    
    
    CLAY(CLAY_ID("LevelFinish_OuterContainer"), {
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        // don't do title while credits if active
        if (!ui_is_modal_active())
        {
            ui_push_font_size(32);
            ui_do_text(world->level.name);
            ui_pop_font_size();
        }
        
        b32 go_to_main_menu = false;
        Campaign_State campaign_state = world_get_campaign_state();
        //  @todo:  if campaign_state is finished, push credits
        
        ui_push_corner_radius(2.0f);
        if (campaign_state == Campaign_State_In_Progress)
        {
            if (world_remaining_campaign_levels() > 1)
            {
                if (game_ui_do_button("Next Level"))
                {
                    if (world_advance_campaign())
                    {
                        game_ui_pop_state();
                    }
                }
            }
            else
            {
                //  @hack:  advance once to finish the campaign
                world_advance_campaign();
            }
        }
        else
        {
            if (campaign_state == Campaign_State_Invalid)
            {
                if (editor->state == Editor_State_Edit_Play)
                {
                    if (game_ui_do_button("Editor"))
                    {
                        game_ui_set_state(Game_UI_State_Main_Menu);
                        game_ui_push_state(Game_UI_State_Editor);
                        editor_resume_from_play();
                    }
                }
            }
            
            if (campaign_state == Campaign_State_Finish)
            {
                if (!play_campaign_credits)
                {
                    game_ui_do_credits(world_get_campaign_name());
                    play_campaign_credits = true;
                }
                // don't do main menu button while credits if active
                if (!ui_is_modal_active())
                {
                    if (game_ui_do_button("Main Menu"))
                    {
                        go_to_main_menu = true;
                    }
                }
            }
        }
        ui_pop_corner_radius();
        
        if (go_to_main_menu)
        {
            game_ui_set_state(Game_UI_State_Main_Menu);
            world_unload_campaign();
            game_ui_start_demo_mode();
            play_campaign_credits = false;
        }
    }
}

void game_ui_do_main_menu()
{
    Game_UI* game_ui = s_app->game_ui;
    
    ui_push_corner_radius(2.0f);
    
    CLAY(CLAY_ID("MainMenu_OuterContainer"), {
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .padding = CLAY_PADDING_ALL(16),
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        Clay_OnHover(clay_on_hover, 0);
        
        // don't do buttons while credits if active
        if (!ui_is_modal_active())
        {
            if (game_ui_do_button("Campaign"))
            {
                game_ui_push_state(Game_UI_State_Campaign_Select);
            }
            if (game_ui_do_button("Editor"))
            {
                game_ui_push_state(Game_UI_State_Editor);
                game_ui_push_state(Game_UI_State_Editor_Pause);
            }
            if (game_ui_do_button("Options"))
            {
                game_ui_push_state(Game_UI_State_Options);
            }
            if (game_ui_do_button("Credits"))
            {
                game_ui_do_credits(NULL);
            }
            if (game_ui_do_button("Quit"))
            {
                cf_app_signal_shutdown();
            }
        }
    }
    
    ui_pop_corner_radius();
}

void game_ui_do_campaign_select()
{
    Assets* assets = s_app->assets;
    Game_UI* game_ui = s_app->game_ui;
    
    ui_push_corner_radius(2.0f);
    
    fixed Asset_Resource** campaign_resources = assets_get_resources_of_type(Asset_Resource_Type_Campaign);
    
    CLAY(CLAY_ID("CampaignSelectMenu_OuterContainer"), {
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .padding = CLAY_PADDING_ALL(16),
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        Clay_OnHover(clay_on_hover, 0);
        
        if (cf_key_just_pressed(CF_KEY_ESCAPE) && !ui_is_modal_active())
        {
            game_ui_pop_state();
        }
        
        const char* current_campaign_name = world_get_campaign_name();
        
        ui_push_font_size(32);
        ui_do_text("Select a campaign");
        ui_pop_font_size();
        
        Clay_LayoutAlignmentX child_alignment_x = CLAY_ALIGN_X_LEFT;
        if (cf_array_count(campaign_resources) < 3)
        {
            child_alignment_x =  CLAY_ALIGN_X_CENTER;
        }
        
        //  @todo:  slider looks bad, set it so its a button to go left or right
        CF_V2 image_size = cf_v2(256.0f, 256.0f);
        CLAY(CLAY_ID("CampaignSelectList_Container"), {
                 .backgroundColor = { 0, 0, 0, 128 },
                 .layout = {
                     .layoutDirection = CLAY_LEFT_TO_RIGHT,
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(0.5f),
                         .height = CLAY_SIZING_FIT(0),
                     },
                     .childGap = 16,
                     .childAlignment = {
                         .x = child_alignment_x,
                         .y = CLAY_ALIGN_Y_TOP,
                     }
                 },
                 .clip = {
                     .horizontal = true,
                     .childOffset = Clay_GetScrollOffset(),
                 },
             })
        {
            for (s32 index = 0; index < cf_array_count(campaign_resources); ++index)
            {
                Asset_Resource* resource = campaign_resources[index];
                Sprite_Reference* reference = resource_get(resource, "sprite");
                CF_Sprite* sprite = assets_get_sprite(reference->sprite);
                
                b32 selected_campaign = false;
                if (reference)
                {
                    if (game_ui_do_sprite_button(sprite, image_size))
                    {
                        selected_campaign = true;
                    }
                }
                else
                {
                    if (game_ui_do_button(resource->name))
                    {
                        selected_campaign = true;
                    }
                }
                
                if (selected_campaign)
                {
                    world_load_campaign(resource->id);
                }
            }
        }
        
        if (current_campaign_name)
        {
            ui_do_text(current_campaign_name);
        }
        if (game_ui_do_button("Play"))
        {
            if (world_advance_campaign())
            {
                game_ui_pop_state();
                game_ui_push_state(Game_UI_State_Play);
            }
        }
        if (game_ui_do_button("Back"))
        {
            game_ui_pop_state();
        }
    }
    
    ui_pop_corner_radius();
}

void game_ui_do_pause()
{
    ui_push_corner_radius(2.0f);
    
    if (cf_key_just_pressed(CF_KEY_ESCAPE) && !ui_is_modal_active())
    {
        game_ui_pop_state();
    }
    
    CLAY(CLAY_ID("Pause_OuterContainer"), {
             .backgroundColor = { 0, 0, 0, 120 },
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .padding = CLAY_PADDING_ALL(16),
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        Clay_OnHover(clay_on_hover, 0);
        
        ui_push_font_size(32);
        ui_do_text(s_app->world->level.name);
        ui_pop_font_size();
        
        if (game_ui_do_button("Continue"))
        {
            game_ui_pop_state();
        }
        if (game_ui_do_button("Retry"))
        {
            world_reload();
            game_ui_pop_state();
        }
        if (game_ui_do_button("Options"))
        {
            game_ui_push_state(Game_UI_State_Options);
        }
        if (s_app->editor->state == Editor_State_Edit_Play)
        {
            if (game_ui_do_button("Editor"))
            {
                game_ui_set_state(Game_UI_State_Main_Menu);
                game_ui_push_state(Game_UI_State_Editor);
                editor_resume_from_play();
            }
        }
        else
        {
            if (game_ui_do_button("Main Menu"))
            {
                game_ui_set_state(Game_UI_State_Main_Menu);
                game_ui_start_demo_mode();
                world_unload_campaign();
            }
        }
    }
    ui_pop_corner_radius();
}

void game_ui_do_options()
{
    UI_Input* input = &s_app->ui->input;
    Audio_System* audio_system = s_app->audio_system;
    
    ui_push_corner_radius(2.0f);
    
    if (input->back_pressed)
    {
        game_ui_pop_state();
    }
    
    CLAY(CLAY_ID("Options_OuterContainer"), {
             .backgroundColor = { 128, 128, 128, 128 },
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .padding = CLAY_PADDING_ALL(16),
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        Clay_OnHover(clay_on_hover, 0);
        
        CLAY(CLAY_ID("Options_InnerContainer"), {
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(0.25f),
                     },
                     .padding = CLAY_PADDING_ALL(16),
                     .childGap = 16,
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_CENTER,
                     }
                 },
             })
        {
            ui_do_text("Audio");
            ui_do_text("Master: %.0f", audio_system->volume_master * 100.0f);
            ui_do_slider(&audio_system->volume_master, 0, 1);
            ui_do_text("Music: %.0f", audio_system->volume_music * 100.0f);
            ui_do_slider(&audio_system->volume_music, 0, 1);
            ui_do_text("SFX: %.0f", audio_system->volume_sfx * 100.0f);
            ui_do_slider(&audio_system->volume_sfx, 0, 1);
            ui_do_text("UI: %.0f", audio_system->volume_ui * 100.0f);
            ui_do_slider(&audio_system->volume_ui, 0, 1);
        }
        
        if (game_ui_do_button("Back"))
        {
            game_ui_pop_state();
        }
    }
    
    ui_pop_corner_radius();
}

void game_ui_new_level_modal_co(mco_coro* co)
{
    World* world = s_app->world;
    Game_UI* game_ui = s_app->game_ui;
    fixed char* level_name = mco_get_user_data(co);
    cf_string_clear(level_name);
    b32 accepted = false;
    b32 canceled = false;
    
    while (!accepted && !canceled)
    {
        ui_push_corner_radius(2.0f);
        Clay_OnHover(clay_on_hover, 0);
        
        CLAY(CLAY_ID("EditorPauseNewLevelModalName_OuterContainer"), {
                 .backgroundColor = { 0, 0, 0, 255 },
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .padding = CLAY_PADDING_ALL(16),
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(0.25f),
                     },
                     .childGap = 16,
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_CENTER,
                         .y = CLAY_ALIGN_Y_CENTER,
                     }
                 },
             })
        {
            CLAY(CLAY_ID("EditorPauseNewLevelModalName_InnerContainer"), {
                     .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                     .layout = {
                         .layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .padding = CLAY_PADDING_ALL(16),
                         .sizing = {
                             .width = CLAY_SIZING_FIT(300),
                         },
                         .childGap = 16,
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_CENTER,
                             .y = CLAY_ALIGN_Y_CENTER,
                         }
                     },
                 })
            {
                ui_do_text("Name");
                ui_do_input_text(level_name, UI_Input_Text_Mode_Default);
            }
            
            CLAY(CLAY_ID("EditorPauseNewLevelModalButtons_InnerContainer"), {
                     .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                     .layout = {
                         .layoutDirection = CLAY_LEFT_TO_RIGHT,
                         .padding = CLAY_PADDING_ALL(16),
                         .childGap = 16,
                         .childAlignment = {
                             .x = CLAY_ALIGN_X_CENTER,
                             .y = CLAY_ALIGN_Y_CENTER,
                         }
                     },
                 })
            {
                if (game_ui_do_button("Cancel"))
                {
                    canceled = true;
                }
                if (game_ui_do_button("Create"))
                {
                    // @todo:  check if file exists,
                    //         prompt another "modal" to overwrite or cancel the create
                    if (cf_string_len(level_name))
                    {
                        accepted = true;
                    }
                }
            }
        }
        
        ui_pop_corner_radius();
        mco_yield(co);
    }
    
    if (accepted)
    {
        game_ui_pop_state();
        editor_new_level(level_name);
        make_event_load_level(EDITOR_LEVEL_EMPTY_NAME);
    }
}

void game_ui_list_level_modal_co(mco_coro* co)
{
    File_Dialog_Params* params = mco_get_user_data(co);
    const char* level_name = params->path;
    
    if (game_ui_do_file_dialog_co_ex(co))
    {
        editor_load_level(level_name, true);
        game_ui_pop_state();
    }
}

void game_ui_do_editor_pause()
{
    Game_UI* game_ui = s_app->game_ui;
    ui_push_corner_radius(2.0f);
    
    b32 switch_to_main_menu = false;
    
    if (cf_key_just_pressed(CF_KEY_ESCAPE) && !ui_is_modal_active())
    {
        if (editor_is_active())
        {
            game_ui_pop_state();
        }
        else
        {
            switch_to_main_menu = true;
        }
    }
    
    CLAY(CLAY_ID("EditorPause_OuterContainer"), {
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_GROW(0),
                 },
                 .padding = CLAY_PADDING_ALL(16),
                 .childGap = 16,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 }
             },
         })
    {
        Clay_OnHover(clay_on_hover, 0);
        CLAY(CLAY_ID("EditorPause_InnerContainer"), {
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(0.25f),
                     },
                     .padding = CLAY_PADDING_ALL(16),
                     .childGap = 16,
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_CENTER,
                         .y = CLAY_ALIGN_Y_CENTER,
                     }
                 },
             })
        {
            if (editor_is_active())
            {
                if (game_ui_do_button("Continue"))
                {
                    game_ui_pop_state();
                }
            }
            
            if (game_ui_do_button("New"))
            {
                ui_start_modal(game_ui_new_level_modal_co, game_ui->level_name);
            }
            if (game_ui_do_button("Load"))
            {
                File_Dialog_Params* params = scratch_alloc(sizeof(File_Dialog_Params));
                fixed const char** filters = NULL;
                MAKE_SCRATCH_ARRAY(filters, 8);
                cf_array_push(filters, LEVEL_FILE_TYPE_SUFFIX);
                
                *params = (File_Dialog_Params){
                    .path = game_ui->level_name,
                    .filters = filters,
                    .filter_count = cf_array_count(filters),
                };
                cf_string_clear(game_ui->level_name);
                ui_start_modal(game_ui_list_level_modal_co, params);
            }
            if (game_ui_do_button("Options"))
            {
                game_ui_push_state(Game_UI_State_Options);
            }
            if (game_ui_do_button("Main Menu"))
            {
                switch_to_main_menu = true;
            }
        }
    }
    ui_pop_corner_radius();
    
    if (switch_to_main_menu)
    {
        game_ui_set_state(Game_UI_State_Main_Menu);
        editor_reset();
        game_ui_start_demo_mode();
        world_unload_campaign();
    }
}

void game_ui_do_perf()
{
    CLAY(CLAY_ID("Perf_OuterContainer"), {
             .floating = {
                 .attachTo = CLAY_ATTACH_TO_ROOT,
                 .attachPoints =  {
                     .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                 },
                 .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
             },
             .layout = {
                 .layoutDirection = CLAY_TOP_TO_BOTTOM,
                 .sizing = {
                     .width = CLAY_SIZING_FIT(0),
                     .height = CLAY_SIZING_FIT(0),
                 },
                 .childGap = 8,
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_LEFT,
                     .y = CLAY_ALIGN_Y_TOP,
                 }
             },
             .backgroundColor = { 0, 0, 0, 10 }
         })
    {
        s32 count = cf_hashtable_count(s_app->perfs);
        const char** tags = (const char**)cf_hashtable_keys(s_app->perfs);
        f64* durations = cf_hashtable_items(s_app->perfs);
        
        for (s32 index = 0; index < count; ++index)
        {
            ui_do_text("%s - %.2fus", tags[index], durations[index]);
        }
        ui_do_text("tile draws - %d", s_app->draw_tile_count);
        ui_do_text("draw calls - %d", s_app->draw_calls);
        ui_do_text("fps - %.2f", cf_app_get_framerate());
        ui_do_text("smooth - %.2f", cf_app_get_smoothed_framerate());
    }
}

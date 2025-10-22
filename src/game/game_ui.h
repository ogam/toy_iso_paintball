#ifndef GAME_UI_H
#define GAME_UI_H

#define GAME_UI_CONTROL_FOOTER_PERCENT (0.2f)

typedef struct File_Dialog_Params
{
    fixed char* path;
    const char** filters;
    s32 filter_count;
} File_Dialog_Params;

typedef s32 Game_UI_State;
enum
{
    Game_UI_State_Splash,
    Game_UI_State_Main_Menu,
    Game_UI_State_Campaign_Select,
    Game_UI_State_Play,
    Game_UI_State_Level_Finish,
    Game_UI_State_Pause,
    Game_UI_State_Options,
    Game_UI_State_Editor,
    Game_UI_State_Editor_Pause,
};

typedef s32 Game_UI_Options_Tab;
enum
{
    Game_UI_Options_Tab_Audio,
    Game_UI_Options_Tab_Input,
    Game_UI_Options_Tab_Controller,
};

typedef struct Game_UI
{
    dyna Game_UI_State* states;
    
    // splash screen
    s32 splash_index;
    f32 splash_opacity;
    
    // main menu demo
    f32 demo_reload_delay;
    
    // controlled units ui to draw during play mode
    dyna ecs_id_t* control_ids;
    
    Game_UI_Options_Tab options_tab;
    
    // this is mainly just for UI typing to not thrash current editor
    // level name
    fixed char* level_name;
    
    //  @todo:  fix this so only 1 is needed
    Clay_ElementId hover_id;
    Clay_ElementId prev_hover_id;
} Game_UI;

void game_ui_handle_state(Game_UI_State state, Game_UI_State next_state);
void game_ui_set_state(Game_UI_State state);
void game_ui_push_state(Game_UI_State state);
Game_UI_State game_ui_peek_state();
Game_UI_State game_ui_pop_state();

void game_ui_control_add(ecs_id_t id);
void game_ui_control_remove(ecs_id_t id);
void game_ui_control_clear(ecs_id_t id);

// use this to have tile selection disabled when hovering over UI layouts
// Clay_OnHover(clay_on_hover, 0)
void clay_on_hover(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData);

void game_ui_play_button_sound();

b32 game_ui_do_button(const char* text);
b32 game_ui_do_button_wide(const char* text);
b32 game_ui_do_image_button(const char* name, const char* animation, CF_V2 size);
b32 game_ui_do_sprite_button(CF_Sprite* sprite, CF_V2 size);
b32 game_ui_is_hovering_over_any_layouts();

void game_ui_set_item_tooltip(const char* fmt, ...);
b32 game_ui_do_file_dialog_co_ex(mco_coro* co);
void game_ui_do_file_dialog_co(mco_coro* co);
void game_ui_do_credits_co(mco_coro* co);
void game_ui_do_credits(const char* campaign_name);

void game_ui_start_demo_mode();
void game_ui_handle_demo_mode();

void game_ui_init();
void game_ui_update();
void game_ui_do_splash();
void game_ui_do_campaign_select();
void game_ui_do_play();
void game_ui_do_level_finish();
void game_ui_do_main_menu();
void game_ui_do_pause();
void game_ui_do_options();
void game_ui_do_editor_pause();
void game_ui_do_perf();

#endif //GAME_UI_H

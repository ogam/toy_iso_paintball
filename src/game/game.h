#ifndef GAME_H
#define GAME_H

typedef s32 Input_Multiselect_State;
enum
{
    Input_Multiselect_State_None,
    Input_Multiselect_State_Selecting,
    Input_Multiselect_State_Commit,
    Input_Multiselect_State_Finish,
};

typedef struct Input
{
    CF_V2 screen_mouse;
    CF_V2 world_mouse;
    V2i tile_select;
    V2i tile_select_start;
    V2i tile_select_end;
    
    b32 is_holding_add_remove;
    b32 move;
    b32 select;
    b32 fire;
    Input_Multiselect_State multiselect;
    mco_coro* multiselect_co;
} Input;

typedef struct App
{
    struct Assets* assets;
    struct Input* input;
    struct Memory* memory;
    struct World* world;
    struct UI* ui;
    struct Game_UI* game_ui;
    struct Audio_System* audio_system;
    struct Editor* editor;
    ecs_t* ecs;
    
    cf_htbl ecs_id_t* component_ids;
    cf_htbl ecs_id_t* system_ids;
    
    //  @todo:  beef up the perf stuff, have it over N frames
    //          use a ring buffer for each frame perf test
    cf_htbl CF_Stopwatch* stopwatches;
    cf_htbl f64* perfs;
    
    //  @todo:  remove
    s32 draw_calls;
    s32 draw_tile_count;
    
    CF_Canvas credits_canvas;
    b32 credits_canvas_dirty;
    
    s32 w;
    s32 h;
} App;

//#define GAME_WIDTH 1024
//#define GAME_HEIGHT 768
#define GAME_WIDTH 1280
#define GAME_HEIGHT 800

void game_handle_window_state();
void game_init();
void game_deinit();
void game_update_input();
void game_update(void* udata);
void game_draw();

void new_frame();

CF_V2 focus_camera(CF_V2* focus_positions, s32 count, ecs_dt_t dt);

#endif //GAME_H

#pragma warning(push)
#pragma warning(disable: 4267)

#define ECS_DT_TYPE float
#define PICO_ECS_MAX_COMPONENTS (64)
#define PICO_ECS_MAX_SYSTEMS (64)
#define PICO_ECS_IMPLEMENTATION
#include "pico/pico_ecs.h"

#pragma warning(pop)

#define MIN_ENTITIES (1024)

#define PICO_QT_IMPLEMENTATION
#include "pico/pico_qt.h"

#define MINICORO_IMPL
#include "edubart/minicoro.h"

#include "common/types.h"
#include "common/macros.h"
#include "common/math.h"
#include "common/memory.h"
#include "common/priority_queue.h"
#include "common/string.h"
#include "assets/assets.h"
#include "game/audio.h"
#include "game/draw.h"
#include "game/entity.h"
#include "game/entity_grid.h"
#include "game/game.h"
#include "game/editor.h"
#include "game/editor_ui.h"
#include "game/ui.h"
#include "game/game_ui.h"
#include "util/perf.h"
#include "util/util.h"

App *s_app;

void game_rebuild_canvases()
{
    s32 w, h;
    cf_app_get_size(&w, &h);
    
    cf_app_set_canvas_size(w, h);
    
    if (s_app->credits_canvas.id)
    {
        cf_destroy_canvas(s_app->credits_canvas);
    }
    
    CF_CanvasParams params = cf_canvas_defaults(w, h);
    s_app->credits_canvas = cf_make_canvas(params);
}

void game_handle_multiselect_co(mco_coro* co)
{
    if (s_app->input->multiselect == Input_Multiselect_State_Commit)
    {
        s_app->input->multiselect = Input_Multiselect_State_Finish;
    }
    
    while(s_app->input->multiselect == Input_Multiselect_State_Finish)
    {
        if (!cf_mouse_down(CF_MOUSE_BUTTON_LEFT) && !cf_mouse_down(CF_MOUSE_BUTTON_RIGHT))
        {
            s_app->input->multiselect = Input_Multiselect_State_None;
            break;
        }
        mco_yield(co);
    }
    
    if ((cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT) || 
         cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT)) &&
        cf_key_shift())
    {
        s_app->input->tile_select_start = s_app->input->tile_select;
        s_app->input->tile_select_end = s_app->input->tile_select;
        s_app->input->multiselect = Input_Multiselect_State_Selecting;
    }
    else
    {
        s_app->input->tile_select_start = v2i(.x = -1, .y = -1);
        s_app->input->tile_select_end = v2i(.x = -1, .y = -1);
        s_app->input->multiselect = Input_Multiselect_State_None;
        return;
    }
    mco_yield(co);
    while ((cf_mouse_down(CF_MOUSE_BUTTON_LEFT) || 
            cf_mouse_down(CF_MOUSE_BUTTON_RIGHT)) &&
           cf_key_shift())
    {
        s_app->input->tile_select_end = s_app->input->tile_select;
        mco_yield(co);
    }
    s_app->input->multiselect = Input_Multiselect_State_Commit;
}

void game_handle_window_state()
{
    if (cf_app_was_resized())
    {
        game_rebuild_canvases();
    }
}

void game_init()
{
    s_app = cf_calloc(sizeof(App), 1);
    s_app->input = cf_calloc(sizeof(Input), 1);
    
    {
        mco_desc desc = mco_desc_init(game_handle_multiselect_co, 0);
        mco_result result = mco_create(&s_app->input->multiselect_co, &desc);
        CF_ASSERT(result == MCO_SUCCESS);
    }
    
    memory_init();
    audio_init();
    assets_load_all();
    world_init();
    ui_init();
    game_ui_init();
    editor_init();
    
    {
        const char* icon = assets_get_resource_property_value("app", "icon");
        if (icon)
        {
            cf_app_set_icon(icon);
        }
    }
    
    game_rebuild_canvases();
}

void game_deinit()
{
    editor_cleanup_temp_files();
}

void game_update_input()
{
    Input* input = s_app->input;
    Editor* editor = s_app->editor;
    Camera* camera = &s_app->world->camera;
    
    b32 is_edit_mode = editor->state == Editor_State_Edit;
    CF_V2 screen_mouse = cf_v2(cf_mouse_x(), cf_mouse_y());
    CF_V2 world_mouse = cf_screen_to_world(screen_mouse);
    world_mouse = cf_add_v2(world_mouse, camera->position);
    V2i tile_select = get_tile_from_input_cursor(world_mouse, is_edit_mode);
    
    b32 multiselect = cf_key_shift();
    b32 select = cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT);
    b32 move = cf_mouse_down(CF_MOUSE_BUTTON_LEFT);
    b32 fire = cf_mouse_just_pressed(CF_MOUSE_BUTTON_RIGHT);
    b32 is_holding_add_remove = cf_key_shift() || cf_key_ctrl();
    
    MCO_RESUME(input->multiselect_co, game_handle_multiselect_co);
    
    if (input->multiselect != Input_Multiselect_State_None)
    {
        move = false;
        fire = false;
    }
    
    if (game_ui_is_hovering_over_any_layouts())
    {
        tile_select = v2i(.x = -1, .y = -1);
        move = false;
        fire = false;
    }
    
    // don't update tile select if out of bounds
    if (tile_select.x == -1 && tile_select.y == -1)
    {
        tile_select = input->tile_select;
    }
    
    if (input->multiselect == Input_Multiselect_State_Finish)
    {
        // ended early don't select anything
        select = false;
    }
    else if (input->multiselect == Input_Multiselect_State_Commit)
    {
        select = cf_mouse_just_released(CF_MOUSE_BUTTON_LEFT);
    }
    
    // disable move and fire while doing any multiselect
    if (input->multiselect != Input_Multiselect_State_None)
    {
        fire = false;
        move = false;
    }
    
    input->is_holding_add_remove = is_holding_add_remove;
    input->screen_mouse = screen_mouse;
    input->world_mouse = world_mouse;
    input->tile_select = tile_select;
    input->move = move;
    input->select = select;
    input->fire = fire;
    
    cf_app_get_size(&s_app->w, &s_app->h);
}

void game_update(void* udata)
{
    UNUSED(udata);
    
    game_update_input();
    
    world_update();
    editor_update();
    audio_update();
    
    perf_begin("ui_update");
    ui_update();
    game_ui_update();
    perf_end("ui_update");
}

void game_draw()
{
    world_draw();
    ui_draw();
    
    if (s_app->credits_canvas_dirty)
    {
        cf_draw_canvas(s_app->credits_canvas, cf_v2(0, 0), cf_v2((f32)s_app->w, (f32)s_app->h));
        cf_render_to(cf_app_get_canvas(), false);
        s_app->credits_canvas_dirty = false;
    }
}

void new_frame()
{
    scratch_reset();
}

CF_V2 focus_camera(CF_V2* focus_positions, s32 count, ecs_dt_t dt)
{
    World* world = s_app->world;
    Editor* editor = s_app->editor;
    Camera* camera = &world->camera;
    
    s32 w = s_app->w;
    s32 h = s_app->h;
    
    CF_Aabb aabb = get_level_aabb();
    // move level bounds down a bit so camera can focus the targets
    aabb.min.y -= h * GAME_UI_CONTROL_FOOTER_PERCENT;
    
    //  @todo:  should be a option to check if editor is on left, right or whatever to adjust
    //          bounds
    if (editor->state == Editor_State_Edit || editor->state == Editor_State_Pause)
    {
        aabb.min.x -= s_app->w * EDITOR_X_PERCENT;
    }
    
    CF_V2 camera_half_extents = cf_v2((f32)w, (f32)h);
    //CF_V2 camera_half_extents = cf_v2(GAME_WIDTH, GAME_HEIGHT);
    camera_half_extents = cf_mul_v2_f(camera_half_extents, 0.5f);
    aabb = cf_expand_aabb(aabb, cf_neg_v2(camera_half_extents));
    
    CF_V2 camera_focus_half_extents = cf_v2(0.075f, 0.075f);
    camera_focus_half_extents = cf_mul_v2(camera_half_extents, camera_focus_half_extents);
    CF_Aabb camera_focus_aabb = cf_make_aabb_center_half_extents(camera->position, camera_focus_half_extents);
    // adjust the focus aabb up so camera won't be in middle of the UI footer
    camera_focus_aabb.min.y += h * GAME_UI_CONTROL_FOOTER_PERCENT;
    
    CF_V2 focus_position = cf_v2(0, 0);
    
    for (s32 index = 0; index < count; ++index)
    {
        focus_position = cf_add_v2(focus_position, focus_positions[index]);
    }
    
    // only move the camera if we're focusing on any targets, otherwise leave the camera alone
    if (count)
    {
        focus_position = cf_div_v2_f(focus_position, (f32)count);
        focus_position = cf_clamp_aabb_v2(aabb, focus_position);
        camera->next_position = focus_position;
    }
    
    if (!cf_contains_point(camera_focus_aabb, camera->next_position))
    {
        f32 distance = cf_distance(camera->position, camera->next_position);
        camera->position = cf_lerp_v2(camera->position, camera->next_position, dt);
    }
    return camera->position;
}

#include "common/math.c"
#include "common/memory.c"
#include "common/string.c"
#include "assets/assets.c"
#include "game/audio.c"
#include "game/draw.c"
#include "game/entity.c"
#include "game/entity_grid.c"
#include "game/editor.c"
#include "game/editor_ui.c"
#include "game/ui.c"
#include "game/game_ui.c"
#include "util/perf.c"
#include "util/util.c"
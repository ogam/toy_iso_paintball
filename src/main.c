#include <cute.h>

#include "game/game.c"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    int display_index = 0;
    int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT;
    int width = GAME_WIDTH;
    int height = GAME_HEIGHT;
    
	CF_Result result = cf_make_app("toy iso paintball", display_index, 0, 0, width, height, options, argv[0]);
    
    if (cf_is_error(result)) return -1;
    
    game_init();
    
    cf_input_enable_ime();
    while (cf_app_is_running())
    {
        perf_begin("total");
        new_frame();
        
        perf_begin("frame");
        cf_app_update(game_update);
        // needs to be called after cf_app_update() since that polls SDL for 
        // any window events
        game_handle_window_state();
        perf_end("frame");
        
        perf_begin("draw");
        game_draw();
        s_app->draw_calls = cf_app_draw_onto_screen(false);
        perf_end("draw");
        perf_end("total");
        
    }
    cf_input_disable_ime();
    
    game_deinit();
    cf_destroy_app();
    
    return 0;
}
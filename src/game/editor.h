#ifndef EDITOR_H
#define EDITOR_H

#define EDITOR_LEVEL_EMPTY_NAME "@editor_empty_template"
#define EDITOR_LEVEL_NAME ".editor.ipl"
#define EDITOR_TILE_LAYER (0)
#define EDITOR_TILE_LAYER_NAME "tile"
#define EDITOR_DEFAULT_ELEVATION_MAX (20)
#define EDITOR_DEFAULT_ELEVATION_MIN (0)

typedef s32 Editor_Command_Type;
enum
{
    Editor_Command_Type_Tile,
    Editor_Command_Type_Object,
    Editor_Command_Type_Brush,
    Editor_Command_Type_Level_Size,
    Editor_Command_Type_Elevation,
    Editor_Command_Type_Remove_Switch_Link,
    Editor_Command_Type_Add_Switch_Link,
    Editor_Command_Type_Update_Switch_Link,
    Editor_Command_Type_Set_Camera_Tile,
};

typedef struct Tile_State
{
    Tile v;
    Asset_Object_ID id;
} Tile_State;

typedef struct Switch_Link_State
{
    // try to maintain same order in the list so it's less jarring to the user
    // when an item is added/removed
    s32 index;
    Switch_Link v;
} Switch_Link_State;

typedef struct Editor_Command
{
    //  @note:  all ids of the same value means it's part of a large single action change
    //          such as place_range() or remove_range(), undo and redo will process all
    //          same ids as a single undo or redo action
    s32 id;
    Editor_Command_Type type;
    union
    {
        struct
        {
            V2i p;
            Tile_State state;
        } tile;
        struct
        {
            V2i p;
            Asset_Object_ID v;
            s32 layer;
        } object;
        Asset_Object_ID brush;
        V2i level_size;
        V2i elevation_range;
        struct
        {
            // try to maintain same order in the list so it's less jarring to the user
            // when an item is added/removed
            s32 index;
            Switch_Link v;
        } switch_link;
        V2i camera_tile;
    };
} Editor_Command;

typedef struct Editor_Input_Config
{
    dyna struct Input_Binding* place;
    dyna struct Input_Binding* remove;
    dyna struct Input_Binding* floodfill_mode;
    dyna struct Input_Binding* brush_mode;
    dyna struct Input_Binding* auto_tiling;
    dyna struct Input_Binding* pan;
    dyna struct Input_Binding* pan_up;
    dyna struct Input_Binding* pan_down;
    dyna struct Input_Binding* pan_left;
    dyna struct Input_Binding* pan_right;
    dyna struct Input_Binding* place_switch_link_stairs_top;
    dyna struct Input_Binding* place_camera_tile;
} Editor_Input_Config;

typedef struct Editor_Input
{
    CF_V2 move_direction;
    b32 redo;
    b32 undo;
    b32 place;
    b32 remove;
    s32 next_brush_direction;
    b32 any_brush_pressed;
    
    b32 switch_floodfill_mode;
    b32 switch_brush_mode;
    
    b32 placed_switch_link_stairs_top;
    
    b32 placed_camera_tile;
} Editor_Input;

typedef s32 Editor_Brush_Mode;
enum
{
    Editor_Brush_Mode_Default = 0,
    Editor_Brush_Mode_Clamp_Min_Height = 1 << 0,
    Editor_Brush_Mode_Clamp_Max_Height = 1 << 1,
    Editor_Brush_Mode_Remove = 1 << 2,
    Editor_Brush_Mode_Floodfill = 1 << 3,
    
    // links tiles to another layer
    Editor_Brush_Mode_Switch_Link = 1 << 4,
    
    Editor_Brush_Mode_Clamp_Height = Editor_Brush_Mode_Clamp_Min_Height | Editor_Brush_Mode_Clamp_Max_Height,
};

typedef s32 Editor_State;
enum
{
    Editor_State_None,
    Editor_State_Edit,
    Editor_State_Edit_Play,
    Editor_State_Pause,
};

typedef struct Editor
{
    V2i level_size;
    CF_V2 position;
    
    fixed char* file_name;
    fixed char* name;
    fixed char* music_file_name;
    fixed char* background_file_name;
    
    Editor_Brush_Mode brush_mode;
    Asset_Object_ID brush;
    s32 elevation_max;
    s32 elevation_min;
    
    V2i camera_tile;
    
    b32 is_auto_tiling;
    dyna V2i* auto_tile_queue;
    
    // there's a `category` tag in each Asset_Resource.editor
    // each one of those will result in a layer name so when
    // placing objects in the editor you can only place objects
    // of different categories on the same tile position
    const char** layer_names;
    Asset_Object_ID** layers;
    s32 layer_count;
    s32 layer_index;
    
    dyna Editor_Command* redos;
    dyna Editor_Command* undos;
    s32 stack_index;
    
    s32 command_id;
    f32 tile_change_delay;
    
    f32 time;
    
    Editor_Input input;
    Editor_Input_Config input_config;
    Editor_Input_Config temp_input_config;
    
    Editor_State state;
} Editor;

void editor_init();

void editor_init_input_config(Editor_Input_Config* config);
Editor_Input_Config* editor_make_temp_input_config();
void editor_apply_temp_input_config();
b32 editor_input_config_has_changed();
void editor_input_config_save();
void editor_input_config_load();
void editor_update_input();

void editor_update();
void editor_draw();
void editor_reset();

b32 editor_is_active();

void editor_set_state(Editor_State state);
void editor_resume_from_play();

void editor_apply_command(Editor_Command* command);
void editor_redo();
void editor_undo();
void editor_push_command(Editor_Command redo, Editor_Command undo);
void editor_push_command_tile_change(V2i position, Tile_State before, Tile_State after);
void editor_push_command_object_change(V2i position, Asset_Object_ID before, Asset_Object_ID after, s32 layer);
void editor_push_command_brush_change(Asset_Object_ID before, Asset_Object_ID after);
void editor_push_command_level_size_change(V2i before, V2i after);
void editor_push_command_elevation_change(V2i before, V2i after);

void editor_brush_mode_update_brush();
void editor_brush_mode_update_switch_link();

void editor_brush_mode_set_floodfill();
void editor_brush_mode_set_brush();
b32 editor_brush_mode_is_floodfill();
b32 editor_brush_mode_is_brush();
b32 editor_brush_mode_is_switch_link();

void editor_brush_mode_set_auto_tiling(b32 true_to_auto_tile);
b32 editor_brush_mode_is_auto_tiling();
// should be called last or at end of editor update frame
// since this relies on all the tile data all ready present
// in the world to query and solve for open tile connections
void editor_process_auto_tiling();
// adding any tile that needs to go through auto tiling must use this
// adding manually to the auto_tile_queue will not give you the correct
// tile connections
void editor_add_auto_tile(V2i tile);

void editor_set_camera_tile(V2i tile);

void editor_remove_switch_link(s32 index);
void editor_add_switch_link(Switch_Link switch_link);
void editor_clone_switch_link(s32 index);
void editor_update_switch_link(Switch_Link before, Switch_Link after, s32 index);

void editor_brush_set(Asset_Object_ID id);
b32 editor_brush_is_selected(Asset_Resource* resource);
b32 editor_brush_select(Asset_Resource* resource);
void editor_brush_select_next(fixed Asset_Resource** resources);

b32 editor_do_brush_place(V2i tile);
b32 editor_do_brush_remove(V2i tile);
b32 editor_do_brush_place_range(V2i start, V2i end);
b32 editor_do_brush_remove_range(V2i start, V2i end);

b32 editor_do_brush_floodfill(V2i tile, Asset_Object_ID id);

b32 editor_do_brush_tile_place(V2i tile, Asset_Object_ID id);
b32 editor_do_brush_tile_remove(V2i tile, Editor_Brush_Mode mode);
b32 editor_do_brush_tile_place_range(V2i start, V2i end, Asset_Object_ID id);

void editor_adjust_level_stride(V2i before, V2i after);
void editor_set_level_size(V2i size);
void editor_set_elevation(s32 min, s32 max);

b32 editor_save_level();
// used to save to a `.editor.ipl` file, this can be used to
// load that file to do a edit test play without affecting
// the current level file. main reason to do this is to reuse
// the way levels are loaded in world_load() and do any quick 
// checks for anything broken in level serialization and 
// deserialization
b32 editor_save_temp_level();
b32 editor_load_level(const char* file_name, b32 flush_undos);
void editor_new_level(const char* name);
void editor_cleanup_temp_files();

#endif //EDITOR_H
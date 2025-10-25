#ifndef INPUT_H
#define INPUT_H

typedef s32 Input_Mod;
enum
{
    Input_Mod_None = 0,
    Input_Mod_Control = 1 << 0,
    Input_Mod_Shift = 1 << 1,
    Input_Mod_Alt = 1 << 2,
    Input_Mod_Gui = 1 << 3,
};

typedef struct Input_Binding
{
    b32 is_mouse_button;
    Input_Mod mod;
    CF_KeyButton key;
    CF_MouseButton mouse;
} Input_Binding;

typedef s32 Input_Multiselect_State;
enum
{
    Input_Multiselect_State_None,
    Input_Multiselect_State_Selecting,
    Input_Multiselect_State_Commit,
    Input_Multiselect_State_Finish,
};

typedef struct Input_Config
{
    dyna Input_Binding* move;
    dyna Input_Binding* move_up;
    dyna Input_Binding* move_down;
    dyna Input_Binding* move_left;
    dyna Input_Binding* move_right;
    dyna Input_Binding* fire;
} Input_Config;

#ifndef AIM_SENSITIVITY_MIN
#define AIM_SENSITIVITY_MIN (1.0f)
#endif

#ifndef AIM_SENSITIVITY_MAX
#define AIM_SENSITIVITY_MAX (50.0f)
#endif

typedef struct Controller_Input_Config
{
    b32 invert_left_stick_y;
    b32 invert_right_stick_y;
    
    CF_Aabb left_dead_zone;
    CF_Aabb right_dead_zone;
    
    f32 aim_sensitivity;
    
    CF_JoypadButton move_up;
    CF_JoypadButton move_down;
    CF_JoypadButton move_left;
    CF_JoypadButton move_right;
    CF_JoypadButton fire;
    
    //  @todo:  left and right trigger right now is used to select next and prev controllable units
    //          this needs to be remappable by the player
    //          so change CF_JoypadButton to include triggers
    
} Controller_Input_Config;

typedef struct Input
{
    CF_V2 screen_mouse;
    CF_V2 world_mouse;
    V2i tile_select;
    V2i tile_select_start;
    V2i tile_select_end;
    
    b32 is_holding_add_remove;
    b32 select;
    b32 move;
    b32 fire;
    b32 select_next;
    b32 select_prev;
    
    // keyboard/controller
    V2i prev_move_direction;
    V2i move_direction;
    
    CF_V2 aim_direction;
    
    CF_JoypadType controller_type;
    
    Input_Multiselect_State multiselect;
    mco_coro* multiselect_co;
} Input;

Input_Binding make_empty_binding();
Input_Binding make_key_binding(CF_KeyButton button, Input_Mod mod);
Input_Binding make_mouse_binding(CF_MouseButton button, Input_Mod mod);
fixed char* input_binding_to_string(Input_Binding binding);
Input_Binding input_binding_from_string(const char* string);
b32 input_binding_mod_check(Input_Mod mod);
b32 input_binding_down(Input_Binding binding);
b32 input_binding_just_pressed(Input_Binding binding);
b32 input_binding_just_released(Input_Binding binding);
b32 input_binding_list_down(dyna Input_Binding* bindings);
b32 input_binding_list_just_pressed(dyna Input_Binding* bindings);
b32 input_binding_list_just_released(dyna Input_Binding* bindings);
CF_KeyButton get_any_key();
CF_MouseButton get_any_mouse();

b32 input_config_save(const char** names, Input_Binding** binding_list, s32 count, const char* output_file);
b32 input_config_load(const char** names, Input_Binding** binding_list, s32 count, const char* input_file);

CF_V2 controller_axis_adjust_dead_zone(CF_V2 axis, CF_Aabb dead_zone);
fixed char* controller_button_to_string(CF_JoypadButton button);
CF_JoypadButton controller_button_from_string(const char* string);
CF_JoypadButton controller_get_any_button();
b32 controller_button_just_pressed(CF_JoypadButton button);
b32 controller_button_down(CF_JoypadButton button);
b32 controller_button_just_released(CF_JoypadButton button);

typedef s32 Controller_Joypad_Axis;
enum
{
    Controller_Joypad_Axis_Left,
    Controller_Joypad_Axis_Right,
    Controller_Joypad_Axis_Trigger,
};

CF_V2 controller_get_axis(Controller_Joypad_Axis type);
CF_V2 controller_get_axis_prev(Controller_Joypad_Axis type);
V2i controller_get_digital_input_from_axis(Controller_Joypad_Axis type, b32 repeat, f32 threshold);

b32 controller_config_save(const char** names, CF_JoypadButton* buttons, s32 count, 
                           CF_Aabb left_dead_zone, CF_Aabb right_dead_zone, f32 aim_sensitivity,
                           const char* output_file);
b32 controller_config_load(const char** names, CF_JoypadButton** buttons, s32 count, 
                           CF_Aabb* left_dead_zone, CF_Aabb* right_dead_zone, f32* aim_sensitivity,
                           const char* input_file);

CF_JoypadType controller_get_type();

#endif //INPUT_H
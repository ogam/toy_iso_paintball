#ifndef INPUT_H
#define INPUT_H

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

#endif //INPUT_H
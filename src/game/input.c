#include "game/input.h"

Input_Binding make_empty_binding()
{
    return (Input_Binding){
        .mouse = CF_MOUSE_BUTTON_COUNT,
        .key = CF_KEY_COUNT,
        .mod = Input_Mod_None
    };
}

Input_Binding make_key_binding(CF_KeyButton button, Input_Mod mod)
{
    return (Input_Binding){
        .key = button,
        .mod = mod
    };
}

Input_Binding make_mouse_binding(CF_MouseButton button, Input_Mod mod)
{
    return (Input_Binding){
        .is_mouse_button = true,
        .mouse = button,
        .mod = mod
    };
}

fixed char* input_binding_to_string(Input_Binding binding)
{
    fixed char* string = make_scratch_string(256);
    
    if (binding.mod)
    {
        if (binding.mod & Input_Mod_Control)
        {
            cf_string_fmt_append(string, "%s", "CTRL+");
        }
        if (binding.mod & Input_Mod_Shift)
        {
            cf_string_fmt_append(string, "%s", "SHIFT+");
        }
        if (binding.mod & Input_Mod_Alt)
        {
            cf_string_fmt_append(string, "%s", "ALT+");
        }
        if (binding.mod & Input_Mod_Gui)
        {
#if defined(_WIN32)
            cf_string_fmt_append(string, "%s", "WIN+");
#else
            cf_string_fmt_append(string, "%s", "CMD+");
#endif
        }
    }
    if (binding.is_mouse_button)
    {
        if (binding.mouse != CF_MOUSE_BUTTON_COUNT)
        {
            const char* button_name = cf_mouse_button_to_string(binding.mouse);
            if (CF_STRSTR(button_name, "CF_") == button_name)
            {
                cf_string_fmt_append(string, "%s", button_name + CF_STRLEN("CF_"));
            }
            else
            {
                cf_string_fmt_append(string, "%s", button_name);
            }
        }
        else
        {
            if (cf_string_len(string))
            {
                cf_string_pop(string);
            }
        }
    }
    else
    {
        if (binding.key != CF_KEY_COUNT)
        {
            const char* button_name = cf_key_button_to_string(binding.key);
            if (CF_STRSTR(button_name, "CF_KEY_") == button_name)
            {
                cf_string_fmt_append(string, "%s", button_name + + CF_STRLEN("CF_KEY_"));
            }
            else
            {
                cf_string_fmt_append(string, "%s", button_name);
            }
        }
        else
        {
            if (cf_string_len(string))
            {
                cf_string_pop(string);
            }
        }
    }
    
    if (cf_string_len(string) == 0)
    {
        cf_string_fmt(string, "%s", "None");
    }
    
    return string;
}

Input_Binding input_binding_from_string(const char* string)
{
    Input_Binding binding = make_empty_binding();
    
    if (CF_STRSTR(string, "MOUSE_BUTTON_"))
    {
        fixed char* name = make_scratch_string(256);
        cf_string_fmt(name, "CF_%s", string);
        
        for (s32 index = 0; index < CF_MOUSE_BUTTON_COUNT; ++index)
        {
            if (cf_string_equ(name, cf_mouse_button_to_string((CF_MouseButton)index)))
            {
                binding.is_mouse_button = true;
                binding.mouse = (CF_MouseButton)index;
                break;
            }
        }
    }
    else
    {
        fixed char* name = make_scratch_string(256);
        cf_string_fmt(name, "CF_KEY_%s", string);
        
        for (s32 index = 0; index < CF_KEY_COUNT; ++index)
        {
            if (cf_string_equ(name, cf_key_button_to_string((CF_KeyButton)index)))
            {
                binding.key = (CF_KeyButton)index;
                break;
            }
        }
    }
    
    return binding;
}

b32 input_binding_mod_check(Input_Mod mod)
{
    b32 is_mod = mod == Input_Mod_None;
    
    if (!is_mod)
    {
        is_mod = true;
        if ((mod & Input_Mod_Control) && !cf_key_ctrl())
        {
            is_mod = false;
        }
        if ((mod & Input_Mod_Shift) && !cf_key_shift())
        {
            is_mod = false;
        }
        if ((mod & Input_Mod_Alt) && !cf_key_alt())
        {
            is_mod = false;
        }
        if ((mod & Input_Mod_Gui) && !cf_key_gui())
        {
            is_mod = false;
        }
    }
    return is_mod;
}

b32 input_binding_down(Input_Binding binding)
{
    b32 down = false;
    
    if (!input_binding_mod_check(binding.mod))
    {
        return false;
    }
    
    if (binding.mouse != CF_MOUSE_BUTTON_COUNT && binding.is_mouse_button)
    {
        if (cf_mouse_down(binding.mouse))
        {
            down = true;
        }
    }
    else
    {
        if (binding.key != CF_KEY_COUNT && cf_key_down(binding.key))
        {
            down = true;
        }
    }
    
    return down;
}

b32 input_binding_just_pressed(Input_Binding binding)
{
    b32 just_pressed = false;
    if (!input_binding_mod_check(binding.mod))
    {
        return false;;
    }
    
    if (binding.mouse != CF_MOUSE_BUTTON_COUNT && binding.is_mouse_button)
    {
        if (cf_mouse_just_pressed(binding.mouse))
        {
            just_pressed = true;
        }
    }
    else
    {
        if (binding.key != CF_KEY_COUNT && cf_key_just_pressed(binding.key))
        {
            just_pressed = true;
        }
    }
    
    return just_pressed;
}

b32 input_binding_just_released(Input_Binding binding)
{
    b32 just_released = false;
    if (!input_binding_mod_check(binding.mod))
    {
        return false;
    }
    
    if (binding.mouse != CF_MOUSE_BUTTON_COUNT && binding.is_mouse_button)
    {
        if (cf_mouse_just_released(binding.mouse))
        {
            just_released = true;
        }
    }
    else
    {
        if (binding.key != CF_KEY_COUNT && cf_key_just_released(binding.key))
        {
            just_released = true;
        }
    }
    return just_released;
}

b32 input_binding_list_down(dyna Input_Binding* bindings)
{
    b32 down = false;
    
    for (s32 index = 0; index < cf_array_count(bindings); ++index)
    {
        if (input_binding_down(bindings[index]))
        {
            down = true;
            break;
        }
    }
    
    return down;
}

b32 input_binding_list_just_pressed(dyna Input_Binding* bindings)
{
    b32 just_pressed = false;
    
    for (s32 index = 0; index < cf_array_count(bindings); ++index)
    {
        if (input_binding_just_pressed(bindings[index]))
        {
            just_pressed = true;
            break;
        }
    }
    
    return just_pressed;
}

b32 input_binding_list_just_released(dyna Input_Binding* bindings)
{
    b32 just_released = false;
    
    for (s32 index = 0; index < cf_array_count(bindings); ++index)
    {
        if (input_binding_just_released(bindings[index]))
        {
            just_released = true;
            break;
        }
    }
    
    return just_released;
}

Input_Mod get_any_mod()
{
    Input_Mod mod = Input_Mod_None;
    if (cf_key_ctrl())
    {
        mod |= Input_Mod_Control;
    }
    if (cf_key_shift())
    {
        mod |= Input_Mod_Shift;
    }
    if (cf_key_alt())
    {
        mod |= Input_Mod_Alt;
    }
    if (cf_key_gui())
    {
        mod |= Input_Mod_Gui;
    }
    return mod;
}

CF_KeyButton get_any_key()
{
    CF_KeyButton button = CF_KEY_COUNT;
    
    for (s32 index = 0; index < CF_KEY_COUNT; ++index)
    {
        if (index == CF_KEY_LCTRL || index == CF_KEY_RCTRL ||
            index == CF_KEY_LSHIFT || index == CF_KEY_RSHIFT ||
            index == CF_KEY_LALT || index == CF_KEY_RALT ||
            index == CF_KEY_LGUI || index == CF_KEY_RGUI || 
            index == CF_KEY_ANY)
        {
            continue;
        }
        
        if (cf_key_just_pressed((CF_KeyButton)index))
        {
            button = (CF_KeyButton)index;
            break;
        }
    }
    
    return button;
}

CF_MouseButton get_any_mouse()
{
    CF_MouseButton button = CF_MOUSE_BUTTON_COUNT;
    
    for (s32 index = 0; index < CF_MOUSE_BUTTON_COUNT; ++index)
    {
        if (cf_mouse_just_pressed((CF_MouseButton)index))
        {
            button = (CF_MouseButton)index;
            break;
        }
    }
    
    return button;
}

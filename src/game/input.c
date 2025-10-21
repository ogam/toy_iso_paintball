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

b32 input_config_save(const char** names, Input_Binding** binding_list, s32 count, const char* output_file)
{
    mount_root_write_directory();
    
    b32 success = false;
    
    CF_JDoc doc = cf_make_json(NULL, 0);
    CF_JVal root = cf_json_object(doc);
    cf_json_set_root(doc, root);
    
    CF_JVal type_val = cf_json_from_string(doc, "input");
    CF_JVal bindings_map = cf_json_object(doc);
    
    cf_json_object_add(doc, root, "type", type_val);
    cf_json_object_add(doc, root, "binds", bindings_map);
    
    for (s32 index = 0; index < count; ++index)
    {
        dyna Input_Binding* bindings = binding_list[index];
        CF_JVal array = cf_json_array(doc);
        for (s32 binding_index = 0; binding_index < cf_array_count(bindings); ++binding_index)
        {
            Input_Binding* binding = bindings + binding_index;
            CF_JVal obj = cf_json_array_add_object(doc, array);
            
            if (binding->mod & Input_Mod_Shift)
            {
                cf_json_object_add_bool(doc, obj, "shift", true);
            }
            if (binding->mod & Input_Mod_Control)
            {
                cf_json_object_add_bool(doc, obj, "control", true);
            }
            if (binding->mod & Input_Mod_Alt)
            {
                cf_json_object_add_bool(doc, obj, "alt", true);
            }
            if (binding->mod & Input_Mod_Gui)
            {
                cf_json_object_add_bool(doc, obj, "gui", true);
            }
            
            cf_json_object_add_string(doc, obj, "bind", input_binding_to_string(*binding));
        }
        cf_json_object_add(doc, bindings_map, names[index], array);
    }
    
    CF_Result save_result = cf_json_to_file(doc, output_file);
    if (save_result.code == CF_RESULT_SUCCESS)
    {
        success = true;
    }
    
    cf_destroy_json(doc);
    
    dismount_root_directory();
    
    return success;
}

b32 input_config_load(const char** names, Input_Binding** binding_list, s32 count, const char* input_file)
{
    mount_root_read_directory();
    
    b32 success = false;
    
    CF_JDoc doc = cf_make_json_from_file(input_file);
    
    if (!doc.id)
    {
        goto JSON_LOAD_CONFIG_CLEANUP;
    }
    
    CF_JVal root = cf_json_get_root(doc);
    CF_JVal type_obj = cf_json_get(root, "type");
    b32 process_file = false;
    
    if (cf_json_is_string(type_obj))
    {
        const char* type_val = cf_json_get_string(type_obj);
        if (type_val && cf_string_equ(type_val, "input"))
        {
            process_file = true;
        }
    }
    
    if (!process_file)
    {
        printf("Failed to load input configs for \"%s\", invalid type\n", input_file);
        goto JSON_LOAD_CONFIG_CLEANUP;
    }
    
    CF_JVal binding_list_obj = cf_json_get(root, "binds");
    if (!cf_json_is_object(binding_list_obj))
    {
        printf("Failed to load input configs for \"%s\", bindings needs to be an object\n", input_file);
        goto JSON_LOAD_CONFIG_CLEANUP;
    }
    
    // walk map
    for (CF_JIter binding_list_it = cf_json_iter(binding_list_obj); 
         !cf_json_iter_done(binding_list_it); 
         binding_list_it = cf_json_iter_next(binding_list_it))
    {
        const char* key = cf_json_iter_key(binding_list_it);
        CF_JVal bindings_obj = cf_json_iter_val(binding_list_it);
        
        if (!cf_json_is_array(bindings_obj))
        {
            continue;
        }
        
        for (s32 index = 0; index < count; ++index)
        {
            if (cf_string_equ(names[index], key))
            {
                dyna Input_Binding* bindings = binding_list[index];
                cf_array_clear(bindings);
                // walk through each binding
                for (CF_JIter binding_it = cf_json_iter(bindings_obj); 
                     !cf_json_iter_done(binding_it); 
                     binding_it = cf_json_iter_next(binding_it))
                {
                    CF_JVal binding_obj = cf_json_iter_val(binding_it);
                    const char* bind_val = JSON_GET_STRING(binding_obj, "bind");
                    Input_Binding binding = input_binding_from_string(bind_val);
                    if (JSON_GET_BOOL(binding_obj, "shift"))
                    {
                        binding.mod |= Input_Mod_Shift;
                    }
                    if (JSON_GET_BOOL(binding_obj, "control"))
                    {
                        binding.mod |= Input_Mod_Control;
                    }
                    if (JSON_GET_BOOL(binding_obj, "alt"))
                    {
                        binding.mod |= Input_Mod_Alt;
                    }
                    if (JSON_GET_BOOL(binding_obj, "gui"))
                    {
                        binding.mod |= Input_Mod_Gui;
                    }
                    
                    cf_array_push(bindings, binding);
                    // limit of 2
                    if (cf_array_count(bindings) > 2)
                    {
                        break;
                    }
                }
                
                break;
            }
        }
        
    }
    
    success = true;
    editor_apply_temp_input_config();
    printf("Loaded input configs from %s\n", input_file);
    
    JSON_LOAD_CONFIG_CLEANUP:
    dismount_root_directory();
    cf_destroy_json(doc);
    
    return success;
}
#include "game/ui.h"

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4305)

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

#pragma warning(pop)

// stb_textedit

#define UI_MOD_KEY_CTRL (1 << 30)
#define UI_MOD_KEY_SHIFT (1 << 31)
#define UI_MOD_KEY_ALL (UI_MOD_KEY_CTRL | UI_MOD_KEY_SHIFT)

s32 key_to_text(s32 k)
{
    CF_KeyButton key = k & ~UI_MOD_KEY_ALL;
    return key;
}

#define STB_TEXTEDIT_STRING char
#define STB_TEXTEDIT_STRINGLEN(obj) (cf_string_len(obj))
// only using single line mode can ignore this
#define STB_TEXTEDIT_LAYOUTROW(r,obj,n)
#define STB_TEXTEDIT_GETWIDTH(obj,n,i) (string_get_width(obj, n, i))
#define STB_TEXTEDIT_KEYTOTEXT(k) (key_to_text(k))
#define STB_TEXTEDIT_GETCHAR(obj,i) (string_get_index(obj, i))
#define STB_TEXTEDIT_NEWLINE '\n'
#define STB_TEXTEDIT_DELETECHARS(obj,i,n) (string_delete_range(obj, i, n))
#define STB_TEXTEDIT_INSERTCHARS(obj,i,c,n) (string_insert_range(obj, i, c, n))
#define STB_TEXTEDIT_K_SHIFT (UI_MOD_KEY_SHIFT)
#define STB_TEXTEDIT_K_LEFT (CF_KEY_LEFT)
#define STB_TEXTEDIT_K_RIGHT (CF_KEY_RIGHT)
#define STB_TEXTEDIT_K_UP (CF_KEY_UP)
#define STB_TEXTEDIT_K_DOWN (CF_KEY_DOWN)
#define STB_TEXTEDIT_K_PGUP (CF_KEY_PAGEUP)
#define STB_TEXTEDIT_K_PGDOWN (CF_KEY_PAGEDOWN)
#define STB_TEXTEDIT_K_LINESTART (CF_KEY_HOME)
#define STB_TEXTEDIT_K_LINEEND (CF_KEY_END)
#define STB_TEXTEDIT_K_TEXTSTART (CF_KEY_HOME | UI_MOD_KEY_CTRL)
#define STB_TEXTEDIT_K_TEXTEND (CF_KEY_END | UI_MOD_KEY_CTRL)
#define STB_TEXTEDIT_K_DELETE (CF_KEY_DELETE)
#define STB_TEXTEDIT_K_BACKSPACE (CF_KEY_BACKSPACE)
#define STB_TEXTEDIT_K_UNDO (CF_KEY_Z | UI_MOD_KEY_CTRL)
#define STB_TEXTEDIT_K_REDO (CF_KEY_R | UI_MOD_KEY_CTRL)
#define STB_TEXTEDIT_IS_SPACE(ch) (string_is_space(ch))
#define STB_TEXTEDIT_K_WORDLEFT (CF_KEY_LEFT | UI_MOD_KEY_CTRL)
#define STB_TEXTEDIT_K_WORDRIGHT (CF_KEY_RIGHT | UI_MOD_KEY_CTRL)

#pragma warning(push)
#pragma warning(disable: 4700)

#define  STB_TEXTEDIT_IMPLEMENTATION
#include "stb/stb_textedit.h"

#pragma warning(pop)

// stb_textedit

fixed char* ui_input_text_string_alloc(const char* src);
void ui_input_text_string_free(fixed char* text);
UI_Input_Text_State* ui_get_input_text_state(fixed char* text);
UI_Input_Text_State* ui_get_input_s32_state(s32* v);
UI_Input_Text_State* ui_get_input_f32_state(f32* v);

void clay_handle_errors(Clay_ErrorData errorData)
{
    printf("Handle Clay Errors: %s\r\n", errorData.errorText.chars);
}

Clay_Dimensions clay_measure_text(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData)
{
    UI* ui = s_app->ui;
    
    UNUSED(userData);
    cf_push_font(ui->fonts[config->fontId]);
    cf_push_font_size(config->fontSize);
    
    CF_V2 text_size = cf_text_size(text.chars, text.length);
    
    cf_pop_font_size();
    cf_pop_font();
    return (Clay_Dimensions){.width = text_size.x, .height = text_size.y};
}

UI_STYLE_FUNCS(Clay_Color, idle_color);
UI_STYLE_FUNCS(Clay_Color, hover_color);
UI_STYLE_FUNCS(Clay_Color, down_color);
UI_STYLE_FUNCS(Clay_Color, select_color);
UI_STYLE_FUNCS(Clay_Color, text_color);
UI_STYLE_FUNCS(Clay_Color, text_select_color);
UI_STYLE_FUNCS(Clay_Color, text_cursor_color);
UI_STYLE_FUNCS(Clay_Color, empty_text_color);
UI_STYLE_FUNCS(Clay_Color, border_color);
UI_STYLE_FUNCS(UI_Layer_Sprite, background_sprite);
UI_STYLE_FUNCS(UI_Layer_Sprite, foreground_sprite);
UI_STYLE_FUNCS(b32, interactable);
UI_STYLE_FUNCS(b32, digital_input);
UI_STYLE_FUNCS(f32, corner_radius);
UI_STYLE_FUNCS(f32, font_size);

void ui_style_reset()
{
    UI_Style* style = &s_app->ui->style;
    
    cf_array_clear(style->idle_color);
    cf_array_clear(style->hover_color);
    cf_array_clear(style->down_color);
    cf_array_clear(style->select_color);
    cf_array_clear(style->text_color);
    cf_array_clear(style->text_select_color);
    cf_array_clear(style->text_cursor_color);
    cf_array_clear(style->empty_text_color);
    cf_array_clear(style->border_color);
    cf_array_clear(style->background_sprite);
    cf_array_clear(style->foreground_sprite);
    cf_array_clear(style->interactable);
    cf_array_clear(style->digital_input);
    cf_array_clear(style->corner_radius);
    cf_array_clear(style->font_size);
    
    ui_push_idle_color((Clay_Color){ 100, 100, 100, 255 });
    ui_push_hover_color((Clay_Color){ 180, 150, 60, 255 });
    ui_push_down_color((Clay_Color){ 180, 255, 60, 255 });
    ui_push_select_color((Clay_Color){ 120, 120, 120, 255 });
    ui_push_text_color((Clay_Color){ 255, 255, 255, 255 });
    ui_push_text_select_color((Clay_Color){ 180, 100, 180, 200 });
    ui_push_text_cursor_color((Clay_Color){ 255, 255, 255, 255 });
    ui_push_empty_text_color((Clay_Color){ 255, 255, 255, 200 });
    ui_push_border_color((Clay_Color){ 255, 255, 255, 255 });
    ui_push_background_sprite((UI_Layer_Sprite){ .type = UI_Sprite_Params_Type_None });
    ui_push_foreground_sprite((UI_Layer_Sprite){ .type = UI_Sprite_Params_Type_None });
    ui_push_interactable(true);
    ui_push_digital_input(true);
    ui_push_corner_radius(0);
    ui_push_font_size(24);
}

void ui_set_fonts(const char** fonts, s32 count)
{
    UI* ui = s_app->ui;
    
    cf_array_clear(ui->fonts);
    for (s32 index = 0; index < count; ++index)
    {
        cf_array_push(ui->fonts, fonts[index]);
    }
}

void ui_init()
{
    UI* ui = cf_calloc(sizeof(UI), 1);
    s_app->ui = ui;
    
    cf_array_fit(ui->input_text_states, 32);
    cf_array_fit(ui->fonts, 8);
    
    // default font that CF has loaded
    cf_array_push(ui->fonts, "Calibri");
    cf_array_fit(ui->navigation_nodes, 1024);
    
    u64 clayRequiredMemory = Clay_MinMemorySize();
    Clay_Arena clayMemory = Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory, cf_alloc(clayRequiredMemory));
    Clay_Initialize(clayMemory, 
                    (Clay_Dimensions){.width = (f32)cf_app_get_width(), .height = (f32)cf_app_get_height()}, 
                    (Clay_ErrorHandler){clay_handle_errors});
    Clay_SetMeasureTextFunction(clay_measure_text, NULL);
}

b32 ui_stb_key_down(CF_KeyButton key)
{
    return cf_key_just_pressed(key) || cf_key_repeating(key);
}

void ui_update_input()
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    {
        input->mouse.x = (f32)cf_mouse_x();
        input->mouse.y = (f32)cf_mouse_y();
        input->mouse_wheel = cf_mouse_wheel_motion();
        input->mouse_down = cf_mouse_down(CF_MOUSE_BUTTON_LEFT);
        input->mouse_press = cf_mouse_just_pressed(CF_MOUSE_BUTTON_LEFT);
        input->mouse_up = !cf_mouse_down(CF_MOUSE_BUTTON_LEFT);
        input->mouse_release = cf_mouse_just_released(CF_MOUSE_BUTTON_LEFT);
        
        b32 keyboard_accept = cf_key_just_pressed(CF_KEY_RETURN) || cf_key_just_pressed(CF_KEY_RETURN2);
        b32 controller_accept = controller_button_just_pressed(CF_JOYPAD_BUTTON_A);
        
        b32 keyboard_back = cf_key_just_pressed(CF_KEY_ESCAPE);
        b32 controller_back = controller_button_just_pressed(CF_JOYPAD_BUTTON_B);
        
        CF_V2 left_axis = controller_get_axis(Controller_Joypad_Axis_Left);
        
        V2i direction = v2i();
        direction.y += cf_key_just_pressed(CF_KEY_UP) || cf_key_just_pressed(CF_KEY_W);
        direction.y += controller_button_just_pressed(CF_JOYPAD_BUTTON_DPAD_UP) || left_axis.y > 0.8f;
        
        direction.y -= cf_key_just_pressed(CF_KEY_DOWN) || cf_key_just_pressed(CF_KEY_S);
        direction.y -= controller_button_just_pressed(CF_JOYPAD_BUTTON_DPAD_DOWN) || left_axis.y < -0.8f;
        
        direction.x += cf_key_just_pressed(CF_KEY_RIGHT) || cf_key_just_pressed(CF_KEY_D);
        direction.x += controller_button_just_pressed(CF_JOYPAD_BUTTON_DPAD_RIGHT) || left_axis.x > 0.8f;
        
        direction.x -= cf_key_just_pressed(CF_KEY_LEFT) || cf_key_just_pressed(CF_KEY_A);
        direction.x -= controller_button_just_pressed(CF_JOYPAD_BUTTON_DPAD_LEFT) || left_axis.x < -0.8f;
        
        direction = v2i_sign(direction);
        
        input->back_pressed = keyboard_back || controller_back;
        input->accept_pressed = keyboard_accept || controller_accept;
        input->direction = direction;
        
        //  @todo:  on screen keyboard or whatever
        input->text_input_accept = keyboard_accept;
        
        cf_input_text_get_buffer(&input->text);
        cf_input_text_clear();
        
        s32 mod = 0;
        if (cf_key_ctrl())
        {
            mod |= UI_MOD_KEY_CTRL;
        }
        if (cf_key_shift())
        {
            mod |= UI_MOD_KEY_SHIFT;
        }
        
        input->stb_inputs[_STB_TEXTEDIT_K_LEFT]      = ui_stb_key_down(CF_KEY_LEFT) ? CF_KEY_LEFT | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_RIGHT]     = ui_stb_key_down(CF_KEY_RIGHT) ? CF_KEY_RIGHT | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_UP]        = ui_stb_key_down(CF_KEY_UP) ? CF_KEY_UP | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_DOWN]      = ui_stb_key_down(CF_KEY_DOWN) ? CF_KEY_DOWN | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_PGUP]      = ui_stb_key_down(CF_KEY_PAGEUP) ? CF_KEY_PAGEUP | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_PGDOWN]    = ui_stb_key_down(CF_KEY_PAGEDOWN) ? CF_KEY_PAGEDOWN | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_LINESTART] = ui_stb_key_down(CF_KEY_HOME) ? CF_KEY_HOME | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_LINEEND]   = ui_stb_key_down(CF_KEY_END) ? CF_KEY_END | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_DELETE]    = ui_stb_key_down(CF_KEY_DELETE) ? CF_KEY_DELETE | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_BACKSPACE] = ui_stb_key_down(CF_KEY_BACKSPACE) ? CF_KEY_BACKSPACE | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_UNDO]      = ui_stb_key_down(CF_KEY_Z) && cf_key_ctrl() ? CF_KEY_Z | mod : 0;
        input->stb_inputs[_STB_TEXTEDIT_K_REDO]      = ui_stb_key_down(CF_KEY_Y) && cf_key_ctrl() ? CF_KEY_Y | mod : 0;
        
        input->do_paste           = cf_key_just_pressed(CF_KEY_V) && cf_key_ctrl();
        input->do_copy            = cf_key_just_pressed(CF_KEY_C) && cf_key_ctrl();
        input->do_cut             = cf_key_just_pressed(CF_KEY_X) && cf_key_ctrl();
        input->do_delete_forward  = cf_key_just_pressed(CF_KEY_DELETE) && cf_key_ctrl();
        input->do_delete_backward = cf_key_just_pressed(CF_KEY_BACKSPACE) && cf_key_ctrl();
        input->do_select_all      = cf_key_just_pressed(CF_KEY_A) && cf_key_ctrl();
        
        CF_V2 mouse_motion = cf_v2(cf_mouse_motion_x(), cf_mouse_motion_y());
        
        if (get_any_key() != CF_KEY_COUNT || controller_get_any_button() != CF_JOYPAD_BUTTON_COUNT)
        {
            input->is_digital_input = true;
        }
        else if (cf_len_sq(mouse_motion))
        {
            input->is_digital_input = false;
        }
    }
    
    // update clay inputs
    {
        Clay_SetPointerState((Clay_Vector2){ .x = input->mouse.x, .y = input->mouse.y }, input->mouse_down);
        Clay_UpdateScrollContainers(true, (Clay_Vector2){.x = 0, .y = input->mouse_wheel}, CF_DELTA_TIME);
    }
}

void ui_update()
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    // update clay screen size
    {
        Clay_Dimensions dim = (Clay_Dimensions){.width = (f32)s_app->w, .height = (f32)s_app->h};
        Clay_SetLayoutDimensions(dim);
    }
    
    ui_update_input();
}

void ui_draw()
{
    UI* ui = s_app->ui;
    
    dyna const char** fonts = ui->fonts;
    
    s32 w = s_app->w;
    s32 h = s_app->h;
    
    cf_draw_push();
    cf_draw_translate(w * -0.5f, h * -0.5f);
    
    fixed char* buffer = make_scratch_string(256);
    
    Clay_RenderCommandArray commands = ui->commands;
    for (s32 command_index = 0; command_index < commands.length; ++command_index)
    {
        Clay_RenderCommand* command = commands.internalArray + command_index;
        f32 bb_x = command->boundingBox.x;
        f32 bb_y = h - command->boundingBox.y;
        CF_Aabb aabb = cf_make_aabb_from_top_left(cf_v2(bb_x, bb_y), command->boundingBox.width, command->boundingBox.height);
        
        switch (command->commandType)
        {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
            {
                Clay_RectangleRenderData* rectangle = &command->renderData.rectangle;
                
                f32 chubbiness = 0.0f;
                f32* corners = (f32*)&rectangle->cornerRadius;
                for (s32 index = 0; index < sizeof(rectangle->cornerRadius) / sizeof(f32); ++index)
                {
                    chubbiness = cf_max(chubbiness, corners[index]);
                }
                
                CF_Color background_color = clay_color_to_color(rectangle->backgroundColor);
                cf_draw_push_color(background_color);
                cf_draw_box_fill(aabb, chubbiness);
                cf_draw_pop_color();
            }
            break;
            case CLAY_RENDER_COMMAND_TYPE_BORDER:
            {
                Clay_BorderRenderData* border = &command->renderData.border;
                CF_Color color = clay_color_to_color(border->color);
                
                // majority of the cases we'll be in this upper portion
                // where all edges and radii are the same
                
                // all borders are same draw aabb
                if (border->width.left == border->width.right && 
                    border->width.top == border->width.bottom &&
                    border->width.left == border->width.top)
                {
                    f32 thickness = (f32)border->width.left;
                    f32 chubbiness = 0.0f;
                    // no corner radius
                    if (border->cornerRadius.topLeft == 0.0f &&
                        border->cornerRadius.bottomLeft == 0.0f &&
                        border->cornerRadius.topRight == 0.0f &&
                        border->cornerRadius.bottomRight == 0.0f)
                    {
                        cf_draw_push_color(color);
                        cf_draw_box(aabb, thickness, chubbiness);
                        cf_draw_pop_color();
                    }
                    else
                    {
                        // all corner radius are the same
                        if (f32_is_zero(border->cornerRadius.topLeft - border->cornerRadius.bottomRight) && f32_is_zero(border->cornerRadius.topLeft - border->cornerRadius.topRight) && f32_is_zero(border->cornerRadius.topLeft - border->cornerRadius.bottomLeft))
                        {
                            chubbiness = border->cornerRadius.topLeft;
                            cf_draw_push_color(color);
                            cf_draw_box(aabb, thickness, chubbiness);
                            cf_draw_pop_color();
                        }
                        else
                        {
                            // draw each corner..
                            {
                                CF_V2 positions[] =
                                {
                                    cf_top_left(aabb),
                                    aabb.max,
                                    aabb.min,
                                    cf_bottom_right(aabb)
                                };
                                
                                f32 corners[] =
                                {
                                    border->cornerRadius.topLeft,
                                    border->cornerRadius.topRight,
                                    border->cornerRadius.bottomLeft,
                                    border->cornerRadius.bottomRight,
                                };
                                
                                f32 thickness[] = 
                                {
                                    (f32)cf_max(border->width.left, border->width.top),
                                    (f32)cf_max(border->width.right, border->width.top),
                                    (f32)cf_max(border->width.left, border->width.bottom),
                                    (f32)cf_max(border->width.right, border->width.bottom),
                                };
                                
                                //  @todo:  does this need shift the edges out slightly?
                                CF_V2 center = cf_center(aabb);
                                for (s32 index = 0; index < CF_ARRAY_SIZE(corners); ++index)
                                {
                                    f32 r = corners[index];
                                    f32 t = thickness[index];
                                    CF_V2 p = positions[index];
                                    if (!f32_is_zero(r))
                                    {
                                        CF_V2 dir = cf_sub_v2(p, center);
                                        dir = cf_sign_v2(dir);
                                        CF_V2 c0 = cf_add_v2(p, cf_mul_v2_f(dir, r * 0.5f));
                                        CF_V2 a = p;
                                        CF_V2 b = p;
                                        a.x -= dir.x * r;
                                        b.y -= dir.y * r;
                                        
                                        cf_draw_bezier_line(a, c0, b, 4, t);
                                    }
                                }
                            }
                            
                            cf_draw_box(aabb, thickness, 0.0f);
                        }
                    }
                }
                else
                {
                    {
                        CF_V2 positions[] =
                        {
                            cf_top_left(aabb),
                            aabb.max,
                            aabb.min,
                            cf_bottom_right(aabb)
                        };
                        
                        f32 corners[] =
                        {
                            border->cornerRadius.topLeft,
                            border->cornerRadius.topRight,
                            border->cornerRadius.bottomLeft,
                            border->cornerRadius.bottomRight,
                        };
                        
                        f32 thickness[] = 
                        {
                            (f32)cf_max(border->width.left, border->width.top),
                            (f32)cf_max(border->width.right, border->width.top),
                            (f32)cf_max(border->width.left, border->width.bottom),
                            (f32)cf_max(border->width.right, border->width.bottom),
                        };
                        
                        //  @todo:  does this need shift the edges out slightly?
                        CF_V2 center = cf_center(aabb);
                        for (s32 index = 0; index < CF_ARRAY_SIZE(corners); ++index)
                        {
                            f32 r = corners[index];
                            f32 t = thickness[index];
                            CF_V2 p = positions[index];
                            if (!f32_is_zero(r))
                            {
                                CF_V2 dir = cf_sub_v2(p, center);
                                dir = cf_sign_v2(dir);
                                CF_V2 c0 = cf_add_v2(p, cf_mul_v2_f(dir, r * 0.5f));
                                CF_V2 a = p;
                                CF_V2 b = p;
                                a.x -= dir.x * r;
                                b.y -= dir.y * r;
                                
                                cf_draw_bezier_line(a, c0, b, 4, t);
                            }
                        }
                    }
                    
                    if (border->width.left)
                    {
                        // left vertical line
                        CF_V2 p0 = aabb.min;
                        CF_V2 p1 = cf_top_left(aabb);
                        cf_draw_line(p0, p1, border->width.left);
                    }
                    if (border->width.right)
                    {
                        // right vertical line
                        CF_V2 p0 = cf_bottom_right(aabb);
                        CF_V2 p1 = aabb.max;
                        cf_draw_line(p0, p1, border->width.right);
                    }
                    if (border->width.top)
                    {
                        // top horizontal line
                        CF_V2 p0 = cf_top_left(aabb);
                        CF_V2 p1 = aabb.max;
                        cf_draw_line(p0, p1, border->width.top);
                    }
                    if (border->width.bottom)
                    {
                        // bottom horizontal line
                        CF_V2 p0 = aabb.min;
                        CF_V2 p1 = cf_bottom_right(aabb);
                        cf_draw_line(p0, p1, border->width.bottom);
                    }
                }
            }
            break;
            case CLAY_RENDER_COMMAND_TYPE_TEXT:
            {
                Clay_TextRenderData* text = &command->renderData.text;
                CF_V2 top_left = cf_top_left(aabb);
                
                CF_Color text_color = clay_color_to_color(text->textColor);
                cf_draw_push_color(text_color);
                cf_push_font(fonts[text->fontId]);
                cf_push_font_size(text->fontSize);
                cf_draw_text(text->stringContents.chars, top_left, text->stringContents.length);
                cf_pop_font_size();
                cf_pop_font();
                cf_draw_pop_color();
            }
            break;
            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
            {
                // couldn't figure out a sameline() with Clay so we get this instead
                // doing a floating layout is an option but that looked more complicated
                // to manage
                Clay_ImageRenderData* image = &command->renderData.image;
                // not supported
                //   image->backgroundColor
                //   image->cornerRadius
                UI_Sprite_Params* sprite_params = (UI_Sprite_Params*)image->imageData;
                
                CF_Sprite sprites[3] = 
                {
                    // background
                    { 0 },
                    // current
                    { 0 },
                    // foreground
                    { 0 },
                };
                
                const char* animations[3] = 
                {
                    NULL,
                    NULL,
                    NULL,
                };
                
                b32 is_9_slice[3] = 
                {
                    sprite_params->background.is_9_slice,
                    sprite_params->middle.is_9_slice,
                    sprite_params->foreground.is_9_slice
                };
                
                if (sprite_params->background.type == UI_Sprite_Params_Type_Image)
                {
                    sprites[0] = cf_make_sprite(sprite_params->background.name);
                    animations[0] = sprite_params->background.animation;
                }
                else if (sprite_params->background.type == UI_Sprite_Params_Type_Sprite)
                {
                    if (sprite_params->background.sprite)
                    {
                        sprites[0] = *sprite_params->background.sprite;
                    }
                }
                
                if (sprite_params->middle.type == UI_Sprite_Params_Type_Image)
                {
                    sprites[1] = cf_make_sprite(sprite_params->middle.name);
                    animations[1] = sprite_params->middle.animation;
                }
                else if (sprite_params->middle.type == UI_Sprite_Params_Type_Sprite)
                {
                    if (sprite_params->middle.sprite)
                    {
                        sprites[1] = *sprite_params->middle.sprite;
                    }
                }
                
                if (sprite_params->foreground.type == UI_Sprite_Params_Type_Image)
                {
                    sprites[2] = cf_make_sprite(sprite_params->foreground.name);
                    animations[2] = sprite_params->foreground.animation;
                }
                else if (sprite_params->foreground.type == UI_Sprite_Params_Type_Sprite)
                {
                    if (sprite_params->foreground.sprite)
                    {
                        sprites[2] = *sprite_params->foreground.sprite;
                    }
                }
                
                for (s32 index = 0; index < CF_ARRAY_SIZE(sprites); ++index)
                {
                    CF_Sprite sprite = sprites[index];
                    const char* animation = animations[index];
                    
                    if (sprite.name)
                    {
                        if (animation)
                        {
                            cf_sprite_play(&sprite, animation);
                        }
                        
                        CF_V2 scale = cf_v2(sprite_params->size.x / sprite.w, sprite_params->size.y / sprite.h);
                        scale = cf_mul_v2(scale, cf_sign_v2(sprite.scale));
                        CF_V2 pivot = cf_v2(0, 0);
                        if (cf_array_count(sprite.pivots))
                        {
                            pivot = sprite.pivots[sprite.frame_index + sprite.animation->frame_offset];
                        }
                        // aabb extents is 0 so adjust by the params size
                        CF_V2 p = cf_add_v2(aabb.min, cf_mul_v2(sprite_params->size, cf_v2(0.5f, -0.5f)));
                        pivot = cf_mul_v2(pivot, scale);
                        p = cf_sub_v2(p, pivot);
                        
                        sprite.scale = scale;
                        sprite.transform.p = p;
                        
                        if (is_9_slice[index])
                        {
                            cf_draw_sprite_9_slice_tiled(&sprite);
                        }
                        else
                        {
                            cf_draw_sprite(&sprite);
                        }
                    }
                }
            }
            break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
            {
                Clay_ClipRenderData* clip = &command->renderData.clip;
                CF_Rect rect = 
                {
                    .x = (s32)command->boundingBox.x,
                    .y = (s32)command->boundingBox.y,
                    .w = (s32)command->boundingBox.width,
                    .h = (s32)command->boundingBox.height,
                };
                
                cf_draw_push_scissor(rect);
            }
            break;
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
            {
                Clay_ClipRenderData* clip = &command->renderData.clip;
                UNUSED(clip);
                cf_draw_pop_scissor();
            }
            break;
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
            {
                Clay_CustomRenderData* custom_render_data = &command->renderData.custom;
                UI_Custom_Params* custom = custom_render_data->customData;
                if (custom->type == UI_Custom_Params_Type_Input_Text)
                {
                    CF_V2 position = cf_top_left(aabb);
                    const char* text = custom->input_text.text;
                    CF_Color text_color = custom->input_text.text_color;
                    CF_Color cursor_color = custom->input_text.text_cursor_color;
                    CF_Color select_color = custom->input_text.text_select_color;
                    f32 font_size = custom->input_text.font_size;
                    s32 select_start = custom->input_text.select_start;
                    s32 select_end = custom->input_text.select_end;
                    
                    cf_draw_push_color(text_color);
                    cf_push_font(fonts[custom->input_text.font_id]);
                    cf_push_font_size(font_size);
                    
                    {
                        CF_Rect prev_scissor = cf_draw_peek_scissor();
                        CF_Rect next_scissor = custom->input_text.scissor;
                        
                        V2i prev_min = v2i(.x = prev_scissor.x, .y = prev_scissor.y);
                        V2i prev_max = v2i(.x = prev_scissor.x + prev_scissor.w, .y = prev_scissor.y + prev_scissor.h);
                        
                        Aabbi prev_aabb = make_aabbi(prev_min, prev_max);
                        
                        V2i next_min = v2i(.x = next_scissor.x, .y = next_scissor.y);
                        V2i next_max = v2i(.x = next_scissor.x + next_scissor.w, .y = next_scissor.y + next_scissor.h);
                        Aabbi next_aabb = make_aabbi(next_min, next_max);
                        
                        if (prev_scissor.w != -1 && prev_scissor.h != -1)
                        {
                            next_aabb = aabbi_clamp(next_aabb, prev_aabb);
                            next_scissor.x = next_aabb.min.x;
                            next_scissor.y = next_aabb.min.y;
                            next_scissor.w = next_aabb.max.x - next_aabb.min.x;
                            next_scissor.h = next_aabb.max.y - next_aabb.min.y;
                        }
                        
                        cf_draw_push_scissor(next_scissor);
                    }
                    
                    position.x -= custom->input_text.x_offset;
                    
                    if (custom->input_text.cursor >= 0)
                    {
                        // draw cursor
                        {
                            f32 cursor_x_offset = cf_text_width(text, custom->input_text.cursor);
                            CF_V2 cursor_start = position;
                            CF_V2 cursor_end = position;
                            cursor_start.x += cursor_x_offset;
                            cursor_end.x += cursor_x_offset;
                            cursor_end.y -= font_size;
                            
                            cf_draw_push_color(cursor_color);
                            cf_draw_line(cursor_start, cursor_end, 1.0f);
                            cf_draw_pop_color();
                        }
                        
                        // draw selection
                        if (select_start - select_end != 0)
                        {
                            s32 select_min = cf_min(select_start, select_end);
                            s32 select_max = cf_max(select_start, select_end);
                            
                            f32 min_x_offset = cf_text_width(text, select_min);
                            f32 max_x_offset = cf_text_width(text, select_max);
                            CF_V2 min = cf_v2(position.x, position.y - font_size);
                            CF_V2 max = position;
                            min.x += min_x_offset;
                            max.x += max_x_offset;
                            
                            CF_Aabb select_aabb = cf_make_aabb(min, max);
                            
                            cf_draw_push_color(select_color);
                            cf_draw_box_fill(select_aabb, 0.0f);
                            cf_draw_pop_color();
                        }
                    }
                    
                    cf_draw_text(text, position, -1);
                    
                    cf_draw_pop_scissor();
                    
                    cf_pop_font_size();
                    cf_pop_font();
                    cf_draw_pop_color();
                }
                else if (custom->type == UI_Custom_Params_Type_Axis_Dead_Zone)
                {
                    CF_V2 position = cf_top_left(aabb);
                    CF_V2 axis = custom->axis_dead_zone.axis;
                    CF_Aabb dead_zone = custom->axis_dead_zone.dead_zone;
                    CF_V2 size = custom->axis_dead_zone.size;
                    CF_V2 half_extents = cf_mul_v2_f(size, 0.5f);
                    position.x += half_extents.x;
                    position.y -= half_extents.y;
                    
                    CF_V2 axis_position = cf_mul_v2(axis, half_extents);
                    axis_position = cf_add_v2(position, axis_position);
                    
                    CF_V2 top = position;
                    CF_V2 bottom = position;
                    CF_V2 left = position;
                    CF_V2 right = position;
                    
                    top.y += half_extents.y;
                    bottom.y -= half_extents.y;
                    left.x -= half_extents.x;
                    right.x += half_extents.x;
                    
                    CF_Aabb bounds_aabb = cf_make_aabb_center_half_extents(position, half_extents);
                    
                    // draw bounds and axes
                    {
                        cf_draw_push_color(cf_color_white());
                        cf_draw_box(bounds_aabb, 1.0f, 0.0f);
                        
                        cf_draw_line(top, bottom, 1.0f);
                        cf_draw_line(left, right, 1.0f);
                        
                        cf_draw_pop_color();
                    }
                    
                    CF_V2 min = cf_mul_v2(dead_zone.min, half_extents);
                    CF_V2 max = cf_mul_v2(dead_zone.max, half_extents);
                    
                    min = cf_add_v2(position, min);
                    max = cf_add_v2(position, max);
                    
                    CF_Aabb aabb = cf_make_aabb(min, max);
                    
                    cf_draw_push_color(cf_color_red());
                    cf_draw_box(aabb, 1.0f, 0.0f);
                    cf_draw_pop_color();
                    
                    cf_draw_push_color(cf_color_cyan());
                    cf_draw_circle_fill2(axis_position, 2.0f);
                    cf_draw_pop_color();
                    
                    cf_draw_push_color(cf_color_white());
                    cf_push_font_size(18.0f);
                    
                    cf_string_fmt(buffer, "%.2f, %.2f", axis.x, axis.y);
                    CF_V2 text_size = cf_text_size(buffer, -1);
                    CF_V2 text_position = cf_bottom_right(bounds_aabb);
                    text_position.x -= text_size.x;
                    text_position.y += text_size.y;
                    
                    cf_draw_text(buffer, text_position, -1);
                    
                    cf_pop_font_size();
                    cf_draw_pop_color();
                }
            }
            break;
            default:
            break;
        }
    }
    
#if 0
    // draw current navigation node paths
    if (ui->hover_id.id != 0)
    {
        for (s32 index = 0; index < cf_array_count(ui->navigation_nodes); ++index)
        {
            UI_Navigation_Node* node = ui->navigation_nodes + index;
            if (node->id.id == ui->hover_id.id)
            {
                CF_V2 position = cf_left(node->aabb);
                position.y = h -  position.y;
                if (node->up)
                {
                    CF_V2 up = cf_left(node->up->aabb);
                    up.y = h - up.y;
                    cf_draw_arrow(position, up, 1.0f, 3.0f);
                }
                if (node->down)
                {
                    CF_V2 down = cf_left(node->down->aabb);
                    down.y = h - down.y;
                    cf_draw_push_color(cf_color_red());
                    cf_draw_arrow(position, down, 1.0f, 3.0f);
                    cf_draw_pop_color();
                }
                if (node->left)
                {
                    CF_V2 left = cf_left(node->left->aabb);
                    left.y = h - left.y;
                    cf_draw_push_color(cf_color_magenta());
                    cf_draw_arrow(position, left, 1.0f, 3.0f);
                    cf_draw_pop_color();
                }
                if (node->right)
                {
                    CF_V2 right = cf_left(node->right->aabb);
                    right.y = h - right.y;
                    cf_draw_push_color(cf_color_blue());
                    cf_draw_arrow(position, right, 1.0f, 3.0f);
                    cf_draw_pop_color();
                }
                break;
            }
        }
    }
#endif
    
    cf_draw_pop();
}

void ui_begin()
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    ui_style_reset();
    
    ui->element_counter = 0;
    ui->hover_id = ui->next_hover_id;
    if (!input->is_digital_input)
    {
        ui->next_hover_id = (Clay_ElementId){ 0 };
    }
    ui->last_id = (Clay_ElementId){ 0 };
    
    for (s32 index = 0; index < cf_array_count(ui->input_text_states);)
    {
        UI_Input_Text_State* state = ui->input_text_states + index;
        state->ref_count--;
        if (state->ref_count < 0)
        {
            ui_input_text_string_free(state->src);
            // key is something else used to fetch an additional string
            if (state->text != state->key)
            {
                ui_input_text_string_free(state->text);
            }
            cf_array_del(ui->input_text_states, index);
            continue;
        }
        ++index;
    }
    
    cf_array_clear(ui->navigation_nodes);
    if (ui_is_modal_active())
    {
        ui_push_interactable(false);
    }
    
    Clay_BeginLayout();
}

void ui_end()
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    ui->modal_container_id = (Clay_ElementId){ 0 };
    
    if (ui_is_modal_active())
    {
        ui_pop_interactable();
    }
    
    ui_update_modal();
    
    if (input->mouse_up)
    {
        ui->down_id = (Clay_ElementId){ 0 };
    }
    
    if (input->mouse_press && ui->hover_id.id != ui->select_id.id)
    {
        ui->select_id = (Clay_ElementId){ 0 };
    }
    
    ui->commands = Clay_EndLayout();
    
    ui_process_navigation_nodes();
    
    if (ui_peek_digital_input())
    {
        ui_handle_digital_input();
    }
}

void ui_handle_digital_input()
{
    UI* ui = s_app->ui;
    UI_Input* ui_input = &ui->input;
    
    UI_Navigation_Node* nodes = ui->navigation_nodes;
    
    V2i direction = ui_input->direction;
    if (ui->hover_id.id == 0)
    {
        if (cf_array_count(nodes))
        {
            ui->hover_id = ui->navigation_nodes[0].id;
        }
    }
    
    UI_Navigation_Node* node = NULL;
    if (ui->hover_id.id == 0)
    {
        return;
    }
    
    for (s32 index = 0; index < cf_array_count(nodes); ++index)
    {
        if (nodes[index].id.id == ui->hover_id.id)
        {
            node = nodes + index;
            break;
        }
    }
    
    if (!node)
    {
        return;
    }
    
    if (direction.y > 0)
    {
        if (node->up)
        {
            ui->next_hover_id = node->up->id;
        }
    }
    else if (direction.y < 0)
    {
        if (node->down)
        {
            ui->next_hover_id = node->down->id;
        }
    }
    else if (direction.x > 0)
    {
        if (node->right)
        {
            ui->next_hover_id = node->right->id;
        }
    }
    else if (direction.x < 0)
    {
        if (node->left)
        {
            ui->next_hover_id = node->left->id;
        }
    }
    
    if (ui_input->accept_pressed)
    {
        ui->select_id = ui->hover_id;
        ui->hover_id = (Clay_ElementId){ 0 };
        ui->next_hover_id = (Clay_ElementId){ 0 };
    }
}

void ui_add_navigation_node(Clay_ElementId id)
{
    UI* ui = s_app->ui;
    Clay_ElementData data = Clay_GetElementData(id);
    if (data.found && ui_peek_interactable())
    {
        CF_Aabb bounds = cf_make_aabb_from_top_left(cf_v2(data.boundingBox.x, data.boundingBox.y), data.boundingBox.width, data.boundingBox.height);
        UI_Navigation_Node node = 
        {
            .id = id,
            .aabb = bounds,
        };
        cf_array_push(ui->navigation_nodes, node);
    }
}

void ui_process_navigation_nodes()
{
    UI* ui = s_app->ui;
    
    dyna UI_Navigation_Node* nodes = ui->navigation_nodes;
    
    f32 gap = 32;
    
    for (s32 index = 0; index < cf_array_count(nodes); ++index)
    {
        UI_Navigation_Node* node = nodes + index;
        CF_V2 position = cf_left(node->aabb);
        
        f32 closest_up = F32_MAX;
        f32 closest_down = F32_MAX;
        f32 closest_left = F32_MAX;
        f32 closest_right = F32_MAX;
        
        s32 closest_up_index = -1;
        s32 closest_down_index = -1;
        s32 closest_left_index = -1;
        s32 closest_right_index = -1;
        
        for (s32 next_index = 0; next_index < cf_array_count(nodes); ++next_index)
        {
            if (index == next_index)
            {
                continue;
            }
            UI_Navigation_Node* next_node = nodes + next_index;
            
            CF_V2 next_position = cf_left(next_node->aabb);
            CF_V2 delta = cf_sub_v2(next_position, position);
            f32 distance = cf_len_sq(delta);
            
            if (delta.y < -gap)
            {
                if (closest_up > distance)
                {
                    closest_up = distance;
                    closest_up_index = next_index;
                }
            }
            if (delta.y > gap)
            {
                if (closest_down > distance)
                {
                    closest_down = distance;
                    closest_down_index = next_index;
                }
            }
            if (delta.x < -gap)
            {
                if (closest_left > distance)
                {
                    closest_left = distance;
                    closest_left_index = next_index;
                }
            }
            if (delta.x > gap)
            {
                if (closest_right > distance)
                {
                    closest_right = distance;
                    closest_right_index = next_index;
                }
            }
        }
        
        if (closest_up_index != -1)
        {
            node->up = nodes + closest_up_index;
        }
        if (closest_down_index != -1)
        {
            node->down = nodes + closest_down_index;
        }
        if (closest_left_index != -1)
        {
            node->left = nodes + closest_left_index;
        }
        if (closest_right_index != -1)
        {
            node->right = nodes + closest_right_index;
        }
    }
}

CF_Color clay_color_to_color(Clay_Color c)
{
    return (CF_Color) {
        .r = c.r / 255.0f,
        .g = c.g / 255.0f,
        .b = c.b / 255.0f,
        .a = c.a / 255.0f,
    };
}

Clay_Color color_to_clay_color(CF_Color c)
{
    return (Clay_Color){
        .r = c.r * 255.0f,
        .g = c.g * 255.0f,
        .b = c.b * 255.0f,
        .a = c.a * 255.0f,
    };
}

Clay_ElementId ui_make_clay_id(const char* prefix, const char* suffix)
{
    UI* ui = s_app->ui;
    s32 allocation_size = 1024;
    char* buf = scratch_alloc(allocation_size);
    s32 id_text_length = (s32)CF_SNPRINTF(buf, allocation_size, "%s___%s__%d", prefix, suffix, ui->element_counter++);
    return Clay_GetElementId((Clay_String){.chars = buf, .length = id_text_length});
}

Clay_ElementId ui_make_clay_id_index(const char* prefix, s32 index)
{
    UI* ui = s_app->ui;
    s32 allocation_size = 1024;
    char* buf = scratch_alloc(allocation_size);
    s32 id_text_length = (s32)CF_SNPRINTF(buf, allocation_size, "%s___%d", prefix, index);
    return Clay_GetElementId((Clay_String){.chars = buf, .length = id_text_length});
}

void ui_update_element_selection(Clay_ElementId id)
{
    UI* ui = s_app->ui;
    
    if (!ui->input.is_digital_input)
    {
        if (ui_peek_interactable())
        {
            if (Clay_PointerOver(id))
            {
                if (ui->input.mouse_press)
                {
                    ui->select_id = (Clay_ElementId){ 0 };
                    ui->down_id = id;
                }
                else if (ui->input.mouse_release)
                {
                    if (ui->down_id.id == id.id)
                    {
                        ui->select_id = id;
                    }
                }
                else
                {
                    ui->next_hover_id = id;
                }
            }
            ui->last_id = id;
        }
    }
}

void ui_do_vtext(const char* fmt, va_list args)
{
    UI* ui = s_app->ui;
    s32 allocation_size = 1024;
    char* buf = scratch_alloc(allocation_size);
    s32 length = vsnprintf(buf, allocation_size, fmt, args);
    
    Clay_String clay_string = (Clay_String){.chars = buf, .length = length};
    Clay_Color text_color = ui_peek_text_color();
    s32 font_size = (s16)ui_peek_font_size();
    
    CLAY_TEXT(clay_string, CLAY_TEXT_CONFIG({ .fontSize = font_size, .textColor = text_color }));
}

void ui_do_text(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ui_do_vtext(fmt, args);
    va_end(args);
}

fixed char* ui_input_text_string_alloc(const char* src)
{
    UI* ui = s_app->ui;
    fixed char* s = NULL;
    cf_string_fit(s, (s32)CF_STRLEN(src) + 1);
    cf_string_append(s, src);
    cf_array_push(ui->input_text_strings, s);
    return s;
}

void ui_input_text_string_free(fixed char* text)
{
    cf_string_free(text);
}

UI_Input_Text_State* ui_get_input_text_state(fixed char* text)
{
    UI* ui = s_app->ui;
    UI_Input_Text_State* result = NULL;
    
    for (s32 index = 0; index < cf_array_count(ui->input_text_states); ++index)
    {
        if (ui->input_text_states[index].key == text)
        {
            ui->input_text_states[index].ref_count++;
            result = ui->input_text_states + index;
            break;
        }
    }
    if (!result)
    {
        UI_Input_Text_State new_state = 
        { 
            .src = ui_input_text_string_alloc(text),
            .text = text,
            .key = text,
            .ref_count = 1,
            .x_offset = 0.0f,
        };
        stb_textedit_initialize_state(&new_state.state, true);
        new_state.state.cursor = cf_string_count(text);
        
        cf_array_push(ui->input_text_states, new_state);
        result = &cf_array_last(ui->input_text_states);
    }
    
    return result;
}

UI_Input_Text_State* ui_get_input_s32_state(s32* v)
{
    UI* ui = s_app->ui;
    UI_Input_Text_State* result = NULL;
    
    char* text = (char*)v;
    
    for (s32 index = 0; index < cf_array_count(ui->input_text_states); ++index)
    {
        if (ui->input_text_states[index].key == v)
        {
            ui->input_text_states[index].ref_count++;
            result = ui->input_text_states + index;
            break;
        }
    }
    if (!result)
    {
        fixed char* buf = make_scratch_string(256);
        cf_string_fmt(buf, "%d", *v);
        
        UI_Input_Text_State new_state = 
        { 
            .src = ui_input_text_string_alloc(buf),
            .text = ui_input_text_string_alloc(buf),
            .key = v,
            .ref_count = 1,
            .x_offset = 0.0f,
        };
        stb_textedit_initialize_state(&new_state.state, true);
        new_state.state.cursor = cf_string_count(text);
        
        cf_array_push(ui->input_text_states, new_state);
        result = &cf_array_last(ui->input_text_states);
    }
    
    return result;
}

UI_Input_Text_State* ui_get_input_f32_state(f32* v)
{
    UI* ui = s_app->ui;
    UI_Input_Text_State* result = NULL;
    
    char* text = (char*)v;
    
    for (s32 index = 0; index < cf_array_count(ui->input_text_states); ++index)
    {
        if (ui->input_text_states[index].key == v)
        {
            ui->input_text_states[index].ref_count++;
            result = ui->input_text_states + index;
            break;
        }
    }
    if (!result)
    {
        fixed char* buf = make_scratch_string(256);
        cf_string_fmt(buf, "%.2f", *v);
        
        UI_Input_Text_State new_state = 
        { 
            .src = ui_input_text_string_alloc(buf),
            .text = ui_input_text_string_alloc(buf),
            .key = v,
            .ref_count = 1,
            .x_offset = 0.0f,
        };
        stb_textedit_initialize_state(&new_state.state, true);
        new_state.state.cursor = cf_string_count(text);
        
        cf_array_push(ui->input_text_states, new_state);
        result = &cf_array_last(ui->input_text_states);
    }
    
    return result;
}

Clay_ElementId ui_do_input_text_ex(UI_Input_Text_State* state, f32 width)
{
    UI* ui = s_app->ui;
    UI_Input* input = &s_app->ui->input;
    Clay_Color idle_color = ui_peek_idle_color();
    Clay_Color hover_color = ui_peek_hover_color();
    Clay_Color down_color = ui_peek_down_color();
    Clay_Color select_color = ui_peek_select_color();
    Clay_Color border_color = ui_peek_border_color();
    Clay_Color text_color = ui_peek_text_color();
    Clay_Color text_select_color = ui_peek_text_select_color();
    Clay_Color text_cursor_color = ui_peek_text_cursor_color();
    Clay_Color empty_text_color = ui_peek_empty_text_color();
    f32 font_size = ui_peek_font_size();
    b32 draw_cursor = false;
    
    f32 t = CF_FMODF((f32)CF_SECONDS, 1.0f);
    t = cf_circle_in_out(t);
    text_cursor_color.a = 255.0f * t;
    
    Clay_ElementId border_id = ui_make_clay_id("generic_border", "input_text");
    Clay_ElementId background_id = ui_make_clay_id("generic", "input_text_background");
    Clay_ElementId text_id = ui_make_clay_id("generic", "input_text");
    
    Clay_Color background_color = idle_color;
    
    STB_TexteditState* stb_state = &state->state;
    
    if (border_id.id == ui->hover_id.id)
    {
        background_color = hover_color;
    }
    if (border_id.id == ui->down_id.id)
    {
        background_color = down_color;
    }
    
    if (border_id.id == ui->select_id.id)
    {
        draw_cursor = true;
        background_color = select_color;
        
        for (s32 index = 0; index < input->text.len; ++index)
        {
            stb_textedit_key(state->text, stb_state, input->text.codepoints[index]);
        }
        
        if (input->do_cut)
        {
            s32 start = cf_min(stb_state->select_end, stb_state->select_start);
            s32 end = cf_max(stb_state->select_end, stb_state->select_start);
            const char* temp = string_slice(state->text, start, end);
            cf_clipboard_set(temp);
            
            stb_textedit_cut(state->text, stb_state);
        }
        if (input->do_paste)
        {
            char* raw_clipboard = cf_clipboard_get();
            s32 raw_clipboard_length = (s32)CF_STRLEN(raw_clipboard) + 1;
            s32* clipboard = scratch_alloc(raw_clipboard_length);
            s32 clipboard_length = 0;
            const char* s = raw_clipboard;
            while (*s)
            {
                s = cf_decode_UTF8(s, clipboard + clipboard_length++);
            }
            
            stb_textedit_paste(state->text, stb_state, clipboard, clipboard_length);
        }
        if (input->do_copy)
        {
            s32 start = cf_min(stb_state->select_end, stb_state->select_start);
            s32 end = cf_max(stb_state->select_end, stb_state->select_start);
            const char* temp = string_slice(state->text, start, end);
            
            cf_clipboard_set(temp);
        }
        if (input->do_delete_backward)
        {
            stb_textedit_key(state->text, stb_state, STB_TEXTEDIT_K_WORDLEFT | UI_MOD_KEY_SHIFT);
            stb_textedit_key(state->text, stb_state, STB_TEXTEDIT_K_DELETE);
        }
        if (input->do_delete_forward)
        {
            stb_textedit_key(state->text, stb_state, STB_TEXTEDIT_K_WORDRIGHT | UI_MOD_KEY_SHIFT);
            stb_textedit_key(state->text, stb_state, STB_TEXTEDIT_K_DELETE);
        }
        if (input->do_select_all)
        {
            stb_textedit_key(state->text, stb_state, STB_TEXTEDIT_K_LINESTART);
            stb_textedit_key(state->text, stb_state, STB_TEXTEDIT_K_LINEEND | UI_MOD_KEY_SHIFT);
        }
        
        for (s32 index = 0; index < _STB_TEXTEDIT_K_COUNT; ++index)
        {
            stb_textedit_key(state->text, stb_state, input->stb_inputs[index]);
        }
    }
    else
    {
        stb_state->cursor = cf_string_len(state->text);
    }
    
    UI_Custom_Params* params = scratch_alloc(sizeof(UI_Custom_Params));
    
    if (cf_string_len(state->text) == 0)
    {
        text_color = empty_text_color;
    }
    
    params->type = UI_Custom_Params_Type_Input_Text;
    params->input_text.text = state->text;
    params->input_text.cursor = draw_cursor ? stb_state->cursor : -1;
    params->input_text.select_start = stb_state->select_start;
    params->input_text.select_end = stb_state->select_end;
    params->input_text.font_size = font_size;
    params->input_text.font_id = 0;
    params->input_text.text_color = clay_color_to_color(text_color);
    params->input_text.text_select_color = clay_color_to_color(text_select_color);
    params->input_text.text_cursor_color = clay_color_to_color(text_cursor_color);
    
    CLAY(border_id, {
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .border = {
                 .color = border_color,
                 .width = CLAY_BORDER_ALL(1),
             },
             .layout = {
                 .sizing = {
                     .width = width <= 0.0f ? CLAY_SIZING_GROW(0) : CLAY_SIZING_FIXED(width),
                 },
             },
         })
    {
        Clay_ElementData data = Clay_GetElementData(border_id);
        cf_push_font(ui->fonts[params->input_text.font_id]);
        cf_push_font_size(params->input_text.font_size);
        CF_V2 text_size = cf_text_size(state->text, -1);
        f32 cursor_x_offset = cf_text_width(state->text, stb_state->cursor);
        // scrolling left
        if (state->x_offset > cursor_x_offset)
        {
            state->x_offset = cursor_x_offset;
        }
        else if (f32_is_zero(cursor_x_offset - text_size.x))
        {
            // scrolling towards the end
            state->x_offset = cf_max(text_size.x - data.boundingBox.width, 0);
        }
        else if (cursor_x_offset - state->x_offset > data.boundingBox.width)
        {
            // try to keep up with scrolling to the right
            f32 dx =  cursor_x_offset - state->x_offset - data.boundingBox.width;
            state->x_offset += dx;
        }
        else if (text_size.x < data.boundingBox.width)
        {
            // if clay decides to rebuild the layout it could flicker and cause
            // this x_offset to be calculated based off a 0 with bounding box
            // which can make this visually appear as an invalid value even
            // though there has been no change
            state->x_offset = 0;
        }
        params->input_text.x_offset = state->x_offset;
        
        cf_pop_font_size();
        cf_pop_font();
        
        params->input_text.scissor = (CF_Rect){
            .x = (s32)data.boundingBox.x,
            .y = (s32)data.boundingBox.y,
            .w = (s32)data.boundingBox.width,
            .h = (s32)data.boundingBox.height,
        };
        
        //  @note:  not using .clip here since the scrolling can cause Clay to crash with
        //          a lot of these ui elements with scrolling
        //          so we're going to manually do the scissor in the custom draw
        CLAY(background_id, {
                 .border = CLAY_BORDER_OUTSIDE(1),
                 .backgroundColor = background_color,
                 .layout = {
                     .sizing = {
                         .width = CLAY_SIZING_GROW(0),
                         .height = CLAY_SIZING_FIT( .min = font_size ),
                     },
                 },
             })
        {
            CLAY(text_id, { 
                     .custom = { .customData = params }
                 });
        }
    }
    
    ui_update_element_selection(border_id);
    return border_id;
}

b32 ui_do_input_text(fixed char* text, UI_Input_Text_Mode mode)
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    UI_Input_Text_State* state = ui_get_input_text_state(text);
    Clay_ElementId border_id = ui_do_input_text_ex(state, 0.0f);
    
    b32 text_changed = false;
    
    if (mode == UI_Input_Text_Mode_Default)
    {
        text_changed = !cf_string_equ(state->src, text);
    }
    else if (mode & UI_Input_Text_Mode_Sets_On_Return)
    {
        if (ui->select_id.id == border_id.id)
        {
            if (input->text_input_accept)
            {
                ui->select_id = (Clay_ElementId){ 0 };
                text_changed = true;
            }
        }
        else
        {
            string_set(text, state->src);
        }
    }
    
    return text_changed;
}

b32 ui_do_input_s32(s32* v, s32 min, s32 max)
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    *v = cf_clamp_int(*v, min, max);
    
    f32 width = 0.0f;
    {
        cf_push_font_size(ui_peek_font_size());
        char buf[256];
        CF_SNPRINTF(buf, sizeof(buf), "%d", -cf_abs_int(min));
        width = cf_max(width, cf_text_width(buf, -1));
        CF_SNPRINTF(buf, sizeof(buf), "%d", -cf_abs_int(max));
        width = cf_max(width, cf_text_width(buf, -1));
        ui_pop_font_size();
    }
    
    UI_Input_Text_State* state = ui_get_input_s32_state(v);
    Clay_ElementId border_id = ui_do_input_text_ex(state, width);
    
    b32 text_changed = false;
    if (ui->select_id.id == border_id.id)
    {
        if (input->text_input_accept)
        {
            s32 temp = 0;
            s32 tokens = sscanf(state->text, "%d", &temp);
            if (tokens)
            {
                temp = cf_clamp_int(temp, min, max);
                text_changed = *v != temp;
                *v = temp;
            }
            
            cf_string_fmt(state->text, "%d", *v);
            ui->select_id = (Clay_ElementId){ 0 };
        }
    }
    else
    {
        cf_string_fmt(state->text, "%d", *v);
    }
    
    return text_changed;
}

b32 ui_do_input_f32(f32* v, f32 min, f32 max)
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    
    *v = cf_clamp(*v, min, max);
    
    f32 width = 0.0f;
    {
        cf_push_font_size(ui_peek_font_size());
        char buf[256];
        CF_SNPRINTF(buf, sizeof(buf), "%.2f", -cf_abs(min));
        width = cf_max(width, cf_text_width(buf, -1));
        CF_SNPRINTF(buf, sizeof(buf), "%.2f", -cf_abs(max));
        width = cf_max(width, cf_text_width(buf, -1));
        ui_pop_font_size();
    }
    
    UI_Input_Text_State* state = ui_get_input_f32_state(v);
    Clay_ElementId border_id = ui_do_input_text_ex(state, width);
    
    b32 text_changed = false;
    if (ui->select_id.id == border_id.id)
    {
        if (input->text_input_accept)
        {
            f32 temp = 0;
            s32 tokens = sscanf(state->text, "%f", &temp);
            if (tokens)
            {
                temp = cf_clamp(temp, min, max);
                text_changed = *v != temp;
                *v = temp;
            }
            
            cf_string_fmt(state->text, "%.2f", *v);
            ui->select_id = (Clay_ElementId){ 0 };
        }
    }
    else
    {
        cf_string_fmt(state->text, "%.2f", *v);
    }
    
    return text_changed;
}


b32 ui_do_button(const char* text)
{
    UI* ui = s_app->ui;
    Clay_Color idle_color = ui_peek_idle_color();
    Clay_Color hover_color = ui_peek_hover_color();
    Clay_Color down_color = ui_peek_down_color();
    Clay_Color text_color = ui_peek_text_color();
    f32 font_size = ui_peek_font_size();
    
    Clay_ElementId id = ui_make_clay_id(text, "button");
    Clay_Color color = idle_color;
    if (id.id == ui->hover_id.id)
    {
        color = hover_color;
    }
    if (id.id == ui->down_id.id)
    {
        color = down_color;
    }
    
    Clay_String clay_string = (Clay_String){.chars = string_clone(text), .length = (s32)CF_STRLEN(text)};
    
    b32 clicked = false;
    
    CF_Aabb button_bounds = { 0 };
    
    CLAY(id, {
             .backgroundColor = color,
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
         })
    {
        ui_add_navigation_node(id);
        
        CLAY_TEXT(clay_string,
                  CLAY_TEXT_CONFIG({.fontSize = (s32)font_size, .textColor = text_color })
                  );
    }
    
    ui_update_element_selection(id);
    
    if (ui->select_id.id == id.id)
    {
        clicked = true;
        //  @note:  prevents button from being repeatedly hit
        ui->select_id = (Clay_ElementId){ 0 };
    }
    
    return clicked;
}

b32 ui_do_button_wide(const char* text)
{
    UI* ui = s_app->ui;
    Clay_Color idle_color = ui_peek_idle_color();
    Clay_Color hover_color = ui_peek_hover_color();
    Clay_Color down_color = ui_peek_down_color();
    Clay_Color text_color = ui_peek_text_color();
    f32 font_size = ui_peek_font_size();
    
    Clay_ElementId id = ui_make_clay_id(text, "button");
    Clay_Color color = idle_color;
    if (id.id == ui->hover_id.id)
    {
        color = hover_color;
    }
    if (id.id == ui->down_id.id)
    {
        color = down_color;
    }
    
    Clay_String clay_string = (Clay_String){.chars = string_clone(text), .length = (s32)CF_STRLEN(text)};
    
    b32 clicked = false;
    
    CLAY(id, {
             .backgroundColor = color,
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                 },
             },
         })
    {
        ui_add_navigation_node(id);
        CLAY_TEXT(clay_string,
                  CLAY_TEXT_CONFIG({.fontSize = (s32)font_size, .textColor = text_color })
                  );
    }
    
    ui_update_element_selection(id);
    
    if (ui->select_id.id == id.id)
    {
        clicked = true;
        //  @note:  prevents button from being repeatedly hit
        ui->select_id = (Clay_ElementId){ 0 };
    }
    
    return clicked;
}

b32 ui_do_checkbox(b32* value)
{
    UI* ui = s_app->ui;
    Clay_Color idle_color = ui_peek_idle_color();
    Clay_Color hover_color = ui_peek_hover_color();
    Clay_Color down_color = ui_peek_down_color();
    Clay_Color inner_color = { 255, 255, 255, 255 };
    f32 font_size = ui_peek_font_size();
    
    Clay_ElementId id = ui_make_clay_id("generic", "checkbox");
    Clay_ElementId inner_id = ui_make_clay_id("generic", "inner_checkbox");
    Clay_Color color = idle_color;
    if (id.id == ui->hover_id.id)
    {
        color = hover_color;
    }
    if (id.id == ui->down_id.id)
    {
        color = down_color;
    }
    
    b32 clicked = false;
    
    CLAY(id, {
             .backgroundColor = color,
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_FIXED(font_size),
                     .height = CLAY_SIZING_FIXED(font_size),
                 },
                 .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_CENTER,
                 },
             },
         })
    {
        ui_add_navigation_node(id);
        if (*value == false)
        {
            inner_color = color;
        }
        
        CLAY(inner_id, {
                 .backgroundColor = inner_color,
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .sizing = {
                         .width = CLAY_SIZING_FIXED(font_size * 0.5f),
                         .height = CLAY_SIZING_FIXED(font_size * 0.5f),
                     },
                 },
             })
        {
        }
    }
    
    ui_update_element_selection(id);
    
    if (ui->select_id.id == id.id)
    {
        clicked = true;
        //  @note:  prevents button from being repeatedly hit
        ui->select_id = (Clay_ElementId){ 0 };
        *value = !*value;
    }
    
    return clicked;
}

b32 ui_do_checkbox_bit(b32* value, b32 bit)
{
    b32 temp = *value & bit;
    b32 clicked = ui_do_checkbox(&temp);
    if (clicked)
    {
        *value &= ~bit;
        if (temp)
        {
            *value |= bit;
        }
    }
    return clicked;
}

typedef struct Slider_On_Hover_Params
{
    f32 *value;
    f32 min;
    f32 max;
} Slider_On_Hover_Params;

void ui_clay_slider_on_hover(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData)
{
    UI* ui = s_app->ui;
    Clay_ElementData data = Clay_GetElementData(elementId);
    
    Slider_On_Hover_Params* params = (Slider_On_Hover_Params*)userData;
    b32 do_value_update = false;
    
    if (elementId.id == ui->hover_id.id)
    {
        if (s_app->ui->input.mouse_press)
        {
            do_value_update = true;
        }
    }
    if (elementId.id == ui->down_id.id)
    {
        do_value_update = true;
    }
    
    if (do_value_update)
    {
        f32 x = pointerData.position.x - data.boundingBox.x;
        f32 value01 = x / data.boundingBox.width;
        *(params->value) = cf_remap01(value01, params->min, params->max);
    }
}

void ui_do_slider(f32 *value, f32 min, f32 max)
{
    UI* ui = s_app->ui;
    UI_Input* input = &ui->input;
    Clay_Color idle_color = ui_peek_idle_color();
    Clay_Color hover_color = ui_peek_hover_color();
    Clay_Color down_color = ui_peek_down_color();
    Clay_Color border_color = ui_peek_border_color();
    
    Clay_ElementId border_id = ui_make_clay_id("generic_border", "slider");
    Clay_ElementId id = ui_make_clay_id("generic", "slider");
    Clay_Color color = idle_color;
    
    Slider_On_Hover_Params* params = scratch_alloc(sizeof(Slider_On_Hover_Params));
    params->value = value;
    params->min = min;
    params->max = max;
    f32 value01 = cf_remap(*value, min, max, 0.0f, 1.0f);
    
    if (id.id == ui->hover_id.id || border_id.id == ui->hover_id.id)
    {
        color = hover_color;
    }
    if (id.id == ui->down_id.id || border_id.id == ui->hover_id.id)
    {
        color = down_color;
    }
    
    
    CLAY(border_id, {
             .border = {
                 .color = border_color,
                 .width = CLAY_BORDER_ALL(1),
             },
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_GROW(0),
                     .height = CLAY_SIZING_FIXED(10),
                 },
             },
         })
    {
        ui_add_navigation_node(border_id);
        // do hover on the border and not the actual slider bar
        // otherwise you'll get a flickering value offset
        Clay_OnHover(ui_clay_slider_on_hover, (intptr_t)params);
        
        CLAY(id, {
                 .backgroundColor = color,
                 .layout = {
                     .sizing = {
                         .width = CLAY_SIZING_PERCENT(value01),
                         .height = CLAY_SIZING_FIXED(10),
                     },
                 },
             })
        {
        }
    }
    
    ui_update_element_selection(border_id);
}

void ui_do_sprite_ex(UI_Sprite_Params* sprite_params)
{
    Clay_ElementId layout_id = ui_make_clay_id("generic_layout", "sprite_ex");
    Clay_ElementId id = ui_make_clay_id("generic_sprite", "sprite_ex");
    
    CLAY(layout_id, { 
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_FIXED(sprite_params->size.x),
                     .height = CLAY_SIZING_FIXED(sprite_params->size.y),
                 }
             },
             .aspectRatio = {
                 .aspectRatio = sprite_params->aspect_ratio,
             },
         })
    {
        CLAY(id, { .image = { .imageData = sprite_params } });
    }
}

b32 ui_do_sprite_button_ex(UI_Sprite_Params* sprite_params)
{
    UI* ui = s_app->ui;
    
    if (!sprite_params)
    {
        return false;
    }
    
    Clay_Color idle_color = ui_peek_idle_color();
    Clay_Color hover_color = ui_peek_hover_color();
    Clay_Color down_color = ui_peek_down_color();
    s32 font_size = 32;
    
    Clay_ElementId layout_id = ui_make_clay_id("generic_sprite_layout", "sprite_button_ex");
    Clay_Color color = idle_color;
    if (layout_id.id == ui->hover_id.id)
    {
        color = hover_color;
    }
    if (layout_id.id == ui->down_id.id)
    {
        color = down_color;
    }
    
    b32 clicked = false;
    
    Clay_ElementId image_id = ui_make_clay_id("generic_sprite", "sprite_button_ex");
    
    CLAY(layout_id, { 
             .backgroundColor = color,
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_FIXED(sprite_params->size.x),
                     .height = CLAY_SIZING_FIXED(sprite_params->size.y),
                 }
             },
         })
    {
        ui_add_navigation_node(layout_id);
        
        CLAY(image_id, { .image = { .imageData = sprite_params } });
    }
    
    ui_update_element_selection(layout_id);
    
    if (ui->select_id.id == layout_id.id)
    {
        clicked = true;
        //  @note:  bit a gross but prevents button from being repeatedly hit
        ui->select_id = (Clay_ElementId){ 0 };
    }
    
    return clicked;
}

void ui_do_image(const char* name, const char* animation, CF_V2 size)
{
    CF_Sprite sprite = cf_make_sprite(name);
    
    if (sprite.name)
    {
        UI_Sprite_Params* sprite_params = scratch_alloc(sizeof(UI_Sprite_Params));
        *sprite_params = (UI_Sprite_Params) {
            .middle.type = UI_Sprite_Params_Type_Image,
            .middle.name = cf_sintern(name),
            .middle.animation = cf_sintern(animation),
            .size = size,
            .aspect_ratio = (f32)sprite.w / (f32)sprite.h,
            .background = ui_peek_background_sprite(),
            .foreground = ui_peek_foreground_sprite(),
        };
        
        ui_do_sprite_ex(sprite_params);
    }
}

b32 ui_do_image_button(const char* name, const char* animation, CF_V2 size)
{
    CF_Sprite sprite = cf_make_sprite(name);
    b32 clicked = false;
    
    if (sprite.name)
    {
        UI_Sprite_Params* sprite_params = scratch_alloc(sizeof(UI_Sprite_Params));
        *sprite_params = (UI_Sprite_Params) {
            .middle.type = UI_Sprite_Params_Type_Image,
            .middle.name = cf_sintern(name),
            .middle.animation = cf_sintern(animation),
            .size = size,
            .aspect_ratio = (f32)sprite.w / (f32)sprite.h,
            .background = ui_peek_background_sprite(),
            .foreground = ui_peek_foreground_sprite(),
        };
        
        clicked = ui_do_sprite_button_ex(sprite_params);
    }
    else
    {
        clicked = ui_do_button(name);
    }
    
    return clicked;
}

void ui_do_sprite(CF_Sprite* sprite, CF_V2 size)
{
    if (sprite)
    {
        CF_Sprite* ui_sprite = scratch_alloc(sizeof(CF_Sprite));
        *ui_sprite = *sprite;
        
        UI_Sprite_Params* sprite_params = scratch_alloc(sizeof(UI_Sprite_Params));
        *sprite_params = (UI_Sprite_Params) {
            .middle.type = UI_Sprite_Params_Type_Sprite,
            .middle.sprite = ui_sprite,
            .size = size,
            .aspect_ratio = (f32)sprite->w / (f32)sprite->h,
            .background = ui_peek_background_sprite(),
            .foreground = ui_peek_foreground_sprite(),
        };
        
        ui_do_sprite_ex(sprite_params);
    }
}

b32 ui_do_sprite_button(CF_Sprite* sprite, CF_V2 size)
{
    UI* ui = s_app->ui;
    b32 clicked = false;
    
    if (sprite)
    {
        UI_Sprite_Params* sprite_params = scratch_alloc(sizeof(UI_Sprite_Params));
        *sprite_params = (UI_Sprite_Params) {
            .middle.type = UI_Sprite_Params_Type_Sprite,
            .middle.sprite = sprite,
            .size = size,
            .aspect_ratio = (f32)sprite->w / (f32)sprite->h,
            .background = ui_peek_background_sprite(),
            .foreground = ui_peek_foreground_sprite(),
        };
        
        clicked = ui_do_sprite_button_ex(sprite_params);
    }
    
    return clicked;
}

void ui_do_controller_axis_dead_zone(CF_V2 axis, CF_Aabb dead_zone, CF_V2 size)
{
    UI* ui = s_app->ui;
    
    s32 font_size = 32;
    
    Clay_ElementId layout_id = ui_make_clay_id("generic", "axis_dead_zone");
    Clay_ElementId id = ui_make_clay_id("generic_id", "axis_dead_zone");
    
    UI_Custom_Params* params = scratch_alloc(sizeof(UI_Custom_Params));
    params->type = UI_Custom_Params_Type_Axis_Dead_Zone;
    params->axis_dead_zone.axis = axis;
    params->axis_dead_zone.dead_zone = dead_zone;
    params->axis_dead_zone.size = size;
    
    CLAY(layout_id, { 
             .backgroundColor = ui_peek_idle_color(),
             .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
             .layout = {
                 .sizing = {
                     .width = CLAY_SIZING_FIXED(size.x),
                     .height = CLAY_SIZING_FIXED(size.y),
                 }
             },
         })
    {
        CLAY(id, { 
                 .custom = { .customData = params }
             });
    }
}

void ui_set_item_vtooltip(const char* fmt, va_list args)
{
    Input* input = s_app->input;
    UI* ui = s_app->ui;
    
    if (ui_is_item_hovered())
    {
        s32 allocation_size = 1024;
        char* buf = scratch_alloc(allocation_size);
        
        CF_Aabb aabb = cf_make_aabb(cf_v2(0, 0), cf_v2((f32)s_app->w, (f32)s_app->h));
        
        s32 length = vsnprintf(buf, allocation_size, fmt, args);
        
        Clay_String clay_string = (Clay_String){.chars = buf, .length = length};
        f32 font_size = ui_peek_font_size();
        Clay_Color text_color = ui_peek_text_color();
        
        cf_push_font_size(font_size);
        CF_V2 text_size = cf_text_size(buf, length);
        cf_pop_font_size();
        
        aabb.max.x -= text_size.x;
        aabb.max.y -= text_size.y;
        
        CF_V2 tooltip_position = input->screen_mouse;
        tooltip_position.y -= text_size.y;
        tooltip_position = cf_clamp_aabb_v2(aabb, tooltip_position);
        
        CLAY(ui_make_clay_id("generic", "tooltip"), {
                 .floating = {
                     .attachTo = CLAY_ATTACH_TO_ROOT,
                     .offset = {
                         .x = tooltip_position.x,
                         .y = tooltip_position.y,
                     },
                     .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                     .clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
                 },
                 .backgroundColor = { 128, 128, 128, 128 },
                 .cornerRadius = CLAY_CORNER_RADIUS(ui_peek_corner_radius()),
                 .layout = {
                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
                     .childAlignment = {
                         .x = CLAY_ALIGN_X_LEFT,
                         .y = CLAY_ALIGN_Y_TOP,
                     },
                     .childGap = 8,
                 },
             })
        {
            CLAY_TEXT(clay_string, CLAY_TEXT_CONFIG({ .fontSize = (s16)font_size, .textColor = text_color }));
        }
    }
}

void ui_set_item_tooltip(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ui_set_item_vtooltip(fmt, args);
    va_end(args);
}

b32 ui_is_item_hovered()
{
    UI* ui = s_app->ui;
    return ui->next_hover_id.id == ui->last_id.id;
}

b32 ui_is_item_selected()
{
    UI* ui = s_app->ui;
    return ui->select_id.id == ui->last_id.id;
}

b32 ui_is_any_selected()
{
    UI* ui = s_app->ui;
    return ui->select_id.id != 0;
}

void ui_start_modal(void (*fn)(mco_coro* co), void* udata)
{
    UI* ui = s_app->ui;
    CF_ASSERT(mco_status(ui->modal_co) == MCO_DEAD);
    
    {
        ui->modal_background_color = (Clay_Color){ 128, 128, 128, 128 };
        mco_desc desc = mco_desc_init(fn, 0);
        desc.user_data = udata;
        mco_result result = mco_create(&ui->modal_co, &desc);
        CF_ASSERT(result == MCO_SUCCESS);
    }
}

void ui_start_modal_with_color(void (*fn)(mco_coro* co), Clay_Color background_color, void* udata)
{
    UI* ui = s_app->ui;
    CF_ASSERT(mco_status(ui->modal_co) == MCO_DEAD);
    
    ui->modal_background_color = background_color;
    mco_desc desc = mco_desc_init(fn, 0);
    desc.user_data = udata;
    mco_result result = mco_create(&ui->modal_co, &desc);
    CF_ASSERT(result == MCO_SUCCESS);
}

b32 ui_is_modal_active()
{
    return mco_status(s_app->ui->modal_co) != MCO_DEAD;
}

void ui_update_modal()
{
    UI* ui = s_app->ui;
    
    if (ui->modal_co)
    {
        if (mco_status(ui->modal_co) == MCO_DEAD)
        {
            mco_destroy(ui->modal_co);
            ui->modal_co = NULL;
        }
        else
        {
            Clay_ElementId id = CLAY_ID("UI_Modal___OuterContainer");
            ui->modal_container_id = id;
            CLAY(id, {
                     .floating = {
                         .attachTo = CLAY_ATTACH_TO_ROOT,
                         .attachPoints =  {
                             .parent = CLAY_ATTACH_POINT_LEFT_TOP,
                         },
                     },
                     .backgroundColor = ui->modal_background_color,
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
                mco_resume(ui->modal_co);
            }
        }
    }
}
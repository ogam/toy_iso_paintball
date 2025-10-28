#ifndef UI_H
#define UI_H

#include "clay/clay.h"
#include "stb/stb_textedit.h"

#ifndef UI_STYLE_FORWARD_FUNCS
#define UI_STYLE_FORWARD_FUNCS(TYPE, NAME) \
void ui_push_##NAME(TYPE v); \
TYPE ui_pop_##NAME(); \
TYPE ui_peek_##NAME();
#endif

#ifndef UI_STYLE_FUNCS
#define UI_STYLE_FUNCS(TYPE, NAME) \
void ui_push_##NAME(TYPE v) \
{ \
UI_Style* style = &s_app->ui->style; \
cf_array_push(style->NAME, v); \
} \
TYPE ui_pop_##NAME() \
{ \
UI_Style* style = &s_app->ui->style; \
TYPE v = ui_peek_##NAME(style->NAME); \
if (cf_array_count(style->NAME) > 1) \
{ \
cf_array_pop(style->NAME); \
} \
return v; \
} \
TYPE ui_peek_##NAME() \
{ \
UI_Style* style = &s_app->ui->style; \
return cf_array_last(style->NAME); \
}
#endif

typedef s32 UI_Sprite_Params_Type;
enum
{
    UI_Sprite_Params_Type_None,
    // assets lookup, params.name and params.animation
    UI_Sprite_Params_Type_Image,
    // directly use sprite, params.sprite
    UI_Sprite_Params_Type_Sprite,
};

typedef struct UI_Layer_Sprite
{
    UI_Sprite_Params_Type type;
    b32 is_9_slice;
    union
    {
        CF_Sprite* sprite;
        struct
        {
            const char* name;
            const char* animation;
        };
    };
} UI_Layer_Sprite;

typedef struct UI_Sprite_Params
{
    UI_Layer_Sprite background;
    UI_Layer_Sprite middle;
    UI_Layer_Sprite foreground;
    CF_V2 size;
    f32 aspect_ratio;
} UI_Sprite_Params;

typedef s32 UI_Custom_Params_Type;
enum
{
    UI_Custom_Params_Type_None,
    UI_Custom_Params_Type_Input_Text,
    UI_Custom_Params_Type_Axis_Dead_Zone,
};

typedef s32 UI_Input_Text_Mode;
enum
{
    // always updates the passed in string
    UI_Input_Text_Mode_Default,
    UI_Input_Text_Mode_Sets_On_Return,
};

typedef struct UI_Custom_Params
{
    UI_Custom_Params_Type type;
    union
    {
        struct
        {
            char* text;
            CF_Color text_color;
            CF_Color text_select_color;
            CF_Color text_cursor_color;
            
            s32 cursor;
            s32 select_start;
            s32 select_end;
            
            f32 font_size;
            s32 font_id;
            
            f32 x_offset;
            CF_Rect scissor;
        } input_text;
        struct
        {
            CF_V2 axis;
            CF_Aabb dead_zone;
            CF_V2 size;
        } axis_dead_zone;
    };
} UI_Custom_Params;

enum
{
    _STB_TEXTEDIT_K_LEFT,
    _STB_TEXTEDIT_K_RIGHT,
    _STB_TEXTEDIT_K_UP,
    _STB_TEXTEDIT_K_DOWN,
    _STB_TEXTEDIT_K_PGUP,
    _STB_TEXTEDIT_K_PGDOWN,
    _STB_TEXTEDIT_K_LINESTART,
    _STB_TEXTEDIT_K_LINEEND,
    _STB_TEXTEDIT_K_DELETE,
    _STB_TEXTEDIT_K_BACKSPACE,
    _STB_TEXTEDIT_K_UNDO,
    _STB_TEXTEDIT_K_REDO,
    _STB_TEXTEDIT_K_COUNT,
};

typedef struct UI_Input_Text_State
{
    fixed char* src;
    fixed char* text;
    void* key;
    STB_TexteditState state;
    s32 ref_count;
    f32 x_offset;
} UI_Input_Text_State;

typedef struct UI_Input
{
    CF_V2 mouse;
    f32 mouse_wheel;
    b32 mouse_down;
    b32 mouse_press;
    b32 mouse_up;
    b32 mouse_release;
    
    b32 menu_pressed;
    b32 back_pressed;
    b32 accept_pressed;
    b32 text_input_accept;
    
    s32 stb_inputs[_STB_TEXTEDIT_K_COUNT];
    b32 do_paste;
    b32 do_copy;
    b32 do_cut;
    b32 do_delete_forward;
    b32 do_delete_backward;
    b32 do_select_all;
    
    CF_InputTextBuffer text;
    
    V2i direction;
    
    b32 is_digital_input;
} UI_Input;

typedef struct UI_Style
{
    dyna Clay_Color* idle_color;
    dyna Clay_Color* hover_color;
    dyna Clay_Color* down_color;
    dyna Clay_Color* select_color;
    dyna Clay_Color* text_color;
    dyna Clay_Color* empty_text_color;
    dyna Clay_Color* text_select_color;
    dyna Clay_Color* text_cursor_color;
    dyna Clay_Color* border_color;
    dyna UI_Layer_Sprite* background_sprite;
    dyna UI_Layer_Sprite* foreground_sprite;
    // not really a style but macro laziness
    dyna b32* interactable;
    dyna b32* digital_input;
    
    dyna f32* corner_radius;
    
    dyna f32* font_size;
} UI_Style;

typedef s32 UI_Navigation_Mode;
enum
{
    UI_Navigation_Mode_None,
    UI_Navigation_Mode_Vertical = 1 << 0,
    UI_Navigation_Mode_Horizontal = 1 << 1,
    UI_Navigation_Mode_Default = UI_Navigation_Mode_Vertical | UI_Navigation_Mode_Horizontal,
};

typedef s32 UI_Navigation_Next_Node_Path;
enum
{
    UI_Navigation_Next_Node_Path_None,
    UI_Navigation_Next_Node_Path_Top = 1 << 0,
    UI_Navigation_Next_Node_Path_Bottom = 1 << 1,
    UI_Navigation_Next_Node_Path_Left = 1 << 2,
    UI_Navigation_Next_Node_Path_Right = 1 << 3,
    UI_Navigation_Next_Node_Path_Top_List = 1 << 4,
    UI_Navigation_Next_Node_Path_Bottom_List = 1 << 5,
    UI_Navigation_Next_Node_Path_Left_List = 1 << 6,
    UI_Navigation_Next_Node_Path_Right_List = 1 << 7,
    UI_Navigation_Next_Node_Path_Default = 
        UI_Navigation_Next_Node_Path_Top | UI_Navigation_Next_Node_Path_Bottom | 
        UI_Navigation_Next_Node_Path_Left | UI_Navigation_Next_Node_Path_Right,
};

typedef struct UI_Navigation_Node
{
    Clay_ElementId id;
    CF_Aabb aabb;
    
    struct UI_Navigation_Layout* layout;
    
    struct UI_Navigation_Node* up;
    struct UI_Navigation_Node* down;
    struct UI_Navigation_Node* left;
    struct UI_Navigation_Node* right;
} UI_Navigation_Node;

typedef struct UI_Navigation_Layout
{
    UI_Navigation_Mode mode;
    UI_Navigation_Next_Node_Path next_node_path;
    
    CF_Aabb aabb;
    
    fixed struct UI_Navigation_Node* nodes;
    
    struct UI_Navigation_Node* up;
    struct UI_Navigation_Node* down;
    struct UI_Navigation_Node* left;
    struct UI_Navigation_Node* right;
    
    // these are indices incase `nodes` grows, then the addresses will point to some old memory
    // and will be invalid
    fixed s32* top_row;
    fixed s32* bottom_row;
    fixed s32* left_column;
    fixed s32* right_column;
} UI_Navigation_Layout;

//  @note:  if doing fixed update use a double buffer arena so ui data
//          doesn't get stomped on or a block allocator and occasionally
//          defrag
typedef struct UI
{
    Clay_RenderCommandArray commands;
    
    Clay_ElementId next_hover_id;
    Clay_ElementId hover_id;
    Clay_ElementId down_id;
    Clay_ElementId select_id;
    Clay_ElementId last_id;
    
    Clay_ElementId modal_container_id;
    Clay_ElementId select_id_before_modal;
    // used for when digital input starts to hover on elements that are out of container
    // view bounds
    Clay_Vector2 digital_input_scroll_offset;
    
    UI_Input input;
    UI_Style style;
    dyna const char** fonts;
    
    dyna const char** input_text_strings;
    dyna UI_Input_Text_State* input_text_states;
    
    Clay_Color modal_background_color;
    mco_coro *modal_co;
    
    dyna struct UI_Navigation_Layout* navigation_layouts;
    dyna struct UI_Navigation_Layout** navigation_layout_stack;
    
    s32 element_counter;
} UI;

UI_STYLE_FORWARD_FUNCS(Clay_Color, idle_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, hover_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, down_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, select_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, text_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, empty_text_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, text_select_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, text_cursor_color);
UI_STYLE_FORWARD_FUNCS(Clay_Color, border_color);
UI_STYLE_FORWARD_FUNCS(UI_Layer_Sprite, background_sprite);
UI_STYLE_FORWARD_FUNCS(UI_Layer_Sprite, foreground_sprite);
UI_STYLE_FORWARD_FUNCS(b32, interactable);
UI_STYLE_FORWARD_FUNCS(b32, digital_input);
UI_STYLE_FORWARD_FUNCS(f32, corner_radius);
UI_STYLE_FORWARD_FUNCS(f32, font_size);

void ui_set_fonts(const char** fonts, s32 count);

void ui_init();
void ui_update_input();
void ui_update();
void ui_draw();
void ui_begin();
void ui_end();

void ui_handle_digital_input();

void ui_navigation_layout_begin(UI_Navigation_Mode mode);
struct UI_Navigation_Layout* ui_peek_navigation_layout();
void ui_navigation_layout_end();
void ui_layout_set_next_node_pathing(UI_Navigation_Next_Node_Path pathing);

UI_Navigation_Node* ui_layout_get_node(Clay_ElementId id);
UI_Navigation_Node* ui_layout_get_first_node();

void ui_add_navigation_node(Clay_ElementId id);
void ui_process_navigation_nodes();

CF_Color clay_color_to_color(Clay_Color c);
Clay_Color color_to_clay_color(CF_Color c);
Clay_ElementId ui_make_clay_id(const char* prefix, const char* suffix);
Clay_ElementId ui_make_clay_id_index(const char* prefix, s32 index);

void ui_do_text(const char* fmt, ...);

Clay_ElementId ui_do_input_text_ex(UI_Input_Text_State* state, f32 width);
b32 ui_do_input_text(fixed char* text, UI_Input_Text_Mode mode);
b32 ui_do_input_s32(s32* v, s32 min, s32 max);
b32 ui_do_input_f32(f32* v, f32 min, f32 max);
b32 ui_do_button(const char* text);
b32 ui_do_button_wide(const char* text);
b32 ui_do_checkbox(b32* value);
b32 ui_do_checkbox_bit(b32* value, b32 bit);

// due to clay processing all changes during end layout phase,
// we won't know the actual value change until it's a frame late
// so not returning anything for this for now..
b32 ui_do_slider(f32 *value, f32 min, f32 max);

//  @todo:  all clay stuff should include a size so we can make sure 
//          render calculations are within the correct bounds
void ui_do_sprite_ex(UI_Sprite_Params* sprite_params);
b32 ui_do_sprite_button_ex(UI_Sprite_Params* sprite_params);

void ui_do_image(const char* name, const char* animation, CF_V2 size);
b32 ui_do_image_button(const char* name, const char* animation, CF_V2 size);
void ui_do_sprite(CF_Sprite* sprite, CF_V2 size);
b32 ui_do_sprite_button(CF_Sprite* sprite, CF_V2 size);
void ui_do_controller_axis_dead_zone(CF_V2 axis, CF_Aabb dead_zone, CF_V2 size);

void ui_set_item_tooltip(const char* fmt, ...);
b32 ui_is_item_hovered();
b32 ui_is_item_selected();
b32 ui_is_any_selected();

void ui_start_modal(void (*fn)(mco_coro* co), void* udata);
void ui_start_modal_with_color(void (*fn)(mco_coro* co), Clay_Color background_color, void* udata);
b32 ui_is_modal_active();
//  internal, used to setup layout for current active modal
void ui_update_modal();

void ui_do_auto_scroll(Clay_ElementId id);
b32 ui_check_back_pressed();

#endif //UI_H
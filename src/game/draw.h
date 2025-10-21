#ifndef DRAW_H
#define DRAW_H

//  @note:  cf_draw_layer() uses 32 bit sort key,
//          we need a bit more precision to include a range of elevation changes
//          so manage and sort this on our own before pushing it all in as a single
//          draw layer
typedef u8 Draw_Sort_Key_Type;
enum
{
    Draw_Sort_Key_Type_None,
    Draw_Sort_Key_Type_Tile_Stack,
    Draw_Sort_Key_Type_Tile,
    Draw_Sort_Key_Type_Tile_Outline,
    Draw_Sort_Key_Type_Tile_Fill,
    Draw_Sort_Key_Type_Shadow,
    Draw_Sort_Key_Type_Decal,
    Draw_Sort_Key_Type_Prop,
    Draw_Sort_Key_Type_Emote,
    Draw_Sort_Key_Type_Unit,
};

// sort key is setup to be drawn back to front, left to right, bottom to top
// left to right high bit range (x)
// back to front      mid range (y)
// elevation          low range
typedef union Draw_Sort_Key
{
    struct
    {
        // lowest bit range should be based on types to allow a prop to be drawn first before
        // a unit does
        u64 type : 8;
        // elevation here is fairly large compared to other parts of this key
        // mainly because total elevation is a f32 which means it can vary a lot
        // we have subtle elvation differences when there's any velocity applied
        // to tiles so these need to be rendered in the correct order otherwise
        // it'll be misleading which one is in front
        u64 elevation : 32;
        u64 y : 8;
        u64 x : 8;
        // not really used for now, should really use it for the background
        u64 layer : 8;
    };
    u64 value;
} Draw_Sort_Key;

typedef s32 Draw_Command_Type;
enum
{
    Draw_Command_Type_Circle,
    Draw_Command_Type_Circle_Fill,
    Draw_Command_Type_Line,
    Draw_Command_Type_Arrow,
    Draw_Command_Type_Polyline,
    Draw_Command_Type_Polygon_Fill,
    Draw_Command_Type_Sprite,
};

typedef struct Draw_Command
{
    Draw_Sort_Key key;
    Draw_Command_Type type;
    CF_Color color;
    f32 thickness;
    union
    {
        struct
        {
            CF_V2 p0;
            CF_V2 p1;
        } line;
        CF_Poly poly;
        CF_Circle circle;
        CF_Sprite sprite;
    };
} Draw_Command;

void draw_push_color(CF_Color c);
CF_Color draw_peek_color();
CF_Color draw_pop_color();

void draw_push_layer(u8 layer);
u8 draw_peek_layer();
u8 draw_pop_layer();

void draw_push_thickness(f32 thickness);
f32 draw_peek_thickness();
f32 draw_pop_thickness();

void draw_push_all();
void draw_sort();
void draw_clear();
Draw_Command draw_make_command(Draw_Sort_Key_Type type, V2i tile, f32 elevation);
void draw_push_sprite(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Sprite* sprite);
void draw_push_circle(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p, f32 r);
void draw_push_circle_fill(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p, f32 r);
void draw_push_line(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p0, CF_V2 p1);
void draw_push_arrow(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p0, CF_V2 p1);
void draw_push_polyline(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Poly poly);
void draw_push_polyline_fill(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Poly poly);

#endif //DRAW_H

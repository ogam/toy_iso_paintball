#include "game/draw.h"

void draw_push_color(CF_Color c)
{
    cf_array_push(s_app->world->draw.colors, c);
}

CF_Color draw_peek_color()
{
    return cf_array_last(s_app->world->draw.colors);
}

CF_Color draw_pop_color()
{
    dyna CF_Color* colors = s_app->world->draw.colors;
    CF_Color c = draw_peek_color();
    if (cf_array_count(colors) > 1)
    {
        cf_array_pop(colors);
    }
    return c;
}

void draw_push_layer(u8 layer)
{
    cf_array_push(s_app->world->draw.layers, layer);
}

u8 draw_peek_layer()
{
    return cf_array_last(s_app->world->draw.layers);
}

u8 draw_pop_layer()
{
    dyna u8* layers = s_app->world->draw.layers;
    u8 layer = draw_peek_layer();
    if (cf_array_count(layers) > 1)
    {
        cf_array_pop(layers);
    }
    return layer;
}

void draw_push_thickenss(f32 thickness)
{
    cf_array_push(s_app->world->draw.thickness, thickness);
}

f32 draw_peek_thickness()
{
    return cf_array_last(s_app->world->draw.thickness);
}

f32 draw_pop_thickness()
{
    dyna f32* thickness = s_app->world->draw.thickness;
    f32 result = draw_peek_thickness();
    if (cf_array_count(thickness) > 1)
    {
        cf_array_pop(thickness);
    }
    return result;
}

void draw_push_all()
{
    World* world = s_app->world;
    dyna Draw_Command* draw_commands = world->draw.commands;
    
    for (s32 index = 0; index < cf_array_count(draw_commands); ++index)
    {
        Draw_Command* command = draw_commands + index;
        switch (command->type)
        {
            case Draw_Command_Type_Circle:
            {
                cf_draw_push_color(command->color);
                cf_draw_circle(command->circle, command->thickness);
                cf_draw_pop_color();
            }
            break;
            case Draw_Command_Type_Circle_Fill:
            {
                cf_draw_push_color(command->color);
                cf_draw_circle_fill(command->circle);
                cf_draw_pop_color();
            }
            break;
            case Draw_Command_Type_Line:
            {
                cf_draw_push_color(command->color);
                cf_draw_line(command->line.p0, command->line.p1, command->thickness);
                cf_draw_pop_color();
            }
            break;
            case Draw_Command_Type_Arrow:
            {
                cf_draw_push_color(command->color);
                cf_draw_arrow(command->line.p0, command->line.p1, command->thickness, 5.0f);
                cf_draw_pop_color();
            }
            break;
            case Draw_Command_Type_Polyline:
            {
                cf_draw_push_color(command->color);
                cf_draw_polyline(command->poly.verts, command->poly.count, command->thickness, true);
                cf_draw_pop_color();
            }
            break;
            case Draw_Command_Type_Polygon_Fill:
            {
                cf_draw_push_color(command->color);
                cf_draw_polygon_fill(command->poly.verts, command->poly.count, command->thickness);
                cf_draw_pop_color();
            }
            break;
            case Draw_Command_Type_Sprite:
            {
                cf_draw_push_color(command->color);
                cf_draw_sprite(&command->sprite);
                cf_draw_pop_color();
            }
            break;
        }
    }
}

int draw_command_compare(const void* a, const void* b)
{
    u64 a_key = ((const Draw_Command*)a)->key.value;
    u64 b_key = ((const Draw_Command*)b)->key.value;
    return a_key > b_key ? 1 : a_key < b_key ? -1 : 0;
}

void draw_sort()
{
    World* world = s_app->world;
    dyna Draw_Command* commands = world->draw.commands;
    //  @todo:  whatever msvc implemention of qsort is it's really bad
    //          don't know how well clang, gcc or mingw qsort handles it but this needs to go yesterday
    //          write a radix sort or something because filling a 20x20 grid blows up the memory
    //          usage from 600mb (yeah that's pretty big for a start up) to 12 gb (excuse me?!)
    qsort(commands, cf_array_count(commands), sizeof(Draw_Command), draw_command_compare);
}

void draw_clear()
{
    cf_array_clear(s_app->world->draw.commands);
}

Draw_Command draw_make_command(Draw_Sort_Key_Type type, V2i tile, f32 elevation)
{
    World* world = s_app->world;
    return (Draw_Command){
        .key = {
            .x = world->level.size.x - tile.x,
            .y = world->level.size.y - tile.y,
            .type = type,
            // using 100,000 value here
            // 6 decimal places should cover a good amount of range
            // with a 30.0f-60.0f elevation height (30 base max)
            // that gives us about 6e6 range that we can cover
            // currently game does not allow for negative elevation 
            // since any negative value means the tile should be destroyed
            // max range for a u32 (u64 : 32) here is ~4e9 so that's
            // enough precision to deal with this
            .elevation = (u64)(elevation * 100000),
            .layer = draw_peek_layer(),
        },
        .thickness = draw_peek_thickness(),
        .color = draw_peek_color(),
    };
}

void draw_push_sprite(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Sprite* sprite)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Sprite;
    command.sprite = *sprite;
    cf_array_push(s_app->world->draw.commands, command);
}

void draw_push_circle(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p, f32 r)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Circle;
    command.circle = cf_make_circle(p, r);
    cf_array_push(s_app->world->draw.commands, command);
}

void draw_push_circle_fill(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p, f32 r)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Circle_Fill;
    command.circle = cf_make_circle(p, r);
    cf_array_push(s_app->world->draw.commands, command);
}

void draw_push_line(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p0, CF_V2 p1)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Line;
    command.line.p0 = p0;
    command.line.p1 = p1;
    cf_array_push(s_app->world->draw.commands, command);
}

void draw_push_arrow(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_V2 p0, CF_V2 p1)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Arrow;
    command.line.p0 = p0;
    command.line.p1 = p1;
    cf_array_push(s_app->world->draw.commands, command);
}

void draw_push_polyline(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Poly poly)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Polyline;
    command.poly = poly;
    cf_array_push(s_app->world->draw.commands, command);
}

void draw_push_polyline_fill(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Poly poly)
{
    Draw_Command command = draw_make_command(type, tile, elevation);
    command.type = Draw_Command_Type_Polygon_Fill;
    command.poly = poly;
    cf_array_push(s_app->world->draw.commands, command);
}

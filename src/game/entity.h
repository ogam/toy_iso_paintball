#ifndef ENTITY_H
#define ENTITY_H

#define LEVEL_SIZE_MIN (v2i(.x = 20, .y = 20))
#define LEVEL_SIZE_MAX (v2i(.x = 100, .y = 100))

#define MAX_ELEVATION (30)
#define ELEVATION_GRAVITY (8)
#define CLIMBABLE_ELEVATION (0.75f)
#define ELEVATION_FALL_THRESHOLD (0.5f)
#define ELEVATION_TOUCH_DISTANCE (0.5f)

typedef struct Tile
{
    // @elevation
    // all tiles are in 1.0f,1.0f,0.5f to match affine matrix drawing and elevation 
    // reachable checks every 2 elvation is 1 tile up, this is to allow half/short 
    // tiles evaluation all offsets to elevation MUST be within 0->1 range otherwise
    // you'll be moving tiles outside of the 1,1,1 mapping
    // elevation SHOULD always be positive
    union
    {
        //  @todo:  actually implement 
        //          slopes
        //          slippery surface
        //          bounce surfaces
        struct
        {
            s8 walkable : 1;
            s8 slope_N : 1;
            s8 slope_S : 1;
            s8 slope_E : 1;
            s8 slope_W : 1;
            s8 slippery : 1;
            s8 bounce_pad : 1;
            s8 padding : 1;
        };
        s8 state;
    };
    s8 elevation;
    //  @todo:  use these 8 bits for auto tiling, so the editor can use auto tiling mode
    //          or turn it off, this allows for users to have more easy brush and control
    //          over the level editor
    s8 tiling;
    //  @todo:  8 bit mask for any switches/lever triggers
    s8 switches;
} Tile;

static_assert(sizeof(Tile) == sizeof(s32), "struct Tile needs to be 4 bytes, check padding");

typedef struct C_Sprite
{
    CF_Sprite sprite;
    const char* name;
    cf_htbl const char** animations;
    CF_V2 scale;
} C_Sprite;

typedef struct C_Transform
{
    CF_V2 position;
    CF_V2 prev_position;
} C_Transform;

typedef struct C_Child_Transform
{
    ecs_id_t parent;
    CF_V2 offset;
} C_Child_Transform;

typedef struct C_Unit_Transform
{
    V2i direction;
    V2i tile;
    V2i prev_tile;
} C_Unit_Transform;

typedef struct C_Mover
{
    f32 next_move_rate;
    f32 move_rate;
    f32 move_time;
    b32 is_moving;
} C_Mover;

typedef struct C_Navigation
{
    dyna V2i* path;
    s32 path_index;
} C_Navigation;

typedef struct C_Action
{
    b32 try_fire;
    V2i fire_tile;
    b32 apply_new_path;
} C_Action;

typedef struct C_AI
{
    ecs_id_t target;
    s32 aim_distance;
    b32 can_aim_at_target;
    f32 patrol_move_rate;
    f32 chase_move_rate;
    f32 aim_move_rate;
    s32 disengage_distance;
} C_AI;

typedef struct C_AI_Patrol
{
    dyna V2i* tiles;
    s32 index;
    b32 restart_path;
    b32 force_new_path;
    s32 patrol_distance;
} C_AI_Patrol;

typedef struct C_AI_View
{
    s32 distance;
    fixed V2i* tiles;
} C_AI_View;

#ifndef CONTROL_INVALID_CONTROL
#define CONTROL_INVALID_CONTROL (-1)
#endif

typedef struct C_Control
{
    fixed V2i* preview_path;
    s32 order;
    b32 is_locked;
} C_Control;

typedef struct C_Weapon
{
    s32 projectiles_per_fire;
    s32 cost_per_fire;
    s32 projectile_distance;
    // lower the better, 0 essentially always goes directly to target
    s32 ammunition;
    s32 max_ammunition;
    s32 accuracy;
    f32 fire_delay;
    f32 fire_rate;
    
    const char* projectile_name;
} C_Weapon;

// projectiles move from start of line to end of line in world space 
// it's a lerp from start to end, but during travel there's checks 
// against tile elevation to determine if the projectile hits a tile by the
// delta of C_Elevation. 
// hitting units on the other hand uses a quad-tree (pico_qt) and a bounding 
// box to determine if a unit has been hit.
// projectile speed is based off of the length of the line and C_Life_Time
// to make projectiles move faster, reduce projectile life time or extend
// the length of the line, ideally length of the line should be a reasonable
// otherwise you would end up having projectiles move insanely fare that
// would be outside of the level sim region.
typedef struct C_Projectile
{
    ecs_id_t owner;
    dyna V2i* line;
    f32 start_elevation;
    s32 team_id;
} C_Projectile;

typedef struct C_Life_Time
{
    f32 duration;
    f32 total;
} C_Life_Time;

//  @todo:  this is really suppose to be a bool component, that if it exists
//          then a system would filter out entities with this component
//          `system_update_camera()` mainly
//          this sometimes works okay but adding and removing this very quickly
//          can cause problems for pico_ecs(), it'll still run through systems
//          for entities that does not have the component
//          another problem being when a component is removed it doesn't remove
//          this from all systems everytime?
//  @todo:  repro in a smaller project and investigate pico_ecs on why this happens
//          @ecs_remove
typedef u8 C_Camera_Focus;

typedef struct C_Collider
{
    CF_V2 half_extents;
} C_Collider;

typedef struct C_Health
{
    s32 value;
    s32 prev_value;
    b32 is_invulnerable;
} C_Health;

typedef u8 C_Corpse;

typedef struct C_Elevation
{
    f32 value;
    f32 prev_value;
    f32 grounded_value;
    f32 velocity;
} C_Elevation;

typedef struct C_Elevation_Effector
{
    f32 impulse;
    f32 start_radius;
    f32 end_radius;
    b32 ignore_start_tile;
} C_Elevation_Effector;

typedef struct C_Decal
{
    f32 fade_delay;
} C_Decal;

typedef struct C_Level_Exit
{
    b32 activated;
} C_Level_Exit;

typedef u8 C_Prop;

typedef struct Emoter_Rule
{
    f32 chance;
    //  @asset_resource 
    dyna Sprite_Reference* emotes;
} Emoter_Rule;

typedef struct C_Emoter
{
    Emoter_Rule on_alert;
    Emoter_Rule on_idle;
    Emoter_Rule on_hit;
    Emoter_Rule on_kill;
    Emoter_Rule on_dead;
    CF_V2 scale;
    ecs_id_t emote_id;
} C_Emoter;

typedef struct C_Emote
{
    ecs_id_t owner;
} C_Emote;

typedef s32 PickupType;
enum
{
    PickupType_Ammunition,
};

typedef struct C_Pickup
{
    PickupType type;
    s32 count;
} C_Pickup;

typedef C_Pickup Pickup_Params;

typedef struct C_Sound_Source
{
    //  @asset_resource 
    dyna const char** on_fire;
    dyna const char** on_alert;
    dyna const char** on_idle;
    dyna const char** on_hit_taken;
    dyna const char** on_dead;
    dyna const char** on_pickup;
    Audio_Source_Type type;
} C_Sound_Source;

// reverse lookup
typedef struct C_Asset_Resource
{
    const char* name;
} C_Asset_Resource;

typedef struct C_Spawner
{
    //  @asset_resource
    dyna const char** on_destroy;
} C_Spawner;

typedef struct C_Jumper
{
    f32 interval;
    f32 impulse;
} C_Jumper;

typedef struct C_UI
{
    Sprite_Reference* current;
    
    Sprite_Reference select;
    Sprite_Reference deselect;
    Sprite_Reference leader;
    Sprite_Reference hover;
    Sprite_Reference dead;
} C_UI;

typedef struct C_Team
{
    s32 id;
    b32 friendly_fire;
} C_Team;

typedef s32 Event_Type;

enum
{
    Event_Type_None,
    // collision
    Event_Type_Ground_Impact,
    Event_Type_Load_Level,
    Event_Type_On_Alert,
    Event_Type_On_Idle,
    Event_Type_On_Hit,
    Event_Type_On_Kill,
    Event_Type_On_Dead,
    Event_Type_On_Fire,
    Event_Type_On_Pickup,
    Event_Type_Do_Select_Control_Unit,
    Event_Type_On_Select_Control_Unit,
    Event_Type_On_Deselect_Control_Unit,
    Event_Type_On_UI_Hover_Control_Unit,
    //  @todo:  vfx, etc
};

//  @note:  talking to Michel Buelens, he mentions their engine uses event entities to 
//          do all events for their game. single frame entities to represent changes 
//          that has happened for other systems to pick up. this cuts out a lot of having 
//          to manage a separate event system and managing read/write policy and allow/disallow
//          per event message consumption. only bit about having events to work across engine 
//          systems or other libraries is to maybe have a separate ECS, 1 for game and 1 for overall
//          engine, that way you can still handle things outside of the core ecs system update loops.

//  @note:  READ ONLY, do not modify after spawning any events
typedef struct C_Event
{
    Event_Type type;
    
    union
    {
        struct
        {
            V2i tile;
            f32 impact;
            b32 ignore_start_tile;
        } ground_impact;
        struct
        {
            const char* name;
        } load_level;
        struct
        {
            ecs_id_t owner;
            ecs_id_t target;
        } on_alert;
        struct
        {
            ecs_id_t owner;
        } on_idle;
        struct
        {
            ecs_id_t owner;
            ecs_id_t target;
        } on_hit;
        struct
        {
            ecs_id_t owner;
            ecs_id_t target;
        } on_kill;
        struct
        {
            ecs_id_t owner;
        } on_dead;
        struct
        {
            ecs_id_t owner;
            CF_V2 position;
            V2i tile;
            f32 elevation;
        } on_fire;
        struct
        {
            ecs_id_t owner;
            const char* asset_resource_name;
        } on_pickup;
        struct
        {
            ecs_id_t entity;
        } select_control_unit;
    };
} C_Event;

typedef struct Entity_Grid
{
    dyna ecs_id_t** slots;
    cf_htbl V2i* lookup;
    s32 w;
    s32 h;
} Entity_Grid;


// ---------------
// Draw Commands
// ---------------

//  @todo:  this doesn't really belong here but since we only really are about
//          draw sorting for the world it's going to stay in this file for now

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

typedef struct Camera
{
    CF_V2 position;
    CF_V2 next_position;
} Camera;

typedef struct Level
{
    fixed char* file_name;
    fixed char* name;
    fixed char* music_file_name;
    fixed char* background_file_name;
    
    CF_Sprite* background;
    
    V2i size;
    
    // half / short tiles are odd (1, 3, 5, etc)
    // tall tiles are even (2, 4, 6, 8)
    Tile* tiles;
    Asset_Object_ID* tile_ids;
    // check @elevation for notes, moving offsets from -1 -> 1 will move the entire tile from
    // 1 tile depth down or 1 tile depth up, 0.5f to move it half way (half/short tiles)
    f32* tile_elevation_offsets;
    f32* tile_elevation_velocity_offsets;
} Level;

//  @todo:  keep track of hits/timers/etc
typedef struct Level_Stats
{
    s32 activated_exits;
    s32 total_exits;
} Level_Stats;

typedef s32 World_State;
enum
{
    World_State_None,
    World_State_Play,
    World_State_Pause,
    World_State_Demo,
};

typedef struct World
{
    Level level;
    Level_Stats level_stats;
    Camera camera;
    
    CF_Rnd prop_rnd;
    CF_Rnd ai_rnd;
    
    Entity_Grid grid;
    qt_t* qt;
    
    dyna ecs_id_t* control_selection_queue;
    dyna ecs_id_t* control_hover_queue;
    
    dyna ecs_id_t* destroy_list;
    
    Asset_Object_ID campaign_id;
    s32 level_index;
    
    struct
    {
        dyna Draw_Command* commands;
        dyna CF_Color* colors;
        dyna f32* thickness;
    } draw;
    
    struct
    {
        b32 show_ai_view_cone;
        b32 show_pathing;
    } debug;
    
    World_State state;
} World;

// ---------------
// Entity Grid
// ---------------

Entity_Grid entity_grid_make(s32 width, s32 height, s32 depth);
void entity_grid_insert(Entity_Grid* grid, V2i slot, ecs_id_t id);
void entity_grid_remove(Entity_Grid* grid, ecs_id_t id);
void entity_grid_clear(Entity_Grid* grid);
dyna ecs_id_t* entity_grid_query(Entity_Grid* grid, V2i tile);
void entity_grid_remove_internal(Entity_Grid* grid, V2i tile, ecs_id_t id);

// ---------------
// Draw Commands
// ---------------

void draw_push_color(CF_Color c);
CF_Color draw_peek_color();
CF_Color draw_pop_color();

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
void draw_push_polyline(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Poly poly);
void draw_push_polyline_fill(Draw_Sort_Key_Type type, V2i tile, f32 elevation, CF_Poly poly);

// ---------------
// world
// ---------------

typedef s32 Campaign_State;
enum
{
    Campaign_State_Invalid,
    Campaign_State_In_Progress,
    Campaign_State_Finish,
};

const char* world_get_campaign_name();
b32 world_load_campaign(Asset_Object_ID id);
void world_unload_campaign();
// true means you continue the campaign, false means campaign is finished
b32 world_advance_campaign();
Campaign_State world_get_campaign_state();
s32 world_remaining_campaign_levels();

f32 elevation_s32_to_f32(s32 elevation);
s32 get_tile_index(V2i tile);
s32 get_tile_elevation(V2i tile);
f32 get_tile_elevation_f32(V2i tile);
f32 get_tile_elevation_offset(V2i tile);
f32 get_tile_elevation_velocity(V2i tile);
f32 get_tile_total_elevation(V2i tile);
b32 tile_set_minimum_elevation(V2i tile, Asset_Object_ID id, s32 minimum_elevation);
b32 tile_raise(V2i tile, s32 amount, s32 max_elevation);
b32 tile_lower(V2i tile, s32 amount, s32 min_elevation);
b32 tile_place_or_raise(V2i tile, Asset_Object_ID id, s32 amount, s32 max_elevation);
b32 tile_remove(V2i tile);
b32 tile_remove_or_lower(V2i tile, s32 amount, s32 min_elevation);
Tile* get_tile(V2i tile);
Asset_Object_ID* get_tile_id(V2i tile);
b32 is_culled(CF_V2 position);
CF_Aabb get_level_aabb();

//  @todo:  @randy @jamesking
//          test out SDF tile sampling instead, this might be better 
//          since we're currently relying on quick hull to determine 
//          if world space coordinate is on top of a tile then additional
//          x,y ordering if it's being overlapped by a nearby tile
//          quick hull is probably slower than a good geometry /
//          sign distance function would be better. quick hull on 4-5
//          and area calculation on 4-5 vertices isn't too slow but
//          it's more complicated than a good math function to determine
//          any clipping.
//          tldr: current version is similar to doing a collision hit
//                test against a shape rather than something more
//                effecient
//  @jamesking
//     you can do quick geometric point-hit testing by representing the geom as distance fields
//  @randy
//    I was thinking about 3D space and sampling, per pixel, what depth each tile is at. 
//    Maybe SDF is a terrible idea (I was thinking signed distance function, not field)

// gets closest tile from actual world position, use this if you want objects to
// go behind tiles. this will early out as soon as it finds a good tile cluster to 
// pick from
V2i get_tile_sample_with_depth(CF_V2 position);
V2i get_tile_from_world(CF_V2 position);
// gets closest tile from depth, use this for any mouse tile picking or editor mode
// this will crawl as far as it can to find the closest tile depth wise
V2i get_tile_from_input_cursor(CF_V2 position, b32 allow_empty_tiles);

CF_Poly get_tile_top_surface_poly(V2i tile);
void draw_tile(V2i tile);
void draw_tile_outline(V2i tile);
void draw_tile_fill(V2i tile);
b32 is_tile_in_bounds(V2i tile);
b32 is_tile_viewable(V2i current, V2i next);
b32 is_tile_reachable(V2i current, V2i next);
b32 is_tile_empty(V2i tile);
b32 is_tile_hazardous(V2i tile);

//  @todo:  write a shader for tile color palette, this is to allow having tiles at different
//          elevations to be colored different along with different coordinates <x,y> to be 
//          slightly off colored if we don't end up with auto tiling. mainly to have it it's 
//          easier to understand what's going on when an entity is moving along the tile surface
//          since right now with all tiles looking essentially the same it's pretty bad on the
//          eyes, hard to decern tiles apart from each other
fixed V2i* astar(V2i start, V2i end, s32 max_distance);

void world_init();

void world_update();
void world_draw();

void world_clear();
void world_load_empty();
void world_load_random_demo_level();
fixed ecs_id_t* world_load(const char* name);
b32 world_is_level_loaded();
void world_reload();
void world_assets_load(const char* music, const char* background);
void world_assets_unload(const char* music, const char* background);

void world_set_state(World_State state);

// ---------------
// ecs
// ---------------

#ifndef ECS_GET_COMPONENT_ID
#define ECS_GET_COMPONENT_ID(NAME) ( cf_hashtable_get(s_app->component_ids, cf_sintern(#NAME)) )
#endif

#ifndef ECS_GET_PROPERTY_COMPONENT_ID
#define ECS_GET_PROPERTY_COMPONENT_ID(NAME) ( cf_hashtable_get(s_app->component_ids, cf_sintern(NAME)) )
#endif

#ifndef ECS_REGISTER_COMPONENT
#define ECS_REGISTER_COMPONENT(TYPE, CTOR, DTOR) \
{ \
ecs_id_t component_id = ecs_register_component(s_app->ecs, sizeof(TYPE), CTOR, DTOR); \
cf_hashtable_set(s_app->component_ids, cf_sintern(#TYPE), component_id); \
}
#endif

#ifndef ECS_ADD_COMPONENT
#define ECS_ADD_COMPONENT(ENTITY, COMPONENT) \
(ecs_add(s_app->ecs, ENTITY, ECS_GET_COMPONENT_ID(COMPONENT), NULL), \
ecs_get(s_app->ecs, ENTITY, ECS_GET_COMPONENT_ID(COMPONENT)))
#endif

#ifndef ECS_ADD_PROPERTY_COMPONENT
#define ECS_ADD_PROPERTY_COMPONENT(ENTITY, COMPONENT) \
(ecs_add(s_app->ecs, ENTITY, ECS_GET_PROPERTY_COMPONENT_ID(COMPONENT), NULL), \
ecs_get(s_app->ecs, ENTITY, ECS_GET_PROPERTY_COMPONENT_ID(COMPONENT)))
#endif

#ifndef ECS_ADD_COMPONENT_DATA
#define ECS_ADD_COMPONENT_DATA(ENTITY, COMPONENT, DATA) \
(ecs_add(s_app->ecs, ENTITY, ECS_GET_COMPONENT_ID(COMPONENT), DATA), \
ecs_get(s_app->ecs, ENTITY, ECS_GET_COMPONENT_ID(COMPONENT)))
#endif

#ifndef ECS_GET_COMPONENT
#define ECS_GET_COMPONENT(ENTITY, COMPONENT) \
(ecs_has(s_app->ecs, ENTITY, ECS_GET_COMPONENT_ID(COMPONENT)) ? \
ecs_get(s_app->ecs, ENTITY, ECS_GET_COMPONENT_ID(COMPONENT)) : \
NULL)
#endif

#ifndef ECS_GET_SYSTEM_ID
#define ECS_GET_SYSTEM_ID(NAME) ( cf_hashtable_get(s_app->system_ids, cf_sintern(#NAME)) )
#endif

#ifndef ECS_REGISTER_SYSTEM
#define ECS_REGISTER_SYSTEM(FUNC, OUT_ID) \
{ \
OUT_ID = ecs_register_system(s_app->ecs, FUNC, NULL, NULL, NULL); \
cf_hashtable_set(s_app->system_ids, cf_sintern(#FUNC), OUT_ID); \
}
#endif

#ifndef ECS_REGISTER_SYSTEM_CB
#define ECS_REGISTER_SYSTEM_CB(FUNC, OUT_ID, ON_ADD, ON_REMOVE) \
{ \
OUT_ID = ecs_register_system(s_app->ecs, FUNC, ON_ADD, ON_REMOVE, NULL); \
cf_hashtable_set(s_app->system_ids, cf_sintern(#FUNC), OUT_ID); \
}
#endif


void ecs_init();

// ---------------
// components
// ---------------


void component_navigation_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args);
void component_navigation_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr);
void component_ai_patrol_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args);
void component_ai_patrol_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr);
void component_control_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args);
void component_control_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr);
void component_projectile_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr);
void component_emoter_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args);
void component_emote_constructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr, void* args);
void component_emote_destructor(ecs_t* ecs, ecs_id_t entity_id, void* ptr);

// ---------------
// system updates
// ---------------

ecs_ret_t system_handle_events(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_pre_transform_update(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_pre_elevation_update(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_pre_health_update(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_level_elevation_offsets(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_elevation_effectors(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_jumper(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_control_unit_selection(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_control_camera_focus(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_control_ui_selection(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_control_navigation_input(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_control_navigation_validation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_control_fire_input(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_ai(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_ai_view(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_ai_view_check(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_pre_ai_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_ai_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_navigation_validation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_ai_move_rate(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_prop_transforms(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_level_exits(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_action_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_action_fire(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_elevation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_move(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_mover_navigation(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_grid_slot(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);

//  @todo:  add a on_remove() call back to call qt_remoe(ecs, entity)
ecs_ret_t system_update_unit_colliders(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
void system_update_unit_colliders_on_remove(ecs_t* ecs, ecs_id_t entity_id, void* udata);

ecs_ret_t system_update_projectiles(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_projectile_hits(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_pickup_hits(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_life_times(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_healths(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_child_transforms(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_unit_sprites(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_sprites(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_demo_spectate(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_update_camera(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);

// ---------------
// system draws
// ---------------

// used when units or projectiles are moving from tile to tile to avoid being clipped by a front or side
// occluding tile
#define DRAW_OCCLUSION_OFFSET (2)

ecs_ret_t system_draw_background(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_setup_camera(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_level_tile(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_ai_view(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_control_preview_path(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_control_path(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_unit_tile(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_decals(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_unit_shadow_blobs(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_props(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_units(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_emotes(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_projectiles(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_editor(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);
ecs_ret_t system_draw_canvas_composite(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata);

// ---------------
// system utils
// ---------------

// this assumes the targets passed in are all ready overlapping either from collider or tile standpoint
// the checks here is to make sure elevations are close enough to be considered touching
s32 get_closest_target_elevation_touches(ecs_t* ecs, dyna ecs_id_t* targets, f32 elevation, pq ecs_id_t* out_targets);
void set_elevation_value(C_Elevation* elevation, f32 value);
void set_unit_transform_tile(C_Unit_Transform* unit_transform, V2i value);
void set_life_time_duration(C_Life_Time* life_time, f32 duration);
void setup_ai_patrol(C_AI_Patrol* ai_patrol, V2i tile, s32 patrol_distance);

// ---------------
// entity spawning
// ---------------

ecs_id_t make_entity(V2i tile, const char* name);
ecs_id_t make_emote(ecs_id_t owner, const char* sprite_name, const char* emote_name, CF_V2 offset);
ecs_id_t make_projectile(ecs_id_t owner, V2i start, V2i end, f32 elevation, s32 distance, const char* name);
ecs_id_t make_paintball_decal(CF_V2 position, f32 elevation, const char* name);
ecs_id_t make_elevation_effector(V2i tile, f32 impulse, f32 start_radius, f32 end_radius, b32 ignore_start_tile);

// pickups
ecs_id_t make_pickup(V2i tile, Pickup_Params params);
ecs_id_t make_pickup_ammunition(V2i tile, s32 count);

void do_emote(ecs_id_t owner, Emoter_Rule rule);
void try_emote(ecs_id_t owner, Emoter_Rule rule);

// ---------------
// entity events
// ---------------

ecs_id_t make_event_ground_impact(V2i tile, f32 impact, b32 ignore_start_tile);
ecs_id_t make_event_load_level(const char* name);
ecs_id_t make_event_on_alert(ecs_id_t owner, ecs_id_t target);
ecs_id_t make_event_on_idle(ecs_id_t owner);
ecs_id_t make_event_on_hit(ecs_id_t owner, ecs_id_t target);
ecs_id_t make_event_on_kill(ecs_id_t owner, ecs_id_t target);
ecs_id_t make_event_on_dead(ecs_id_t owner);
ecs_id_t make_event_on_fire(ecs_id_t owner, CF_V2 position, V2i tile, f32 elevation);
ecs_id_t make_event_on_pickup(ecs_id_t owner, const char* asset_resource_name);
ecs_id_t make_event_do_select_control_unit(ecs_id_t select_entity);
ecs_id_t make_event_on_select_control_unit(ecs_id_t select_entity);
ecs_id_t make_event_on_deselect_control_unit(ecs_id_t select_entity);

#endif //ENTITY_H

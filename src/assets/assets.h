#ifndef ASSETS_H
#define ASSETS_H

#ifndef ASSETS_FILE_WRITE_STRING
#define ASSETS_FILE_WRITE_STRING(FILE, DATA) { \
s32 __length = (s32)CF_STRLEN(DATA); \
cf_fs_write(FILE, &__length, sizeof(__length)); \
cf_fs_write(FILE, DATA, __length); \
}
#endif

#ifndef ASSETS_BUF_READ
#define ASSETS_BUF_READ(BUF, DATA, SIZE) { \
CF_MEMCPY(DATA, BUF, SIZE); \
BUF += SIZE; \
}
#endif

#ifndef ASSETS_BUF_READ_STRING
#define ASSETS_BUF_READ_STRING(BUF, DATA) { \
s32 __length = 0; \
CF_MEMCPY(&__length, BUF, sizeof(__length)); \
BUF += sizeof(__length); \
CF_MEMCPY(DATA, BUF, __length); \
BUF += __length; \
DATA[__length] = '\0'; \
}
#endif


#ifndef LEVEL_FILE_TYPE_SUFFIX
#define LEVEL_FILE_TYPE_SUFFIX ".ipl"
#endif

// used for tile and object layer maps, this will map from runtime asset ids to
// local ones to each level save file
typedef s32 Asset_Object_ID;

// list of sprite animations with prefixes
// all animation sets are expected to have a `_` delimiter at the end of each
// tag to be considered as part of a set, otherwise animation will be stand alone
// example a sprite with the following tags
//   alert_0
//   alert_1
//   dead_0
//   dead_1
//   dead_2
//   excited_0
//   jumping

// here you'll get a set of names with
//   [alert_0, alert_1]
//   [dead_0, dead_1, dead_2]
//   [excited_0]
//   [jumping]

//  @note:  all strings handled by assets are expected to be interned at some point
//          since it relies on using cf_htbl

typedef struct Sprite_Reference
{
    const char* sprite;
    const char* animation;
} Sprite_Reference;

typedef s32 Credits_Command_Type;
enum
{
    Credits_Command_Type_None,
    Credits_Command_Type_Text,
    Credits_Command_Type_Sprite,
    Credits_Command_Type_Font_Size
};

typedef struct Credits_Command
{
    const char* text;
    const char* sprite_name;
    const char* animation_name;
    CF_Sprite* sprite;
    f32 font_size;
    Credits_Command_Type type;
} Credits_Command;

typedef s32 Asset_Resource_Type;
enum
{
    Asset_Resource_Type_None,
    Asset_Resource_Type_App,
    Asset_Resource_Type_Entity,
    Asset_Resource_Type_Tile,
    Asset_Resource_Type_UI,
    Asset_Resource_Type_Demo,
    Asset_Resource_Type_Campaign,
    Asset_Resource_Type_Editor,
};

typedef struct Property
{
    const char* key;
    void* value;
    s32 size;
} Property;

typedef struct Asset_Resource
{
    b32 initialized;
    Asset_Resource_Type type;
    Asset_Object_ID id;
    const char* file;
    const char* name;
    struct
    {
        Sprite_Reference reference;
        CF_V2 scale;
        const char* display_name;
        const char* description;
        const char* category;
    } editor;
    dyna Property* properties;
} Asset_Resource;

typedef s32 Asset_Type;
enum
{
    Asset_Type_None,
    Asset_Type_Sound,
    Asset_Type_Sprite,
    Asset_Type_PNG
};

typedef struct Asset
{
    Asset_Type type;
    const char* path;
    s32 ref_count;
    union
    {
        CF_Audio sound;
        CF_Sprite sprite;
    };
} Asset;

typedef struct Assets
{
    cf_htbl Asset* assets;
    
    cf_htbl Asset_Resource* resources;
    // revserse resource lookup based on ids
    cf_htbl Asset_Resource* resource_ids;
    
    CF_V2 tile_size;
    
    CF_Rnd rnd;
} Assets;

char* mount_get_directory_path();
void mount_data_read_directory();
void mount_data_write_directory();
void mount_root_read_directory();
void mount_root_write_directory();
void dismount_data_directory();
void dismount_root_directory();

void assets_load_all();
CF_Audio* assets_get_sound(const char* name);
// for most cases any `.ase` sprite does not need this and can call
// cf_make_sprite() directly with the sprite's virtual path
// but since we're also loading PNGs these needs to hold some sort of
// state so it can be accessed or cleaned up later on
CF_Sprite* assets_get_sprite(const char* name);

CF_Audio* assets_load_sound(const char* file_name);
CF_Sprite* assets_load_sprite(const char* file_name);
CF_Sprite* assets_load_png(const char* file_name);

// make sure these assets are not being used when unloading, 
// can cause a crash if using these after unloading
void assets_unload_sound(const char* name);
void assets_unload_sprite(const char* name);
void assets_unload_png(const char* name);

//  @todo:  this should be a known value, convert this to a macro for release
CF_V2 assets_get_tile_size();

Asset_Resource* assets_get_resource_from_id(Asset_Object_ID id);
Asset_Resource* assets_get_resource(const char* name);
fixed Asset_Resource** assets_get_resources_of_type(Asset_Resource_Type type);

void* assets_get_resource_property_value(const char* name, const char* property_key);
void* resource_get(Asset_Resource* resource, const char* name);
cf_htbl struct Event_Reaction_Info** resource_get_event_reactions(Asset_Resource* resource);
void property_copy_to(Property* property, void* data);

// all fields are required except for @optional ones
typedef struct Save_Level_Params
{
    u64 version;
    const char* file_name;
    const char* name;
    // @optional
    const char* music_file_name;
    // @optional
    const char* background_file_name;
    V2i size;
    struct Tile* tiles;
    const char** layer_names;
    Asset_Object_ID** layers;
    s32 layer_count;
    
    dyna struct Switch_Link* switch_links;
    V2i camera_tile;
} Save_Level_Params;

typedef struct Load_Level_Result
{
    u64 version;
    V2i size;
    s32 tile_count;
    s32 layer_count;
    const char* file_name;
    const char* name;
    // @optional
    const char* music_file_name;
    // @optional
    const char* background_file_name;
    fixed const char** object_names;
    fixed struct Tile* tiles;
    
    fixed const char** layer_names;
    fixed Asset_Object_ID** layers;
    
    // for world loading only, to avoid having to run through
    // every single index to figure out if there's an entity to
    // spawn
    fixed V2i* entity_tiles;
    fixed Asset_Object_ID* entity_resource_ids;
    // addes as part of v3
    // @optional
    fixed struct Switch_Link* switch_links;
    V2i camera_tile;
    b32 success;
} Load_Level_Result;

b32 save_level(Save_Level_Params params);
Load_Level_Result load_level(const char* file_name);
void load_level_version_1(u8* file, u8* data, u64 file_size, Load_Level_Result* result);
// added RLE to tiles and asset resource ids
void load_level_version_2(u8* file, u8* data, u64 file_size, Load_Level_Result* result);
// added switch links
void load_level_version_3(u8* file, u8* data, u64 file_size, Load_Level_Result* result);

#endif //ASSETS_H

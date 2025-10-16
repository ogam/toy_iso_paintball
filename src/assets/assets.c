#include "assets/assets.h"

char* mount_get_directory_path()
{
    //  mounting vfs
    char *path = cf_path_normalize(cf_fs_get_base_directory());
    char *dir = cf_path_directory_of(path);
    s32 directory_depth = 0;
    //  running from debugger
    if (cf_string_equ(dir, "/build"))
    {
        directory_depth = 2;
    }
    //  running from debug build
    else if (cf_string_equ(dir, "/Debug") || cf_string_equ(dir, "/Release"))
    {
        directory_depth = 2;
    }
    cf_string_free(dir);
    path = cf_path_pop_n(path, directory_depth);
    cf_string_append(path, "/data");
    
    return path;
}

void mount_data_directory()
{
    char *path = mount_get_directory_path();
    cf_fs_mount(path, "/", false);
    cf_string_free(path);
}

void mount_write_directory()
{
    char *path = mount_get_directory_path();
    cf_string_append(path, "/");
    cf_fs_set_write_directory(path);
    cf_string_free(path);
}

// ---------------
// json utils
// ---------------

#define JSON_GET_BOOL(OBJ, NAME) cf_json_get_bool(cf_json_get(OBJ, NAME))
#define JSON_GET_INT(OBJ, NAME) cf_json_get_int(cf_json_get(OBJ, NAME))
#define JSON_GET_FLOAT(OBJ, NAME) cf_json_get_float(cf_json_get(OBJ, NAME))
#define JSON_GET_STRING(OBJ, NAME) cf_json_get_string(cf_json_get(OBJ, NAME))
#define JSON_GET_STRING_INTERN(OBJ, NAME) cf_sintern(cf_json_get_string(cf_json_get(OBJ, NAME)))
#define JSON_GET_STRING_COPY(OBJ, NAME) parse_string_copy(OBJ, NAME)
#define JSON_GET_STRING_ARRAY(OBJ, NAME) parse_string_array(OBJ, NAME, Parse_String_Array_Mode_Default)
#define JSON_GET_STRING_INTERN_ARRAY(OBJ, NAME) parse_string_array(OBJ, NAME, Parse_String_Array_Mode_Intern)
#define JSON_GET_STRING_COPY_ARRAY(OBJ, NAME) parse_string_array(OBJ, NAME, Parse_String_Array_Mode_Copy)

typedef s32 Parse_String_Array_Mode;
enum
{
    Parse_String_Array_Mode_Default,
    Parse_String_Array_Mode_Intern,
    Parse_String_Array_Mode_Copy,
};

dyna const char** parse_string_array(CF_JVal obj, const char* key, Parse_String_Array_Mode mode)
{
    dyna const char** strings = NULL;
    obj = cf_json_get(obj, key);
    
    if (cf_json_is_array(obj))
    {
        for (CF_JIter it = cf_json_iter(obj); 
             !cf_json_iter_done(it); 
             it = cf_json_iter_next(it))
        {
            CF_JVal val_obj = cf_json_iter_val(it);
            if (!cf_json_is_string(val_obj))
            {
                break;
            }
            
            const char* value = cf_json_get_string(val_obj);
            if (mode == Parse_String_Array_Mode_Intern)
            {
                value = cf_sintern(value);
            }
            else if (mode == Parse_String_Array_Mode_Copy)
            {
                s32 length = cf_json_get_len(val_obj);
                char* temp = cf_alloc(length);
                CF_MEMCPY(temp, value, length);
                temp[length] = '\0';
                value = temp;
            }
            
            cf_array_push(strings, value);
        }
    }
    return strings;
}

const char* parse_string_copy(CF_JVal obj, const char* key)
{
    const char* result = NULL;
    obj = cf_json_get(obj, key);
    
    if (cf_json_is_string(obj))
    {
        s32 length = cf_json_get_len(obj);
        char* temp = cf_alloc(length);
        const char* value = cf_json_get_string(obj);
        CF_MEMCPY(temp, value, length);
        temp[length] = '\0';
        result = temp;
    }
    
    return result;
}

Sprite_Reference parse_sprite_reference(CF_JVal obj)
{
    Sprite_Reference reference = { 0 };
    if (cf_json_is_object(obj))
    {
        reference.sprite = JSON_GET_STRING_INTERN(obj, "sprite");
        reference.animation = JSON_GET_STRING_INTERN(obj, "animation");
    }
    return reference;
}

dyna Credits_Command* parse_credits(CF_JVal obj)
{
    dyna Credits_Command* commands = NULL;
    if (cf_json_is_array(obj))
    {
        for (CF_JIter it = cf_json_iter(obj); 
             !cf_json_iter_done(it); 
             it = cf_json_iter_next(it))
        {
            CF_JVal val_obj = cf_json_iter_val(it);
            if (cf_json_is_object(val_obj))
            {
                Credits_Command command = { 
                    .text = JSON_GET_STRING_COPY(val_obj, "text"),
                    .sprite_name = JSON_GET_STRING_COPY(val_obj, "sprite"),
                    .animation_name = JSON_GET_STRING_COPY(val_obj, "animation"),
                    .font_size = JSON_GET_FLOAT(val_obj, "font_size"),
                };
                
                const char* type_str = JSON_GET_STRING_COPY(val_obj, "type");
                if (type_str)
                {
                    if (cf_string_equ(type_str, "font_size"))
                    {
                        command.type = Credits_Command_Type_Font_Size;
                    }
                }
                else if (command.text)
                {
                    command.type = Credits_Command_Type_Text;
                }
                else if (command.sprite_name)
                {
                    command.type = Credits_Command_Type_Sprite;
                }
                
                if (command.type)
                {
                    cf_array_push(commands, command);
                }
            }
        }
    }
    return commands;
}

// ---------------
// app
// ---------------

void parse_app(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal obj = cf_json_get(root, "app");
    
    {
        Property property = 
        {
            .key = cf_sintern("icon"),
            .value = (void*)JSON_GET_STRING_COPY(obj, "icon"),
            .size = sizeof(const char*),
        };
        cf_array_push(resource->properties, property);
    }
    {
        dyna Credits_Command* commands = parse_credits(cf_json_get(obj, "credits"));
        if (cf_array_count(commands))
        {
            Property property = 
            {
                .key = cf_sintern("credits"),
                .value = commands,
                .size = sizeof(Credits_Command*),
            };
            cf_array_push(resource->properties, property);
        }
    }
    
    resource->initialized = true;
}

// ---------------
// entity
// ---------------

// components that has no meta data that can be assigned
void parse_component_stub(CF_JVal obj, Asset_Resource* resource, const char* name)
{
    UNUSED(obj);
    Property property = {
        .key = cf_sintern(name),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_sprite(CF_JVal obj, Asset_Resource* resource)
{
    C_Sprite* sprite = cf_calloc(sizeof(C_Sprite), 1);
    *sprite = (C_Sprite){
        .name = JSON_GET_STRING_INTERN(obj, "name"),
        .scale = {
            .x = JSON_GET_FLOAT(obj, "scale_x"),
            .y = JSON_GET_FLOAT(obj, "scale_y"),
        },
    };
    
    CF_JVal animations = cf_json_get(obj, "animations");
    
    for (CF_JIter it = cf_json_iter(animations); 
         !cf_json_iter_done(it); 
         it = cf_json_iter_next(it))
    {
        const char* key = cf_json_iter_key(it);
        CF_JVal val_obj = cf_json_iter_val(it);
        
        if (cf_json_is_string(val_obj))
        {
            cf_hashtable_set(sprite->animations, cf_sintern(key), cf_sintern(cf_json_get_string(val_obj)));
        }
    }
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_Sprite)),
        .value = sprite,
        .size = sizeof(C_Sprite),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_mover(CF_JVal obj, Asset_Resource* resource)
{
    C_Mover* mover = cf_calloc(sizeof(C_Mover), 1);
    *mover = (C_Mover){
        .move_rate = JSON_GET_FLOAT(obj, "move_rate"),
        .next_move_rate = JSON_GET_FLOAT(obj, "move_rate"),
    };
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_Mover)),
        .value = mover,
        .size = sizeof(C_Mover),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_ai(CF_JVal obj, Asset_Resource* resource)
{
    C_AI* ai = cf_calloc(sizeof(C_AI), 1);
    *ai = (C_AI){
        .aim_distance = JSON_GET_INT(obj, "aim_distance"),
        .patrol_move_rate = JSON_GET_FLOAT(obj, "patrol_move_rate"),
        .chase_move_rate = JSON_GET_FLOAT(obj, "chase_move_rate"),
        .aim_move_rate = JSON_GET_FLOAT(obj, "aim_move_rate"),
        .disengage_distance = JSON_GET_INT(obj, "disengage_distance"),
        .alert_radius = JSON_GET_INT(obj, "alert_radius"),
    };
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_AI)),
        .value = ai,
        .size = sizeof(C_AI),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_ai_patrol(CF_JVal obj, Asset_Resource* resource)
{
    C_AI_Patrol* ai_patrol = cf_calloc(sizeof(C_AI_Patrol), 1);
    *ai_patrol= (C_AI_Patrol){
        .patrol_distance = JSON_GET_INT(obj, "patrol_distance"),
    };
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_AI_Patrol)),
        .value = ai_patrol,
        .size = sizeof(C_AI_Patrol),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_ai_view(CF_JVal obj, Asset_Resource* resource)
{
    C_AI_View* ai_view = cf_calloc(sizeof(C_AI_View), 1);
    *ai_view = (C_AI_View){
        .distance = JSON_GET_INT(obj, "distance"),
    };
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_AI_View)),
        .value = ai_view,
        .size = sizeof(C_AI_View),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_weapon(CF_JVal obj, Asset_Resource* resource)
{
    C_Weapon* weapon = cf_calloc(sizeof(C_Weapon), 1);
    *weapon = (C_Weapon){
        .projectiles_per_fire = JSON_GET_INT(obj, "projectiles_per_fire"),
        .cost_per_fire = JSON_GET_INT(obj, "cost_per_fire"),
        .projectile_distance = JSON_GET_INT(obj, "projectile_distance"),
        .ammunition = JSON_GET_INT(obj, "ammunition"),
        .max_ammunition = JSON_GET_INT(obj, "max_ammunition"),
        .accuracy = JSON_GET_INT(obj, "accuracy"),
        .fire_delay = JSON_GET_FLOAT(obj, "fire_delay"),
        .fire_rate = JSON_GET_FLOAT(obj, "fire_rate"),
        .projectile_name = JSON_GET_STRING_INTERN(obj, "projectile_name"),
    };
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_Weapon)),
        .value = weapon,
        .size = sizeof(C_Weapon),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_decal(CF_JVal obj, Asset_Resource* resource)
{
    C_Decal* decal = cf_calloc(sizeof(C_Decal), 1);
    *decal = (C_Decal){
        .fade_delay = JSON_GET_FLOAT(obj, "fade_delay"),
    };
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_Decal)),
        .value = decal,
        .size = sizeof(C_Decal),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_emoter_rule(CF_JVal obj, Emoter_Rule* rule)
{
    rule->chance = JSON_GET_FLOAT(obj, "chance");
    CF_JVal emotes = cf_json_get(obj, "emotes");
    
    if (cf_json_is_array(emotes))
    {
        for (CF_JIter it = cf_json_iter(emotes); 
             !cf_json_iter_done(it); 
             it = cf_json_iter_next(it))
        {
            CF_JVal val = cf_json_iter_val(it);
            Sprite_Reference reference = parse_sprite_reference(val); 
            cf_array_push(rule->emotes, reference);
        }
    }
}

void parse_component_emoter(CF_JVal obj, Asset_Resource* resource)
{
    C_Emoter* emoter = cf_calloc(sizeof(C_Emoter), 1);
    *emoter = (C_Emoter){
        .scale = {
            .x = JSON_GET_FLOAT(obj, "scale_x"),
            .y = JSON_GET_FLOAT(obj, "scale_y"),
        }
    };
    
    
    for (CF_JIter it = cf_json_iter(obj); 
         !cf_json_iter_done(it); 
         it = cf_json_iter_next(it))
    {
        const char* key = cf_json_iter_key(it);
        CF_JVal val_obj = cf_json_iter_val(it);
        key = cf_sintern(key);
        if (key == cf_sintern("on_alert") && cf_json_is_object(val_obj))
        {
            parse_emoter_rule(val_obj, &emoter->on_alert);
        }
        else if (key == cf_sintern("on_idle") && cf_json_is_object(val_obj))
        {
            parse_emoter_rule(val_obj, &emoter->on_idle);
        }
        else if (key == cf_sintern("on_hit") && cf_json_is_object(val_obj))
        {
            parse_emoter_rule(val_obj, &emoter->on_hit);
        }
        else if (key == cf_sintern("on_kill") && cf_json_is_object(val_obj))
        {
            parse_emoter_rule(val_obj, &emoter->on_kill);
        }
        else if (key == cf_sintern("on_dead") && cf_json_is_object(val_obj))
        {
            parse_emoter_rule(val_obj, &emoter->on_dead);
        }
    }
    
    Property property = {
        .key = cf_sintern(CF_STRINGIZE(C_Emoter)),
        .value = emoter,
        .size = sizeof(C_Emoter),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_life_time(CF_JVal obj, Asset_Resource* resource)
{
    C_Life_Time* life_time = cf_calloc(sizeof(C_Life_Time), 1);
    *life_time = (C_Life_Time){
        .duration = JSON_GET_FLOAT(obj, "value"),
        .total = JSON_GET_FLOAT(obj, "value"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Life_Time)),
        .value = life_time,
        .size = sizeof(C_Life_Time),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_health(CF_JVal obj, Asset_Resource* resource)
{
    C_Health* health = cf_calloc(sizeof(C_Health), 1);
    *health = (C_Health){
        .value = JSON_GET_INT(obj, "value"),
        .prev_value = JSON_GET_INT(obj, "value"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Health)),
        .value = health,
        .size = sizeof(C_Health),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_pickup(CF_JVal obj, Asset_Resource* resource)
{
    C_Pickup* pickup = cf_calloc(sizeof(C_Pickup), 1);
    *pickup = (C_Pickup){
        .count = JSON_GET_INT(obj, "count"),
    };
    
    const char* type_val = JSON_GET_STRING_INTERN(obj, "type");
    if (type_val == cf_sintern("ammunition"))
    {
        pickup->type = PickupType_Ammunition;
    }
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Pickup)),
        .value = pickup,
        .size = sizeof(C_Pickup),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_sound_source(CF_JVal obj, Asset_Resource* resource)
{
    C_Sound_Source* sound_source = cf_calloc(sizeof(C_Sound_Source), 1);
    
    *sound_source = (C_Sound_Source){
        .on_fire = JSON_GET_STRING_INTERN_ARRAY(obj, "on_fire"),
        .on_alert = JSON_GET_STRING_INTERN_ARRAY(obj, "on_alert"),
        .on_idle = JSON_GET_STRING_INTERN_ARRAY(obj, "on_idle"),
        .on_hit_taken = JSON_GET_STRING_INTERN_ARRAY(obj, "on_hit_taken"),
        .on_dead = JSON_GET_STRING_INTERN_ARRAY(obj, "on_dead"),
        .on_pickup = JSON_GET_STRING_INTERN_ARRAY(obj, "on_pickup"),
    };
    
    const char* type_key = JSON_GET_STRING_INTERN(obj, "type");
    sound_source->type = Audio_Source_Type_SFX;
    if (type_key == cf_sintern("sfx"))
    {
        sound_source->type = Audio_Source_Type_SFX;
    }
    else if (type_key == cf_sintern("music"))
    {
        sound_source->type = Audio_Source_Type_Music;
    }
    else if (type_key == cf_sintern("ui"))
    {
        sound_source->type = Audio_Source_Type_UI;
    }
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Sound_Source)),
        .value = sound_source,
        .size = sizeof(C_Sound_Source),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_spawner(CF_JVal obj, Asset_Resource* resource)
{
    C_Spawner* spawner = cf_calloc(sizeof(C_Spawner), 1);
    
    *spawner = (C_Spawner){
        .on_destroy = JSON_GET_STRING_INTERN_ARRAY(obj, "on_destroy"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Spawner)),
        .value = spawner,
        .size = sizeof(C_Spawner),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_jumper(CF_JVal obj, Asset_Resource* resource)
{
    C_Jumper* jumper = cf_calloc(sizeof(C_Jumper), 1);
    
    *jumper = (C_Jumper){
        .interval = JSON_GET_FLOAT(obj, "interval"),
        .impulse = JSON_GET_FLOAT(obj, "impulse"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Jumper)),
        .value = jumper,
        .size = sizeof(C_Jumper),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_ui(CF_JVal obj, Asset_Resource* resource)
{
    C_UI* c_ui = cf_calloc(sizeof(C_UI), 1);
    
    *c_ui = (C_UI){
        .select = parse_sprite_reference(cf_json_get(obj, "select")),
        .deselect = parse_sprite_reference(cf_json_get(obj, "deselect")),
        .leader = parse_sprite_reference(cf_json_get(obj, "leader")),
        .hover = parse_sprite_reference(cf_json_get(obj, "hover")),
        .dead = parse_sprite_reference(cf_json_get(obj, "dead")),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_UI)),
        .value = c_ui,
        .size = sizeof(C_UI),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_team(CF_JVal obj, Asset_Resource* resource)
{
    C_Team* team = cf_calloc(sizeof(C_Team), 1);
    
    *team = (C_Team){
        .id = JSON_GET_INT(obj, "id"),
        .friendly_fire = JSON_GET_BOOL(obj, "friendly_fire"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Team)),
        .value = team,
        .size = sizeof(C_Team),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_switch(CF_JVal obj, Asset_Resource* resource)
{
    C_Switch* c_switch = cf_calloc(sizeof(C_Switch), 1);
    
    *c_switch = (C_Switch){
        .reset_time = JSON_GET_FLOAT(obj, "reset_time"),
        .mask = JSON_GET_INT(obj, "mask"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Switch)),
        .value = c_switch,
        .size = sizeof(C_Switch),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_component_tile_switch(CF_JVal obj, Asset_Resource* resource)
{
    C_Tile_Switch* tile_switch = cf_calloc(sizeof(C_Tile_Switch), 1);
    
    *tile_switch = (C_Tile_Switch){
        .is_filler = JSON_GET_BOOL(obj, "is_filler"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Tile_Switch)),
        .value = tile_switch,
        .size = sizeof(C_Tile_Switch),
    };
    
    cf_array_push(resource->properties, property);
}


void parse_component_bounce_pad(CF_JVal obj, Asset_Resource* resource)
{
    C_Bounce_Pad* bounce_pad = cf_calloc(sizeof(C_Bounce_Pad), 1);
    
    *bounce_pad = (C_Bounce_Pad){
        .impulse = JSON_GET_FLOAT(obj, "impulse"),
        .restitution = JSON_GET_FLOAT(obj, "restitution"),
    };
    
    Property property = 
    {
        .key = cf_sintern(CF_STRINGIZE(C_Bounce_Pad)),
        .value = bounce_pad,
        .size = sizeof(C_Bounce_Pad),
    };
    
    cf_array_push(resource->properties, property);
}

void parse_entity(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal component_obj = cf_json_get(root, "components");
    
    // every entity has a reverse resource lookup
    {
        C_Asset_Resource* asset_resource = cf_calloc(sizeof(C_Asset_Resource), 1);
        *asset_resource = (C_Asset_Resource){
            .name = resource->name,
        };
        Property property = 
        {
            .key = cf_sintern(CF_STRINGIZE(C_Asset_Resource)),
            .value = asset_resource,
            .size = sizeof(C_Asset_Resource),
        };
        cf_array_push(resource->properties, property);
    }
    
    for (CF_JIter component_it = cf_json_iter(component_obj); 
         !cf_json_iter_done(component_it); 
         component_it = cf_json_iter_next(component_it))
    {
        const char* key = cf_json_iter_key(component_it);
        CF_JVal val_obj = cf_json_iter_val(component_it);
        key = cf_sintern(key);
        if (key == cf_sintern(CF_STRINGIZE(C_Sprite)))
        {
            parse_component_sprite(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Transform)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Transform));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Child_Transform)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Child_Transform));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Unit_Transform)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Unit_Transform));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Mover)))
        {
            parse_component_mover(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Navigation)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Navigation));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Action)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Action));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_AI)))
        {
            parse_component_ai(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_AI_Patrol)))
        {
            parse_component_ai_patrol(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_AI_View)))
        {
            parse_component_ai_view(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Control)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Control));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Weapon)))
        {
            parse_component_weapon(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Projectile)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Projectile));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Camera_Focus)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Camera_Focus));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Life_Time)))
        {
            parse_component_life_time(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Collider)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Collider));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Health)))
        {
            parse_component_health(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Corpse)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Corpse));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Elevation)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Elevation));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Elevation_Effector)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Elevation_Effector));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Decal)))
        {
            parse_component_decal(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Level_Exit)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Level_Exit));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Collider)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Collider));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Emoter)))
        {
            parse_component_emoter(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Emote)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Emote));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Pickup)))
        {
            parse_component_pickup(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Sound_Source)))
        {
            parse_component_sound_source(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Prop)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Prop));
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Spawner)))
        {
            parse_component_spawner(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Jumper)))
        {
            parse_component_jumper(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_UI)))
        {
            parse_component_ui(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Team)))
        {
            parse_component_team(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Switch)))
        {
            parse_component_switch(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Tile_Switch)))
        {
            parse_component_tile_switch(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Bounce_Pad)))
        {
            parse_component_bounce_pad(val_obj, resource);
        }
        else if (key == cf_sintern(CF_STRINGIZE(C_Surface_Icy)))
        {
            parse_component_stub(val_obj, resource, CF_STRINGIZE(C_Surface_Icy));
        }
    }
    
    resource->initialized = true;
}

// ---------------
// tile
// ---------------

void parse_tile(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal tile_obj = cf_json_get(root, "tile");
    
    // walkable property is always available for all tile resources
    {
        Property property = 
        {
            .key = cf_sintern("walkable"),
            .value = cf_calloc(sizeof(s8), 1),
            .size = sizeof(s8)
        };
        *(s8*)property.value = false;
        cf_array_push(resource->properties, property);
    }
    
    {
        Property property = 
        {
            .key = cf_sintern("filler"),
            .value = cf_calloc(sizeof(s8), 1),
            .size = sizeof(s8)
        };
        *(s8*)property.value = false;
        cf_array_push(resource->properties, property);
    }
    
    // switches property is alawys available
    {
        Property property = 
        {
            .key = cf_sintern("switches"),
            .value = cf_calloc(sizeof(s8), 1),
            .size = sizeof(s8)
        };
        *(s8*)property.value = 0;
        cf_array_push(resource->properties, property);
    }
    
    for (CF_JIter tile_it = cf_json_iter(tile_obj); 
         !cf_json_iter_done(tile_it); 
         tile_it = cf_json_iter_next(tile_it))
    {
        const char* key = cf_json_iter_key(tile_it);
        CF_JVal val_obj = cf_json_iter_val(tile_it);
        key = cf_sintern(key);
        
        if (key == cf_sintern("walkable") && cf_json_is_bool(val_obj))
        {
            s8* walkable = resource_get(resource, "walkable");
            *walkable = cf_json_get_bool(val_obj);
        }
        else if (key == cf_sintern("filler") && cf_json_is_bool(val_obj))
        {
            s8* filler = resource_get(resource, "filler");
            *filler = cf_json_get_bool(val_obj);
        }
        else if (key == cf_sintern("sprite") && cf_json_is_string(val_obj))
        {
            Property property = 
            {
                .key = key,
                .value = (void*)cf_sintern(cf_json_get_string(val_obj)),
                .size = sizeof(const char*),
            };
            cf_array_push(resource->properties, property);
        }
        else if (key == cf_sintern("animations") && cf_json_is_object(val_obj))
        {
            cf_htbl const char** animations = NULL;
            
            for (CF_JIter it = cf_json_iter(val_obj); 
                 !cf_json_iter_done(it); 
                 it = cf_json_iter_next(it))
            {
                const char* animation_key = cf_json_iter_key(it);
                CF_JVal animation_val_obj = cf_json_iter_val(it);
                
                if (cf_json_is_string(animation_val_obj))
                {
                    cf_hashtable_set(animations, cf_sintern(animation_key), cf_sintern(cf_json_get_string(animation_val_obj)));
                }
            }
            
            Property property = 
            {
                .key = key,
                .value = animations,
                .size = sizeof(const char**)
            };
            cf_array_push(resource->properties, property);
        }
        else if (key == cf_sintern("switches") && cf_json_is_int(val_obj))
        {
            s8* switches = resource_get(resource, "switches");
            *switches = (s8)cf_json_get_int(val_obj);
        }
        
        CF_V2 *scale = cf_alloc(sizeof(CF_V2));
        *scale = cf_v2(JSON_GET_FLOAT(tile_obj, "scale_x"),
                       JSON_GET_FLOAT(tile_obj, "scale_y"));
        
        Property property = 
        {
            .key = cf_sintern("scale"),
            .value = scale,
            .size = sizeof(CF_V2)
        };
        cf_array_push(resource->properties, property);
    }
    
    resource->initialized = true;
}

// ---------------
// ui
// ---------------

void parse_ui_element(const char* element_name, CF_JVal obj, Asset_Resource* resource)
{
    //  @todo:  if we end up with more stuff maybe just have this linked to sub properties
    cf_htbl const char*** data = NULL;
    
    for (CF_JIter it = cf_json_iter(obj); 
         !cf_json_iter_done(it); 
         it = cf_json_iter_next(it))
    {
        const char* key = cf_json_iter_key(it);
        CF_JVal val_obj = cf_json_iter_val(it);
        key = cf_sintern(key);
        
        // assume string array
        if (cf_json_is_array(val_obj))
        {
            cf_hashtable_set(data, key, JSON_GET_STRING_INTERN_ARRAY(obj, key));
        }
    }
    
    Property property = 
    {
        .key = cf_sintern(element_name),
        .value = data,
        .size = sizeof(const char***)
    };
    cf_array_push(resource->properties, property);
}

void parse_ui(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal ui_obj = cf_json_get(root, "ui");
    
    for (CF_JIter it = cf_json_iter(ui_obj); 
         !cf_json_iter_done(it); 
         it = cf_json_iter_next(it))
    {
        const char* key = cf_json_iter_key(it);
        CF_JVal val_obj = cf_json_iter_val(it);
        key = cf_sintern(key);
        
        if (key == cf_sintern("fonts"))
        {
            Property property = 
            {
                .key = key,
                .value = JSON_GET_STRING_INTERN_ARRAY(ui_obj, key),
                .size = sizeof(const char**)
            };
            cf_array_push(resource->properties, property);
        }
        else if (cf_json_is_object(val_obj))
        {
            parse_ui_element(key, val_obj, resource);
        }
    }
    
    resource->initialized = true;
}

void parse_demo(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal obj = cf_json_get(root, "demo");
    
    Property property = 
    {
        .key = cf_sintern("levels"),
        .value = JSON_GET_STRING_COPY_ARRAY(obj, "levels"),
        .size = sizeof(const char**)
    };
    cf_array_push(resource->properties, property);
    
    resource->initialized = true;
}

void parse_campaign(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal obj = cf_json_get(root, "campaign");
    
    {
        Property property = 
        {
            .key = cf_sintern("levels"),
            .value = JSON_GET_STRING_COPY_ARRAY(obj, "levels"),
            .size = sizeof(const char**)
        };
        cf_array_push(resource->properties, property);
    }
    
    {
        Sprite_Reference* reference = cf_calloc(sizeof(Sprite_Reference), 1);
        *reference = parse_sprite_reference(cf_json_get(obj, "sprite"));
        
        Property property = 
        {
            .key = cf_sintern("sprite"),
            .value = reference,
            .size = sizeof(Sprite_Reference),
        };
        cf_array_push(resource->properties, property);
    }
    
    {
        dyna Credits_Command* commands = parse_credits(cf_json_get(obj, "credits"));
        if (cf_array_count(commands))
        {
            Property property = 
            {
                .key = cf_sintern("credits"),
                .value = commands,
                .size = sizeof(Credits_Command*),
            };
            cf_array_push(resource->properties, property);
        }
    }
    
    resource->initialized = true;
}

void parse_editor(CF_JVal root, Asset_Resource* resource)
{
    CF_JVal obj = cf_json_get(root, "editor");
    
    for (CF_JIter it = cf_json_iter(obj); 
         !cf_json_iter_done(it); 
         it = cf_json_iter_next(it))
    {
        const char* key = cf_json_iter_key(it);
        CF_JVal val_obj = cf_json_iter_val(it);
        key = cf_sintern(key);
        
        Sprite_Reference* reference = cf_calloc(sizeof(Sprite_Reference), 1);
        *reference = parse_sprite_reference(val_obj);
        
        Property property = 
        {
            .key = key,
            .value = reference,
            .size = sizeof(Sprite_Reference),
        };
        cf_array_push(resource->properties, property);
    }
    
    resource->initialized = true;
}

// more *s
void parse_asset_references(CF_JVal root, pq const char*** sprite_files, pq const char*** sound_files)
{
    for (CF_JIter root_it = cf_json_iter(root); 
         !cf_json_iter_done(root_it); 
         root_it = cf_json_iter_next(root_it))
    {
        CF_JVal val_obj = cf_json_iter_val(root_it);
        if (cf_json_is_string(val_obj))
        {
            const char* val = cf_json_get_string(val_obj);
            if (cf_path_ext_equ(val, ".ase"))
            {
                pq_add_weight(*sprite_files, cf_sintern(val), 1);
            }
            else if (cf_path_ext_equ(val, ".png"))
            {
                pq_add_weight(*sprite_files, cf_sintern(val), 1);
            }
            else if (cf_path_ext_equ(val, ".ogg"))
            {
                pq_add_weight(*sound_files, cf_sintern(val), 1);
            }
        }
        else if (cf_json_is_object(val_obj) || cf_json_is_array(val_obj))
        {
            parse_asset_references(val_obj, sprite_files, sound_files);
        }
    }
}

void assets_load_all()
{
    if (!s_app->assets)
    {
        s_app->assets = cf_calloc(sizeof(Assets), 1);
        s_app->assets->rnd = cf_rnd_seed(cf_get_tick_frequency());
    }
    
    Assets* assets = s_app->assets;
    mount_data_directory();
    
    Asset_Object_ID id = 1;
    // used to make sure all audio files are loaded and sanity check that sprites exist
    pq const char** sprite_files = NULL;
    pq const char** sound_files = NULL;
    pq_fit(sprite_files, 128);
    pq_fit(sound_files, 128);
    
    char path[1024];
    const char* directory = "meta";
    const char** files = cf_fs_enumerate_directory(directory);
    const char** file = files;
    while (*file)
    {
        // ignore files that are not the correct file extension or hidden files (`.`)
        if ((*file)[0] == '.' || !cf_path_ext_equ(*file, ".json"))
        {
            ++file;
            continue;
        }
        
        CF_SNPRINTF(path, sizeof(path), "%s/%s", directory, *file);
        
        // build out each json file as either an entity, tile, background or whatever else
        {
            u64 file_size = 0;
            CF_JDoc doc = cf_make_json_from_file(path);
            if (!doc.id)
            {
                printf("Failed to parse %s\n", path);
                goto JSON_CLEANUP;
            }
            CF_JVal root = cf_json_get_root(doc);
            
            Asset_Resource resource = { 0 };
            
            for (CF_JIter root_it = cf_json_iter(root); 
                 !cf_json_iter_done(root_it); 
                 root_it = cf_json_iter_next(root_it))
            {
                const char* key = cf_json_iter_key(root_it);
                key = cf_sintern(key);
                
                CF_JVal val_obj = cf_json_iter_val(root_it);
                if (key == cf_sintern("type") && cf_json_is_string(val_obj))
                {
                    const char* val = cf_sintern(cf_json_get_string(val_obj));
                    val = cf_sintern(val);
                    if (val == cf_sintern("app"))
                    {
                        resource.type = Asset_Resource_Type_App;
                    }
                    else if (val == cf_sintern("entity"))
                    {
                        resource.type = Asset_Resource_Type_Entity;
                    }
                    else if (val == cf_sintern("tile"))
                    {
                        resource.type = Asset_Resource_Type_Tile;
                    }
                    else if (val == cf_sintern("ui"))
                    {
                        resource.type = Asset_Resource_Type_UI;
                    }
                    else if (val == cf_sintern("demo"))
                    {
                        resource.type = Asset_Resource_Type_Demo;
                    }
                    else if (val == cf_sintern("campaign"))
                    {
                        resource.type = Asset_Resource_Type_Campaign;
                    }
                    else if (val == cf_sintern("editor"))
                    {
                        resource.type = Asset_Resource_Type_Editor;
                    }
                }
                else if (key == cf_sintern("name") && cf_json_is_string(val_obj))
                {
                    resource.name = JSON_GET_STRING_INTERN(root, "name");
                }
                else if (key == cf_sintern("editor") && cf_json_is_object(val_obj))
                {
                    resource.editor.reference.sprite = JSON_GET_STRING_INTERN(val_obj, "sprite");
                    resource.editor.reference.animation = JSON_GET_STRING_INTERN(val_obj, "animation");
                    resource.editor.scale = cf_v2(JSON_GET_FLOAT(val_obj, "scale_x"), 
                                                  JSON_GET_FLOAT(val_obj, "scale_y"));
                    resource.editor.display_name = JSON_GET_STRING_COPY(val_obj, "display_name");
                    resource.editor.description = JSON_GET_STRING_COPY(val_obj, "description");
                    resource.editor.category = JSON_GET_STRING_INTERN(val_obj, "category");
                }
            }
            
            switch (resource.type)
            {
                case Asset_Resource_Type_App: parse_app(root, &resource); break;
                case Asset_Resource_Type_Entity: parse_entity(root, &resource); break;
                case Asset_Resource_Type_Tile: parse_tile(root, &resource); break;
                case Asset_Resource_Type_UI: parse_ui(root, &resource); break;
                case Asset_Resource_Type_Demo: parse_demo(root, &resource); break;
                case Asset_Resource_Type_Campaign: parse_campaign(root, &resource); break;
                case Asset_Resource_Type_Editor: parse_editor(root, &resource); break;
                case Asset_Resource_Type_None:
                default:
                break;
            }
            
            if (!resource.initialized)
            {
                if (resource.properties)
                {
                    cf_array_free(resource.properties);
                }
            }
            
            // for debugging
            resource.file = cf_sintern(*file);
            resource.id = id++;
            cf_hashtable_set(assets->resources, resource.name, resource);
            cf_hashtable_set(assets->resource_ids, resource.id, resource);
            
            parse_asset_references(root, &sprite_files, &sound_files);
            
            JSON_CLEANUP:
            cf_destroy_json(doc);
        }
        
        ++file;
    }
    cf_fs_free_enumerated_directory(files);
    
    printf("Resource List:\n");
    for (s32 index = 0; index < pq_count(sprite_files); ++index)
    {
        printf("%-32s - %.0f\n", sprite_files[index], pq_weight_at(sprite_files, index));
    }
    for (s32 index = 0; index < pq_count(sound_files); ++index)
    {
        printf("%-32s - %.0f\n", sound_files[index], pq_weight_at(sound_files, index));
    }
    printf("Loaded resources - %d\n", cf_hashtable_count(assets->resources));
    
    // this is mainly needed for anything that has references to png files but
    // also any large ase files takes a while to load so might as well do it here
    // at the same time
    for (s32 index = 0; index < pq_count(sprite_files); ++index)
    {
        const char* sprite_name = sprite_files[index];
        if (!assets_load_sprite(sprite_name))
        {
            assets_load_png(sprite_name);
        }
    }
    for (s32 index = 0; index < pq_count(sound_files); ++index)
    {
        assets_load_sound(sound_files[index]);
    }
    
    Asset_Resource* resources = cf_hashtable_items(assets->resources);
    for (s32 index = 0; index < cf_hashtable_count(assets->resources); ++index)
    {
        Asset_Resource* resource = resources + index;
        if (resource->type == Asset_Resource_Type_Tile)
        {
            CF_V2* scale = resource_get(resource, "scale");
            const char* sprite_name = resource_get(resource, "sprite");
            CF_Sprite sprite = cf_make_sprite(sprite_name);
            if (sprite.name)
            {
                CF_V2 tile_size = cf_v2(sprite.w * scale->x, sprite.h * scale->y);
                assets->tile_size = cf_max_v2(assets->tile_size, tile_size);
            }
        }
    }
    
    pq_free(sprite_files);
    pq_free(sound_files);
}

CF_V2 assets_get_tile_size()
{
    return s_app->assets->tile_size;
}

CF_Audio* assets_get_sound(const char* name)
{
    Assets* assets = s_app->assets;
    CF_Audio* sound = NULL;
    
    name = cf_sintern(name);
    
    if (cf_hashtable_has(assets->assets, name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, name);
        if (asset->type == Asset_Type_Sound)
        {
            sound = &asset->sound;
        }
    }
    
    return sound;
}

CF_Sprite* assets_get_sprite(const char* name)
{
    Assets* assets = s_app->assets;
    CF_Sprite* sprite = NULL;
    
    name = cf_sintern(name);
    
    if (cf_hashtable_has(assets->assets, name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, name);
        if (asset->type == Asset_Type_Sprite || asset->type == Asset_Type_PNG)
        {
            sprite = &asset->sprite;
        }
    }
    
    return sprite;
}

CF_Audio* assets_load_sound(const char* file_name)
{
    if (!cf_path_ext_equ(file_name, ".ogg"))
    {
        return NULL;
    }
    
    Assets* assets = s_app->assets;
    file_name = cf_sintern(file_name);
    CF_Audio* sound = NULL;
    
    if (!cf_hashtable_has(assets->assets, file_name))
    {
        if (cf_fs_file_exists(file_name))
        {
            Asset new_asset = {
                .type = Asset_Type_Sound,
                .path = file_name,
                .sound = cf_audio_load_ogg(file_name)
            };
            
            if (new_asset.sound.id)
            {
                cf_hashtable_set(assets->assets, file_name, new_asset);
                printf("Loaded sound: %s\n", file_name);
            }
            else
            {
                printf("Failed to load sound - %s\n", file_name);
            }
        }
        else
        {
            printf("Failed to load sound - file does not exist - %s\n", file_name);
        }
    }
    
    // try to get again since loading could have failed
    if (cf_hashtable_has(assets->assets, file_name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, file_name);
        if (asset->type == Asset_Type_Sound)
        {
            sound = &asset->sound;
            asset->ref_count++;
        }
    }
    
    return sound;
}

CF_Sprite* assets_load_sprite(const char* file_name)
{
    if (!cf_path_ext_equ(file_name, ".ase"))
    {
        return NULL;
    }
    
    Assets* assets = s_app->assets;
    file_name = cf_sintern(file_name);
    CF_Sprite* sprite = NULL;
    
    if (!cf_hashtable_has(assets->assets, file_name))
    {
        if (cf_fs_file_exists(file_name))
        {
            CF_Result result = { 0 };
            Asset new_asset = {
                .type = Asset_Type_Sprite,
                .path = file_name,
                .sprite = cf_make_sprite(file_name)
            };
            
            if (result.code == CF_RESULT_SUCCESS && new_asset.sprite.name)
            {
                cf_hashtable_set(assets->assets, file_name, new_asset);
                printf("Loaded sprite: %s\n", file_name);
            }
            else
            {
                printf("Failed to load sprite - %s\n", file_name);
            }
        }
        else
        {
            printf("Failed to load sprite, files doesn't exist - %s\n", file_name);
        }
    }
    
    // try to get again since loading could have failed
    if (cf_hashtable_has(assets->assets, file_name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, file_name);
        if (asset->type == Asset_Type_Sprite)
        {
            sprite = &asset->sprite;
            asset->ref_count++;
        }
    }
    
    return sprite;
}

CF_Sprite* assets_load_png(const char* file_name)
{
    if (!cf_path_ext_equ(file_name, ".png"))
    {
        return NULL;
    }
    
    Assets* assets = s_app->assets;
    file_name = cf_sintern(file_name);
    CF_Sprite* png = NULL;
    
    if (!cf_hashtable_has(assets->assets, file_name))
    {
        if (cf_fs_file_exists(file_name))
        {
            CF_Result result = { 0 };
            Asset new_asset = {
                .type = Asset_Type_PNG,
                .path = file_name,
                .sprite = cf_make_easy_sprite_from_png(file_name, &result)
            };
            
            if (result.code == CF_RESULT_SUCCESS && new_asset.sprite.name)
            {
                cf_hashtable_set(assets->assets, file_name, new_asset);
                printf("Loaded png: %s\n", file_name);
            }
            else
            {
                printf("Failed to load png - %s\n", file_name);
            }
        }
        else
        {
            printf("Failed to load png, files doesn't exist - %s\n", file_name);
        }
    }
    
    // try to get again since loading could have failed
    if (cf_hashtable_has(assets->assets, file_name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, file_name);
        if (asset->type == Asset_Type_PNG)
        {
            png = &asset->sprite;
            asset->ref_count++;
        }
    }
    
    return png;
}

void assets_unload_sound(const char* name)
{
    Assets* assets = s_app->assets;
    name = cf_sintern(name);
    if (cf_hashtable_has(assets->assets, name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, name);
        if (asset->type == Asset_Type_Sound)
        {
            asset->ref_count--;
            if (asset->ref_count <= 0)
            {
                cf_audio_destroy(asset->sound);
                cf_hashtable_del(assets->assets, name);
                printf("Unloaded sound: %s\n", name);
            }
        }
    }
}

void assets_unload_sprite(const char* name)
{
    Assets* assets = s_app->assets;
    name = cf_sintern(name);
    if (cf_hashtable_has(assets->assets, name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, name);
        if (asset->type == Asset_Type_Sprite)
        {
            asset->ref_count--;
            if (asset->ref_count <= 0)
            {
                cf_sprite_unload(asset->path);
                cf_hashtable_del(assets->assets, name);
                printf("Unloaded sprite: %s\n", name);
            }
        }
    }
}

void assets_unload_png(const char* name)
{
    Assets* assets = s_app->assets;
    name = cf_sintern(name);
    if (cf_hashtable_has(assets->assets, name))
    {
        Asset* asset = cf_hashtable_get_ptr(assets->assets, name);
        if (asset->type == Asset_Type_PNG)
        {
            asset->ref_count--;
            if (asset->ref_count <= 0)
            {
                cf_easy_sprite_unload(&asset->sprite);
                cf_hashtable_del(assets->assets, name);
                printf("Unloaded png: %s\n", name);
            }
        }
    }
}

Asset_Resource* assets_get_resource_from_id(Asset_Object_ID id)
{
    Assets* assets = s_app->assets;
    u64 lookup_id = (u64)id;
    Asset_Resource* resource = NULL;
    
    if (cf_hashtable_has(assets->resource_ids, lookup_id))
    {
        resource = cf_hashtable_get_ptr(assets->resource_ids, lookup_id);
    }
    
    return resource;
}

Asset_Resource* assets_get_resource(const char* name)
{
    Assets* assets = s_app->assets;
    name = cf_sintern(name);
    Asset_Resource* resource = NULL;
    
    if (cf_hashtable_has(assets->resources, name))
    {
        resource = cf_hashtable_get_ptr(assets->resources, name);
    }
    
    return resource;
}

fixed Asset_Resource** assets_get_resources_of_type(Asset_Resource_Type type)
{
    Assets* assets = s_app->assets;
    
    s32 resources_count = cf_hashtable_count(assets->resources);
    Asset_Resource* resources = cf_hashtable_items(assets->resources);
    fixed Asset_Resource** same_type_resources = NULL;
    MAKE_SCRATCH_ARRAY(same_type_resources, 8);
    for (s32 index = 0; index < resources_count; ++index)
    {
        if (resources[index].type == type)
        {
            if (cf_array_count(same_type_resources) >= cf_array_capacity(same_type_resources))
            {
                GROW_SCRATCH_ARRAY(same_type_resources);
            }
            
            cf_array_push(same_type_resources, resources + index);
        }
    }
    
    return same_type_resources;
}

void* assets_get_resource_property_value(const char* name, const char* property_key)
{
    Asset_Resource* resource = assets_get_resource(name);
    return resource_get(resource, property_key);
}

void* resource_get(Asset_Resource* resource, const char* name)
{
    void* result = NULL;
    
    if (resource)
    {
        name = cf_sintern(name);
        for (s32 index = 0; index < cf_array_count(resource->properties); ++index)
        {
            if (resource->properties[index].key == name)
            {
                result = resource->properties[index].value;
            }
        }
    }
    
    return result;
}

void property_copy_to(Property* property, void* data)
{
    if (property->value && data && property->size)
    {
        u8* property_data_walker = property->value;
        u8* data_walker = data;
        s32 size = property->size;
        
        while (size-- > 0)
        {
            *data_walker++ |= *property_data_walker++;
        }
    }
}

#ifndef LEVEL_FILE_MUSIC_BIT
#define LEVEL_FILE_MUSIC_BIT (1 << 0)
#endif

#ifndef LEVEL_FILE_BACKGROUND_BIT
#define LEVEL_FILE_BACKGROUND_BIT (1 << 1)
#endif

// version 2
b32 save_level(Save_Level_Params params)
{
    mount_write_directory();
    
    const char* file_name = params.file_name;
    const char* name = params.name;
    const char* music_file_name = params.music_file_name;
    const char* background_file_name = params.background_file_name;
    V2i size = params.size;
    Tile* tiles = params.tiles;
    const char** layer_names = params.layer_names;
    Asset_Object_ID** layers = params.layers;
    s32 layer_count = params.layer_count;
    
    b32 success = false;
    if (!name || !file_name || CF_STRLEN(name) == 0 || CF_STRLEN(file_name) == 0)
    {
        return false;
    }
    
    char path[1024];
    
    if (!cf_path_ext_equ(file_name, LEVEL_FILE_TYPE_SUFFIX))
    {
        CF_SNPRINTF(path, sizeof(path), "%s%s", file_name, LEVEL_FILE_TYPE_SUFFIX);
    }
    else
    {
        CF_SNPRINTF(path, sizeof(path), file_name);
    }
    
    V2i min = v2i();
    V2i max = size;
    s32 tile_count = v2i_size(size);
    
    pq Asset_Object_ID* referenced_ids = NULL;
    pq_fit(referenced_ids, 128);
    
    // 0 is default empty
    pq_add(referenced_ids, 0, -1);
    
    for (s32 index = 0; index < layer_count; ++index)
    {
        Asset_Object_ID* layer = layers[index];
        for (s32 y = min.y; y < max.y; ++y)
        {
            for (s32 x = min.x; x < max.x; ++x)
            {
                s32 index = x + y * size.x;
                if (layer[index])
                {
                    pq_add_weight(referenced_ids, layer[index], 1);
                }
            }
        }
    }
    
    if (pq_count(referenced_ids))
    {
        Asset_Object_ID** temp_layers = scratch_alloc(sizeof(layers[0]) * layer_count);
        for (s32 index = 0; index < layer_count; ++index)
        {
            temp_layers[index] = scratch_alloc(sizeof(layers[0][0]) * tile_count);
            CF_MEMCPY(temp_layers[index], layers[index], sizeof(layers[0][0]) * tile_count);
        }
        
        Asset_Resource** local_resources = scratch_alloc(sizeof(Asset_Resource*) * (pq_count(referenced_ids) + 1));
        CF_MEMSET(local_resources, 0, sizeof(local_resources[0]) * (pq_count(referenced_ids) + 1)); 
        
        
        for (s32 index = 0; index < pq_count(referenced_ids); ++index)
        {
            local_resources[index] = assets_get_resource_from_id(referenced_ids[index]);
        }
        
        // update all layer objects to have local ids
        for (s32 index = 0; index < layer_count; ++index)
        {
            Asset_Object_ID* temp_layer = temp_layers[index];
            for (s32 y = min.y; y < max.y; ++y)
            {
                for (s32 x = min.x; x < max.x; ++x)
                {
                    s32 index = x + y * size.x;
                    s32 local_index = pq_index_of(referenced_ids, temp_layer[index]);
                    if (local_resources[local_index])
                    {
                        temp_layer[index] = local_index;
                    }
                }
            }
        }
        
        // write out to a temp file to make sure it successfully wrote everything out
        // to avoid stomping on a pre-existing level data
        const char* temp_path = ".temp.ipl";
        CF_File* file = NULL;
        if (cf_fs_file_exists(temp_path))
        {
            file = cf_fs_open_file_for_write(temp_path);
        }
        else
        {
            file = cf_fs_create_file(temp_path);
        }
        
        CF_ASSERT(file);
        // write out level contents
        {
            s32 referenced_ids_count = pq_count(referenced_ids);
            s32 empty = 0;
            
            // format
            // version
            // NAME_LENGTH NAME
            // LEVEL_SIZE
            // LOCAL_IDS_COUNT
            //   LOCAL_ID_LENGTH LOCAL_ID
            // LAYER_COUNT
            // LAYER_NAMES (1..N)
            // TILES
            // LAYER (1..N)
            // OPT_STRING_FIELD(S)
            u64 version = 2;
            cf_fs_write(file, &version, sizeof(version));
            
            ASSETS_FILE_WRITE_STRING(file, name);
            cf_fs_write(file, &size, sizeof(size));
            cf_fs_write(file, &referenced_ids_count, sizeof(s32));
            // first entry is always empty
            cf_fs_write(file, &empty, sizeof(empty));
            //cf_fs_write(file, &empty, 0);
            
            // write out all local ids
            for (s32 index = 1; index < referenced_ids_count; ++index)
            {
                Asset_Resource* resource = local_resources[index];
                ASSETS_FILE_WRITE_STRING(file, resource->name);
            }
            
            cf_fs_write(file, &layer_count, sizeof(layer_count));
            for (s32 index = 0; index < layer_count; ++index)
            {
                const char* layer_name = layer_names[index];
                s32 length = (s32)CF_STRLEN(layer_name);
                cf_fs_write(file, &length, sizeof(length));
                cf_fs_write(file, layer_name, sizeof(layer_name[0]) * length);
            }
            
            s32 rle_line_count = 0;
            fixed RLE* rle_lines = grid_to_rle(size, (s32*)tiles, NULL);
            
            rle_line_count = cf_array_count(rle_lines);
            cf_fs_write(file, &rle_line_count, sizeof(rle_line_count));
            cf_fs_write(file, rle_lines, sizeof(rle_lines[0]) * cf_array_count(rle_lines));
            
            for (s32 index = 0; index < layer_count; ++index)
            {
                Asset_Object_ID* temp_layer = temp_layers[index];
                rle_lines = grid_to_rle(size, (s32*)temp_layer, rle_lines);
                
                rle_line_count = cf_array_count(rle_lines);
                cf_fs_write(file, &rle_line_count, sizeof(rle_line_count));
                cf_fs_write(file, rle_lines, sizeof(rle_lines[0]) * cf_array_count(rle_lines));
            }
            
            if (music_file_name && CF_STRLEN(music_file_name))
            {
                s32 type = LEVEL_FILE_MUSIC_BIT;
                cf_fs_write(file, &type, sizeof(type));
                ASSETS_FILE_WRITE_STRING(file, music_file_name);
            }
            if (background_file_name && CF_STRLEN(background_file_name))
            {
                s32 type = LEVEL_FILE_BACKGROUND_BIT;
                cf_fs_write(file, &type, sizeof(type));
                ASSETS_FILE_WRITE_STRING(file, background_file_name);
            }
        }
        cf_fs_close(file);
        
        // move over from temp file to actual file
        if (cf_fs_file_exists(path))
        {
            cf_fs_remove(path);
        }
        u64 temp_file_size = 0;
        void* temp_file = cf_fs_read_entire_file_to_memory(temp_path, &temp_file_size);
        
        cf_fs_write_entire_buffer_to_file(path, temp_file, temp_file_size);
        
        cf_free(temp_file);
        cf_fs_remove(temp_path);
        
        success = true;
    }
    
    pq_free(referenced_ids);
    
    return success;
}

Load_Level_Result load_level(const char* file_name)
{
    Load_Level_Result result = (Load_Level_Result){
        .file_name = file_name,
        .success = false,
    };
    
    if (!file_name || CF_STRLEN(file_name) == 0)
    {
        return result;
    }
    
    if (!cf_fs_file_exists(file_name))
    {
        return result;
    }
    
    u64 file_size = 0;
    void* file = cf_fs_read_entire_file_to_memory(file_name, &file_size);
    u8* data = file;
    
    ASSETS_BUF_READ(data, &result.version, sizeof(result.version));
    
    switch (result.version)
    {
        case 1:
        {
            load_level_version_1(file, data, file_size, &result);
        }
        break;
        case 2:
        {
            load_level_version_2(file, data, file_size, &result);
        }
        break;
        default:
        {
            printf("Failed to load level, version %" PRIu64 " is not supported\n", result.version);
        }
        break;
    }
    
    cf_free(file);
    
    return result;
}

void load_level_version_1(u8* file, u8* data, u64 file_size, Load_Level_Result* result)
{
    char* buf = scratch_alloc(1024);
    
    // level name
    ASSETS_BUF_READ_STRING(data, buf);
    result->name = string_clone(buf);
    
    // level size
    ASSETS_BUF_READ(data, &result->size, sizeof(result->size));
    if (result->size.x < LEVEL_SIZE_MIN.x || result->size.y < LEVEL_SIZE_MIN.y ||
        result->size.x > LEVEL_SIZE_MAX.x || result->size.y > LEVEL_SIZE_MAX.y)
    {
        printf("Failed to load level %s, invalid level size\n", result->file_name);
        return;
    }
    
    s32 referenced_ids_count = 0;
    ASSETS_BUF_READ(data, &referenced_ids_count, sizeof(referenced_ids_count));
    
    if  (referenced_ids_count < 0)
    {
        printf("Failed to load level %s, invalid reference ids count %d\n", result->file_name, referenced_ids_count);
        return;
    }
    
    // local reference names
    fixed const char** reference_names = NULL;
    MAKE_SCRATCH_ARRAY(reference_names, referenced_ids_count);
    for (s32 index = 0; index < referenced_ids_count; ++index)
    {
        s32 length = 0;
        ASSETS_BUF_READ(data, &length, sizeof(length));
        char *name = NULL;
        if (length > 0)
        {
            name = scratch_alloc(length + 1);
            ASSETS_BUF_READ(data, name, sizeof(char) * length);
            name[length] = '\0';
        }
        cf_array_push(reference_names, name);
    }
    
    // layer count
    s32 layer_count = 0;
    ASSETS_BUF_READ(data, &layer_count, sizeof(layer_count));
    if  (layer_count < 0)
    {
        printf("Failed to load level %s, invalid layer count %d\n", result->file_name, layer_count);
        return;
    }
    
    // layer names
    MAKE_SCRATCH_ARRAY(result->layer_names, layer_count);
    for (s32 index = 0; index < layer_count; ++index)
    {
        s32 length = 0;
        ASSETS_BUF_READ(data, &length, sizeof(length));
        
        char* layer_name = scratch_alloc(sizeof(char) * length + 1);
        ASSETS_BUF_READ(data, layer_name, sizeof(layer_name[0]) * length);
        layer_name[length] = '\0';
        
        cf_array_push(result->layer_names, layer_name);
    }
    
    // tiles
    result->tile_count = v2i_size(result->size);
    result->tiles = scratch_alloc(sizeof(Tile) * result->tile_count);
    ASSETS_BUF_READ(data, result->tiles, sizeof(Tile) * result->tile_count);
    MAKE_SCRATCH_ARRAY(result->layers, layer_count);
    
    for (s32 index = 0; index < layer_count; ++index)
    {
        Asset_Object_ID* layer = scratch_alloc(sizeof(Asset_Object_ID) * result->tile_count);
        cf_array_push(result->layers, layer);
    }
    
    // layers
    for (s32 index = 0; index < layer_count; ++index)
    {
        ASSETS_BUF_READ(data, result->layers[index], sizeof(Asset_Object_ID) * result->tile_count);
    }
    
    // additional data
    while ((u64)(data - (u8*)file) < file_size)
    {
        s32 type = 0;
        ASSETS_BUF_READ(data, &type, sizeof(type));
        
        b32 processed_additional_data = false;
        
        switch (type)
        {
            case LEVEL_FILE_MUSIC_BIT:
            {
                ASSETS_BUF_READ_STRING(data, buf);
                processed_additional_data = CF_STRLEN(buf) > 0;
                if (processed_additional_data)
                {
                    result->music_file_name = string_clone(buf);
                }
            }
            break;
            case LEVEL_FILE_BACKGROUND_BIT:
            {
                ASSETS_BUF_READ_STRING(data, buf);
                processed_additional_data = CF_STRLEN(buf) > 0;
                if (processed_additional_data)
                {
                    result->background_file_name = string_clone(buf);
                }
            }
            break;
            default:
            break;
        }
        
        if (!processed_additional_data)
        {
            break;
        }
    }
    
    // fix up ids from local file back to runtime global
    fixed Asset_Object_ID* resource_ids = NULL;
    MAKE_SCRATCH_ARRAY(resource_ids, referenced_ids_count);
    for (s32 index = 0; index < referenced_ids_count; ++index)
    {
        Asset_Resource* resource = assets_get_resource(reference_names[index]);
        if (resource)
        {
            cf_array_push(resource_ids, resource->id);
        }
        else
        {
            if (reference_names[index])
            {
                printf("Failed to find reference for %s\n", reference_names[index]);
            }
            cf_array_push(resource_ids, 0);
        }
    }
    
    fixed V2i* entity_tiles = NULL;
    fixed Asset_Object_ID* entity_resource_ids;
    MAKE_SCRATCH_ARRAY(entity_tiles, 128);
    MAKE_SCRATCH_ARRAY(entity_resource_ids, 128);
    
    for (s32 layer_index = 0; layer_index < layer_count; ++layer_index)
    {
        Asset_Object_ID* layer = result->layers[layer_index];
        for (s32 index = 0; index < result->tile_count; ++index)
        {
            layer[index] = resource_ids[layer[index]];
            if (layer_index > EDITOR_TILE_LAYER && layer[index])
            {
                if (cf_array_count(entity_tiles) >= cf_array_capacity(entity_tiles))
                {
                    GROW_SCRATCH_ARRAY(entity_tiles);
                    GROW_SCRATCH_ARRAY(entity_resource_ids);
                }
                
                V2i tile = v2i(.x = index % result->size.x);
                tile.y = (index - tile.x) / result->size.x;
                cf_array_push(entity_tiles, tile);
                cf_array_push(entity_resource_ids, layer[index]);
            }
        }
    }
    
    result->entity_tiles = entity_tiles;
    result->entity_resource_ids = entity_resource_ids;
    result->layer_count = layer_count;
    result->object_names = reference_names;
    result->success = true;
}

void load_level_version_2(u8* file, u8* data, u64 file_size, Load_Level_Result* result)
{
    char* buf = scratch_alloc(1024);
    
    // level name
    ASSETS_BUF_READ_STRING(data, buf);
    result->name = string_clone(buf);
    
    // level size
    ASSETS_BUF_READ(data, &result->size, sizeof(result->size));
    if (result->size.x < LEVEL_SIZE_MIN.x || result->size.y < LEVEL_SIZE_MIN.y ||
        result->size.x > LEVEL_SIZE_MAX.x || result->size.y > LEVEL_SIZE_MAX.y)
    {
        printf("Failed to load level %s, invalid level size\n", result->file_name);
        return;
    }
    
    s32 referenced_ids_count = 0;
    ASSETS_BUF_READ(data, &referenced_ids_count, sizeof(referenced_ids_count));
    
    if  (referenced_ids_count < 0)
    {
        printf("Failed to load level %s, invalid reference ids count %d\n", result->file_name, referenced_ids_count);
        return;
    }
    
    // local reference names
    fixed const char** reference_names = NULL;
    MAKE_SCRATCH_ARRAY(reference_names, referenced_ids_count);
    for (s32 index = 0; index < referenced_ids_count; ++index)
    {
        s32 length = 0;
        ASSETS_BUF_READ(data, &length, sizeof(length));
        char *name = NULL;
        if (length > 0)
        {
            name = scratch_alloc(length + 1);
            ASSETS_BUF_READ(data, name, sizeof(char) * length);
            name[length] = '\0';
        }
        cf_array_push(reference_names, name);
    }
    
    // layer count
    s32 layer_count = 0;
    ASSETS_BUF_READ(data, &layer_count, sizeof(layer_count));
    if  (layer_count < 0)
    {
        printf("Failed to load level %s, invalid layer count %d\n", result->file_name, layer_count);
        return;
    }
    
    // layer names
    MAKE_SCRATCH_ARRAY(result->layer_names, layer_count);
    for (s32 index = 0; index < layer_count; ++index)
    {
        s32 length = 0;
        ASSETS_BUF_READ(data, &length, sizeof(length));
        
        char* layer_name = scratch_alloc(sizeof(char) * length + 1);
        ASSETS_BUF_READ(data, layer_name, sizeof(layer_name[0]) * length);
        layer_name[length] = '\0';
        
        cf_array_push(result->layer_names, layer_name);
    }
    
    // tiles
    result->tile_count = v2i_size(result->size);
    result->tiles = scratch_alloc(sizeof(Tile) * result->tile_count);
    
    s32 rle_line_count = 0;
    ASSETS_BUF_READ(data, &rle_line_count, sizeof(rle_line_count));
    
    if (rle_line_count < 0)
    {
        printf("Failed to load level %s, invalid rle line count %d", result->file_name, rle_line_count);
        return;
    }
    
    fixed RLE* rle_lines = NULL;
    MAKE_SCRATCH_ARRAY(rle_lines, next_power_of_2(rle_line_count));
    ASSETS_BUF_READ(data, rle_lines, sizeof(rle_lines[0]) * rle_line_count);
    cf_array_len(rle_lines) = rle_line_count;
    
    // needs to be zero'd out since grid_to_rle() ignores 0s
    CF_MEMSET(result->tiles, 0, sizeof(result->tiles[0]) * result->tile_count);
    rle_to_grid(result->size, (s32*)result->tiles, rle_lines);
    
    MAKE_SCRATCH_ARRAY(result->layers, layer_count);
    
    for (s32 index = 0; index < layer_count; ++index)
    {
        Asset_Object_ID* layer = scratch_alloc(sizeof(Asset_Object_ID) * result->tile_count);
        cf_array_push(result->layers, layer);
    }
    
    // layers
    for (s32 index = 0; index < layer_count; ++index)
    {
        ASSETS_BUF_READ(data, &rle_line_count, sizeof(rle_line_count));
        if (rle_line_count < 0)
        {
            printf("Failed to load level %s, invalid rle line count %d", result->file_name, rle_line_count);
            return;
        }
        
        while (cf_array_capacity(rle_lines) < rle_line_count)
        {
            GROW_SCRATCH_ARRAY(rle_lines);
        }
        
        // needs to be zero'd out since grid_to_rle() ignores 0s
        ASSETS_BUF_READ(data, rle_lines, sizeof(rle_lines[0]) * rle_line_count);
        cf_array_len(rle_lines) = rle_line_count;
        
        CF_MEMSET(result->layers[index], 0, sizeof(result->layers[index][0]) * result->tile_count);
        rle_to_grid(result->size, (s32*)result->layers[index], rle_lines);
    }
    
    // additional data
    while ((u64)(data - (u8*)file) < file_size)
    {
        s32 type = 0;
        ASSETS_BUF_READ(data, &type, sizeof(type));
        
        b32 processed_additional_data = false;
        
        switch (type)
        {
            case LEVEL_FILE_MUSIC_BIT:
            {
                ASSETS_BUF_READ_STRING(data, buf);
                processed_additional_data = CF_STRLEN(buf) > 0;
                if (processed_additional_data)
                {
                    result->music_file_name = string_clone(buf);
                }
            }
            break;
            case LEVEL_FILE_BACKGROUND_BIT:
            {
                ASSETS_BUF_READ_STRING(data, buf);
                processed_additional_data = CF_STRLEN(buf) > 0;
                if (processed_additional_data)
                {
                    result->background_file_name = string_clone(buf);
                }
            }
            break;
            default:
            break;
        }
        
        if (!processed_additional_data)
        {
            break;
        }
    }
    
    // fix up ids from local file back to runtime global
    fixed Asset_Object_ID* resource_ids = NULL;
    MAKE_SCRATCH_ARRAY(resource_ids, referenced_ids_count);
    for (s32 index = 0; index < referenced_ids_count; ++index)
    {
        Asset_Resource* resource = assets_get_resource(reference_names[index]);
        if (resource)
        {
            cf_array_push(resource_ids, resource->id);
        }
        else
        {
            if (reference_names[index])
            {
                printf("Failed to find reference for %s\n", reference_names[index]);
            }
            cf_array_push(resource_ids, 0);
        }
    }
    
    fixed V2i* entity_tiles = NULL;
    fixed Asset_Object_ID* entity_resource_ids;
    MAKE_SCRATCH_ARRAY(entity_tiles, 128);
    MAKE_SCRATCH_ARRAY(entity_resource_ids, 128);
    
    for (s32 layer_index = 0; layer_index < layer_count; ++layer_index)
    {
        Asset_Object_ID* layer = result->layers[layer_index];
        for (s32 index = 0; index < result->tile_count; ++index)
        {
            layer[index] = resource_ids[layer[index]];
            if (layer_index > EDITOR_TILE_LAYER && layer[index])
            {
                if (cf_array_count(entity_tiles) >= cf_array_capacity(entity_tiles))
                {
                    GROW_SCRATCH_ARRAY(entity_tiles);
                    GROW_SCRATCH_ARRAY(entity_resource_ids);
                }
                
                V2i tile = v2i(.x = index % result->size.x);
                tile.y = (index - tile.x) / result->size.x;
                cf_array_push(entity_tiles, tile);
                cf_array_push(entity_resource_ids, layer[index]);
            }
        }
    }
    
    result->entity_tiles = entity_tiles;
    result->entity_resource_ids = entity_resource_ids;
    result->layer_count = layer_count;
    result->object_names = reference_names;
    result->success = true;
}
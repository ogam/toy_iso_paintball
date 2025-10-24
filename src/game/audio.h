#ifndef AUDIO_H
#define AUDIO_H

typedef s32 Audio_Source_Type;
enum
{
    Audio_Source_Type_SFX,
    Audio_Source_Type_Music,
    Audio_Source_Type_UI,
    Audio_Source_Type_Count,
};

typedef s32 Audio_Source_Type_Transition;
enum
{
    Audio_Source_Type_Transition_None,
    Audio_Source_Type_Transition_Play,
    Audio_Source_Type_Transition_Pause,
    Audio_Source_Type_Transition_Stop,
};

typedef struct Audio_Source
{
    Audio_Source_Type type;
    CF_Sound sound;
} Audio_Source;

typedef struct Audio_Source_Params
{
    Audio_Source_Type type;
    const char* name;
} Audio_Source_Params;

typedef struct Audio_Volumes
{
    f32 master;
    f32 sfx;
    f32 music;
    f32 ui;
} Audio_Volumes;

typedef struct Audio_System
{
    dyna Audio_Source* active_list;
    
    // using a pq as a set to avoid playing multiple same sounds,
    // otherwise if a bunch of things play the same sound at the
    // same time then you could end up blowing someone's ears out
    pq Audio_Source_Params* queue;
    
    Audio_Source_Type_Transition global_transitions[Audio_Source_Type_Count];
    
    Audio_Volumes volumes;
    Audio_Volumes temp_volumes;
    
    b32 use_temp_settings;
} Audio_System;

void audio_init();
void audio_update();

void audio_play(const char *name, Audio_Source_Type type);
void audio_play_random(dyna const char** names, Audio_Source_Type type);
void audio_pause(Audio_Source_Type type);
void audio_unpause(Audio_Source_Type type);
void audio_stop(Audio_Source_Type type);
void audio_stop_all(Audio_Source_Type type);

Audio_Volumes* audio_make_temp_volumes();
void audio_apply_temp_volumes();
void audio_settings_save(CF_JDoc doc, CF_JVal audio_obj);
void audio_settings_load(CF_JVal audio_obj);
b32 audio_seetings_has_changed();

#endif //AUDIO_H

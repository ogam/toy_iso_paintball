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

typedef struct Audio_System
{
    dyna Audio_Source* active_list;
    
    // using a pq as a set to avoid playing multiple same sounds,
    // otherwise if a bunch of things play the same sound at the
    // same time then you could end up blowing someone's ears out
    pq Audio_Source_Params* queue;
    
    Audio_Source_Type_Transition global_transitions[Audio_Source_Type_Count];
    
    f32 volume_master;
    f32 volume_sfx;
    f32 volume_music;
    f32 volume_ui;
} Audio_System;

void audio_init();
void audio_update();

void audio_play(const char *name, Audio_Source_Type type);
void audio_play_random(dyna const char** names, Audio_Source_Type type);
void audio_pause(Audio_Source_Type type);
void audio_unpause(Audio_Source_Type type);
void audio_stop(Audio_Source_Type type);
void audio_stop_all(Audio_Source_Type type);

#endif //AUDIO_H

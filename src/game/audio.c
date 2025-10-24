#include "game/audio.h"

void audio_init()
{
    Audio_System* audio_system = cf_calloc(sizeof(Audio_System), 1);
    s_app->audio_system = audio_system;
    
    audio_system->volumes.master = 0.5f;
    audio_system->volumes.sfx = 1.0f;
    audio_system->volumes.music = 1.0f;
    audio_system->volumes.ui = 1.0f;
    
    cf_array_fit(audio_system->active_list, 128);
    pq_fit(audio_system->queue, 128);
}

void audio_update()
{
    perf_begin("audio_update");
    Audio_System* audio_system = s_app->audio_system;
    
    f32 volume_sfx = audio_system->volumes.master * audio_system->volumes.sfx;
    f32 volume_music = audio_system->volumes.master * audio_system->volumes.music;
    f32 volume_ui = audio_system->volumes.master * audio_system->volumes.ui;
    
    if (audio_system->use_temp_settings)
    {
        volume_sfx = audio_system->temp_volumes.master * audio_system->temp_volumes.sfx;
        volume_music = audio_system->temp_volumes.master * audio_system->temp_volumes.music;
        volume_ui = audio_system->temp_volumes.master * audio_system->temp_volumes.ui;
    }
    
    // remove anything that's done playing
    {
        for (s32 index = 0; index < cf_array_count(audio_system->active_list);)
        {
            Audio_Source* source = audio_system->active_list + index;
            if (!cf_sound_is_active(source->sound))
            {
                cf_array_del(audio_system->active_list, index);
                continue;
            }
            ++index;
        }
    }
    
    // try to play all sources from queue
    {
        for (s32 index = 0; index < pq_count(audio_system->queue); ++index)
        {
            Audio_Source_Params params = audio_system->queue[index];
            CF_Audio* audio = assets_get_sound(params.name);
            
            if (audio)
            {
                CF_SoundParams sound_params = cf_sound_params_defaults();
                //  @todo:  either have music use cf_music_play() or write a crossfade
                if (params.type == Audio_Source_Type_Music)
                {
                    sound_params.looped = true;
                }
                
                Audio_Source source = 
                {
                    .type = params.type,
                    .sound = cf_play_sound(*audio, sound_params),
                };
                cf_array_push(audio_system->active_list, source);
            }
        }
    }
    
    // update all volumes
    {
        for (s32 index = 0; index < cf_array_count(audio_system->active_list); ++index)
        {
            Audio_Source* source = audio_system->active_list + index;
            f32 volume = volume_ui;
            switch (source->type)
            {
                case Audio_Source_Type_SFX:   volume = volume_sfx;   break;
                case Audio_Source_Type_Music: volume = volume_music; break;
                default: break;
            }
            cf_sound_set_volume(source->sound, volume);
        }
    }
    
    // update all play/pause
    {
        for (s32 type_index = 0; type_index < CF_ARRAY_SIZE(audio_system->global_transitions); ++type_index)
        {
            Audio_Source_Type type = type_index;
            Audio_Source_Type_Transition transition = audio_system->global_transitions[type_index];
            
            if (transition == Audio_Source_Type_Transition_None)
            {
                continue;
            }
            
            if (transition == Audio_Source_Type_Transition_Pause)
            {
                for (s32 index = 0; index < cf_array_count(audio_system->active_list); ++index)
                {
                    Audio_Source* source = audio_system->active_list + index;
                    if (source->type == type)
                    {
                        cf_sound_set_is_paused(source->sound, true);
                    }
                }
            }
            else if (transition == Audio_Source_Type_Transition_Play)
            {
                for (s32 index = 0; index < cf_array_count(audio_system->active_list); ++index)
                {
                    Audio_Source* source = audio_system->active_list + index;
                    if (source->type == type)
                    {
                        cf_sound_set_is_paused(source->sound, false);
                    }
                }
            }
        }
    }
    
    pq_clear(audio_system->queue);
    CF_MEMSET(audio_system->global_transitions, 0, sizeof(audio_system->global_transitions));
    perf_end("audio_update");
}

void audio_play(const char *name, Audio_Source_Type type)
{
    CF_ASSERT(type >= 0 && type < Audio_Source_Type_Count);
    Audio_Source_Params params = 
    {
        .type = type,
        .name = cf_sintern(name),
    };
    pq_add_weight(s_app->audio_system->queue, params, 1);
}

void audio_play_random(dyna const char** names, Audio_Source_Type type)
{
    if (cf_array_count(names))
    {
        s32 index = cf_rnd_range_int(&s_app->assets->rnd, 0, cf_array_count(names) - 1);
        
        CF_ASSERT(type >= 0 && type < Audio_Source_Type_Count);
        Audio_Source_Params params = 
        {
            .type = type,
            .name = names[index],
        };
        pq_add_weight(s_app->audio_system->queue, params, 1);
    }
}

void audio_pause(Audio_Source_Type type)
{
    CF_ASSERT(type >= 0 && type < Audio_Source_Type_Count);
    s_app->audio_system->global_transitions[type] = Audio_Source_Type_Transition_Pause;
}

void audio_unpause(Audio_Source_Type type)
{
    CF_ASSERT(type >= 0 && type < Audio_Source_Type_Count);
    s_app->audio_system->global_transitions[type] = Audio_Source_Type_Transition_Play;
}

void audio_stop(Audio_Source_Type type)
{
    CF_ASSERT(type >= 0 && type < Audio_Source_Type_Count);
    Audio_System* audio_system = s_app->audio_system;
    
    for (s32 index = 0; index < cf_array_count(audio_system->active_list); ++index)
    {
        Audio_Source* source = audio_system->active_list + index;
        if (source->type == type)
        {
            cf_sound_stop(source->sound);
        }
    }
}

void audio_stop_all(Audio_Source_Type type)
{
    Audio_System* audio_system = s_app->audio_system;
    for (s32 index = 0; index < cf_array_count(audio_system->active_list); ++index)
    {
        Audio_Source* source = audio_system->active_list + index;
        cf_sound_stop(source->sound);
    }
}

Audio_Volumes* audio_make_temp_volumes()
{
    Audio_System* audio_system = s_app->audio_system;
    Audio_Volumes* volumes = &audio_system->volumes;
    Audio_Volumes* temp_volumes = &audio_system->temp_volumes;
    
    CF_MEMCPY(temp_volumes, volumes, sizeof(*volumes));
    return volumes;
}

void audio_apply_temp_volumes()
{
    Audio_System* audio_system = s_app->audio_system;
    Audio_Volumes* volumes = &audio_system->volumes;
    Audio_Volumes* temp_volumes = &audio_system->temp_volumes;
    
    CF_MEMCPY(volumes, temp_volumes, sizeof(*volumes));
}

void audio_settings_save(CF_JDoc doc, CF_JVal audio_obj)
{
    Audio_System* audio_system = s_app->audio_system;
    Audio_Volumes* volumes = &audio_system->volumes;
    
    cf_json_object_add_float(doc, audio_obj, "master", volumes->master);
    cf_json_object_add_float(doc, audio_obj, "sfx", volumes->sfx);
    cf_json_object_add_float(doc, audio_obj, "music", volumes->music);
    cf_json_object_add_float(doc, audio_obj, "ui", volumes->ui);
}

void audio_settings_load(CF_JVal audio_obj)
{
    Audio_System* audio_system = s_app->audio_system;
    Audio_Volumes* volumes = &audio_system->volumes;
    
    if (cf_json_is_object(audio_obj))
    {
        CF_JVal master = cf_json_get(audio_obj, "master");
        CF_JVal sfx = cf_json_get(audio_obj, "sfx");
        CF_JVal music = cf_json_get(audio_obj, "music");
        CF_JVal ui = cf_json_get(audio_obj, "ui");
        
        if (cf_json_is_float(master))
        {
            volumes->master = cf_json_get_float(master);
        }
        if (cf_json_is_float(sfx))
        {
            volumes->sfx = cf_json_get_float(sfx);
        }
        if (cf_json_is_float(music))
        {
            volumes->music = cf_json_get_float(music);
        }
        if (cf_json_is_float(ui))
        {
            volumes->ui = cf_json_get_float(ui);
        }
    }
}

b32 audio_seetings_has_changed()
{
    Audio_System* audio_system = s_app->audio_system;
    Audio_Volumes* volumes = &audio_system->volumes;
    Audio_Volumes* temp_volumes = &audio_system->temp_volumes;
    
    b32 changed = false;
    
    changed |= !f32_is_zero(volumes->master - temp_volumes->master);
    changed |= !f32_is_zero(volumes->music - temp_volumes->music);
    changed |= !f32_is_zero(volumes->sfx - temp_volumes->sfx);
    changed |= !f32_is_zero(volumes->ui - temp_volumes->ui);
    
    return changed;
}
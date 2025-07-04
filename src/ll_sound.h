// this is a linked-list for sfx/sounds
// it lets you just laod the sound, and if it's already been loaded, it will return that.

#include "pntr_app_sfx.h"

// push to front of LL
#ifndef LL_PUSH
#define LL_PUSH(head, node) do { (node)->next = (head); (head) = (node); } while(0)
#endif

typedef struct sound_holder_t {
    struct sound_holder_t* next;
    char* filename;
    pntr_sound* sound;
    SfxParams* params;
} sound_holder_t;

// add/get a sound-effect
sound_holder_t* sfx_load(sound_holder_t** sounds, pntr_app* app, char* filename) {
    // search for same name
    sound_holder_t* found = (*sounds);
    while(found != NULL) {
        if (PNTR_STRCMP(found->filename, filename) == 0) {
            pntr_app_log_ex(PNTR_APP_LOG_DEBUG, "Sound: found '%s' (preloaded.)", filename);
            break;
        }
        found = found->next;
    }

    if (found != NULL) {
        return found;
    }
    
    pntr_app_log_ex(PNTR_APP_LOG_DEBUG, "Sound: loading '%s' (not preloaded.)", filename);
    
    sound_holder_t* current = pntr_load_memory(sizeof(sound_holder_t));
    current->filename = strdup(filename);
    current->params = pntr_load_memory(sizeof(SfxParams));
    pntr_app_sfx_load_params(current->params, filename);
    current->sound = pntr_app_sfx_sound(app, current->params);

    LL_PUSH(*sounds, current);
    return current;
}

// add/get a sound
sound_holder_t* sound_load(sound_holder_t** sounds, pntr_app* app, char* filename) {
    // search for same name
    sound_holder_t* found = (*sounds);
    while(found != NULL) {
        if (PNTR_STRCMP(found->filename, filename) == 0) {
            // pntr_app_log_ex(PNTR_APP_LOG_DEBUG, "Sound: found '%s' (preloaded.)", filename);
            break;
        }
        found = found->next;
    }

    if (found != NULL) {
        return found;
    }
    
    // pntr_app_log_ex(PNTR_APP_LOG_DEBUG, "Sound: loading '%s' (not preloaded.)", filename);
    
    sound_holder_t* current = pntr_load_memory(sizeof(sound_holder_t));
    current->filename = strdup(filename);
    current->sound = pntr_load_sound(filename);

    LL_PUSH(*sounds, current);
    return current;
}

// free the current head of the list
void sounds_unload(sound_holder_t** sounds) {
    if (sounds && *sounds) {
        sound_holder_t* to_free = *sounds;
        *sounds = (*sounds)->next;
        free(to_free);
    }
}
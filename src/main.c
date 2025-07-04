#define PNTR_APP_IMPLEMENTATION
#define PNTR_APP_SFX_IMPLEMENTATION
#define PNTR_TILED_IMPLEMENTATION

#define PNTR_ENABLE_VARGS
#define PNTR_ENABLE_DEFAULT_FONT

#include "pntr_app.h"
#include "pntr_tiled.h"

#include "adventure.h"

// I'm really into linked-lists right now
#include "ll_sound.h"
#include "ll_animation_queue.h"

// head of linked list
static adventure_map_t* maps = NULL;
static sound_holder_t* sounds = NULL;
static animation_queue_t* animations = NULL;

// default font for dialogs
static pntr_font* font;

// current-loaded game map
static adventure_map_t* currentMap = NULL;

// eventually, I could get these from the map somehow
static float player_speed = 200;

// on a 16x16 map, it's at the "body" of a charater tile
static pntr_rectangle player_hitbox = {4, 8, 8, 8};

// stores direction across frames, so if you face a way, it will keep facing that way
static int gid_direction = 0; 

// if there is a current-dialog, it triggers game to pause
static char dialogText[1024] = {0};

// set this to false, for 1-time render
static bool shownDialog = true;

// set this to tru to show the title-screen
static bool showTitle = true;

// your roopies
static int gemCount = 0;

// we can derive the correct tile (specific to my spritesheet layout)
// since each row (12 tiles) is a character, broken into 3 frames per direction
// with the 2nd frame as the indicator for "walking animation"
// this will work with my spritesheet, but you can adjust for yours, if it's differnt
static void set_gid(cute_tiled_object_t* character, int gid_direction, int gid_walking) {
    int gid_character = character->gid/12;
    character->gid = (gid_character*12) + 1 + gid_walking + (gid_direction*3);
}

// move in opposite direction currently facing
static void bump_back(cute_tiled_object_t* character, float player_speed) {
    int gid_character = character->gid / 12;
    int gid_direction = character->gid % 4;
    int gid_direction_opposite = gid_direction ^ 1;

    printf("bump back: %d %d\n", gid_character, gid_direction, gid_direction_opposite);
    character->gid = (gid_character * 12) + 1 + (gid_direction_opposite * 3);

    // Direction deltas: S, N, E, W
    const int dx[4] = { 0,  0,  1, -1 };
    const int dy[4] = { 1, -1,  0,  0 };

    // Move in the opposite direction
    character->x += dx[gid_direction_opposite] * player_speed;
    character->y += dy[gid_direction_opposite] * player_speed;
}

// this is called when the player or an NPC touches something
// object will be NULL, if it's static geometry (from collision layer)
void CollisionCallback(pntr_app* app, adventure_map_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object) {
    if (gemCount < 0) {
        // nothing happens when you're dead
        return;
    }
    if (object == NULL) {
        pntr_app_log_ex(PNTR_APP_LOG_DEBUG,"Map: %s bumped static\n", subject->name.ptr);
    } else {
        // get all properties from object
        char sound[PNTR_PATH_MAX] = {0};
        int value = 1;

        for (int i = 0; i < object->property_count; i++) {
            cute_tiled_property_t* prop = &object->properties[i];
            if (prop->type == CUTE_TILED_PROPERTY_STRING) {
                if (PNTR_STRCMP("text", prop->name.ptr) == 0) {
                    dialogText[0] = 0;
                    PNTR_STRCAT(dialogText, prop->data.string.ptr);
                    shownDialog = false;
                }
                else if (PNTR_STRCMP("sound", prop->name.ptr) == 0) {
                    PNTR_STRCAT(sound, "assets/rfx/");
                    PNTR_STRCAT(sound, prop->data.string.ptr);
                    PNTR_STRCAT(sound, ".rfx");
                }
                else if (PNTR_STRCMP("facing", prop->name.ptr) == 0) {
                    // TODO: use this to set directiopn of player on portal
                }
            }
            else if (prop->type == CUTE_TILED_PROPERTY_INT) {
                if (PNTR_STRCMP("pos_x", prop->name.ptr) == 0) {
                    // TODO: use this to set position of player on portal
                }
                else if (PNTR_STRCMP("pos_y", prop->name.ptr) == 0) {
                    // TODO: use this to set position of player on portal
                }
                else if (PNTR_STRCMP("value", prop->name.ptr) == 0) {
                    value = prop->data.integer;
                }
            }
        }

        if (PNTR_STRCMP(subject->name.ptr, "player") == 0) {
            if (PNTR_STRCMP(object->type.ptr, "portal") == 0) {
                // portal name is the map it links to
                char filename[PNTR_PATH_MAX] = {0};
                PNTR_STRCAT(filename, "assets/");
                PNTR_STRCAT(filename, object->name.ptr);
                PNTR_STRCAT(filename, ".tmj");
                currentMap = adventure_load(filename, &maps);
            }
            else if (PNTR_STRCMP(object->type.ptr, "loot") == 0) {
                object->visible = false;
                gemCount += value;
            }

            // traps have 3 frames, with animation in middle
            // this will animate, wait 0.4s, then  go back to "default state"
            else if (PNTR_STRCMP(object->type.ptr, "trap") == 0) {
                set_gid(object, 0, 1);
                gemCount -= value;
                bump_back(subject, 8);
                animation_queue_add(&animations, object, object->gid - 1, 0.4f, NULL);
            }
        }

        // anything can have a sound prop
        if (sound[0] != 0) {
            // sound_holder_t* s = sfx_load(&sounds, app, sound);
            // if (s != NULL && s->sound != NULL) {
            //     pntr_play_sound(s->sound, false);
            // }
        }
    }
}

bool Init(pntr_app* app) {
    font = pntr_load_font_default();
    
    // you can prelaod any maps too, just set currentMap to the one you want
    currentMap = adventure_load("assets/main.tmj", &maps);
    
    return true;
}

void Close(pntr_app* app) {
    while(maps != NULL) {
       adventure_unload(&maps);
    }
    // TODO: unload sounds
    // TODO: unload unfired animations
}


bool Update(pntr_app* app, pntr_image* screen) {
    float dt = pntr_app_delta_time(app);

    // no roopies, you're dead!
    if (gemCount < 0) {
        adventure_map_t* dialogMap = adventure_load("assets/dead.tmj", &maps);
        
        if (dialogMap != NULL && dialogMap->map != NULL) {
            pntr_clear_background(screen, dialogMap->map->backgroundcolor ? pntr_tiled_color(dialogMap->map->backgroundcolor) : PNTR_BLACK);
            pntr_update_tiled(dialogMap->map,  dt);
            dialogMap->player->y -= dt * (player_speed/8);
            pntr_draw_tiled(screen, dialogMap->map, 0, 0, PNTR_WHITE);
        }


        pntr_draw_text_wrapped(screen, font, "You died, penniless.\nSomeone will be along to attend your grave, forthwith.", 20, 180, 280, PNTR_RAYWHITE);

        // restart on SPACE
        if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
            gemCount = 0;
            // unload all maps to reset state
            while(maps != NULL) {
                adventure_unload(&maps);
            }
            currentMap = adventure_load("assets/main.tmj", &maps);
        }

        return true;
    }

    if (showTitle) {
        adventure_map_t* titleMap = adventure_load("assets/title.tmj", &maps);
        pntr_update_tiled(titleMap->map,  dt);
        pntr_clear_background(screen, titleMap->map->backgroundcolor ? pntr_tiled_color(titleMap->map->backgroundcolor) : PNTR_BLACK);
        pntr_draw_tiled(screen, titleMap->map, 0, 0, PNTR_WHITE);
        pntr_draw_text(screen, font, "the legend\n of pntr", 130, 100, PNTR_RAYWHITE);

        // start on SPACE
        if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
            showTitle = false;
        }

        return true;
    }

    // if dialog is open, just render that once
    else if (dialogText[0] != 0) {
        // 1-time render of dialog map
        if (!shownDialog) {
            shownDialog = true;
            adventure_map_t* dialogMap = adventure_load("assets/dialog.tmj", &maps);
            pntr_draw_tiled(screen, dialogMap->map, 0, 0, PNTR_WHITE);
            pntr_draw_text_wrapped(screen, font, dialogText, 20, 180, 280, PNTR_RAYWHITE);
        }
        // close on SPACE
        if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
            dialogText[0] = 0;
        }
        return true;
    }

    else if (currentMap != NULL) {
        animation_queue_run(&animations, dt);

        pntr_vector req = {0};
        pntr_vector camera = {0};

        if (currentMap->player != NULL){
            int gid_walking = 0;

            if (pntr_app_key_down(app, PNTR_APP_KEY_DOWN) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_DOWN)) {
                req.y += player_speed * dt;
                gid_direction = 0;
                gid_walking = 1;
            }
            else if (pntr_app_key_down(app, PNTR_APP_KEY_UP) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_UP)) {
                req.y -= player_speed * dt;
                gid_direction = 1;
                gid_walking = 1;
            }
            else if (pntr_app_key_down(app, PNTR_APP_KEY_RIGHT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_RIGHT)) {
                req.x += player_speed * dt;
                gid_direction = 2;
                gid_walking = 1;
            }
            else if (pntr_app_key_down(app, PNTR_APP_KEY_LEFT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_LEFT)) {
                req.x -= player_speed * dt;
                gid_direction = 3;
                gid_walking = 1;
            }

            set_gid(currentMap->player, gid_direction, gid_walking);

            // this requests the new position (but collisions or map bounds might deny)
            adventure_try_to_move_player(app, currentMap, &req, &player_hitbox, &CollisionCallback);
            adventure_camera_look_at(&camera, screen, currentMap->map, currentMap->player);
        }

        // TODO: update all objects that are not player
        
        pntr_update_tiled(currentMap->map,  dt);
        pntr_clear_background(screen, currentMap->map->backgroundcolor ? pntr_tiled_color(currentMap->map->backgroundcolor) : PNTR_BLACK);
        pntr_draw_tiled(screen, currentMap->map, camera.x, camera.y, PNTR_WHITE);

        if (gemCount > 0) {
            pntr_draw_text_ex(screen, font, 10, 10, PNTR_RAYWHITE, "GEMS: %d", gemCount);
        }
    }

    else {
        pntr_app_log(PNTR_APP_LOG_WARNING, "No map loaded.");
    }

    return true;
}

// this is not used directly, but I define it, because it seems needed for web
void Event(pntr_app* app, pntr_app_event* event) {}


pntr_app Main(int argc, char* argv[]) {
#ifdef PNTR_APP_RAYLIB
    SetTraceLogLevel(LOG_ERROR);
#endif

    return (pntr_app) {
        .width = 320,
        .height = 240,
        .title = "legend of pntr",
        .init = Init,
        .update = Update,
        .close = Close,
        .event = Event,
        .fps = 60
    };
}
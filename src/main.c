#define PNTR_APP_IMPLEMENTATION
#define PNTR_ENABLE_VARGS
#define PNTR_ENABLE_DEFAULT_FONT
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#include "pntr_tiled.h"

#include "adventure.h"

// default font for dialogs
static pntr_font* font;

// head of linked list
static adventure_map_t* maps = NULL;
static adventure_map_t* currentMap = NULL;

// these are pre-loaded maps used for UI
adventure_map_t* dialogMap = NULL;
adventure_map_t* titleMap = NULL;

// eventually, I could get these from the map somehow
static float player_speed = 200;

// on a 16x16 map, it's at the "body" of a charater tile
static pntr_rectangle player_hitbox = {4, 8, 8, 8};

// stores direction across frames, so if you face a way, it will keep facing that way
static int gid_direction = 0; 

// if there is a current-dialog, it triggers game to pause
static char dialogText[1024] = {0};

bool Init(pntr_app* app) {
    font = pntr_load_font_default();
    
    // you can prelaod maps, just set currentMap to the one you want
    dialogMap = adventure_load("assets/dialog.tmj", &maps);
    adventure_load("assets/dialog.tmj", &maps);
    adventure_load("assets/dialog.tmj", &maps);
    titleMap = adventure_load("assets/title.tmj", &maps);
    currentMap = adventure_load("assets/main.tmj", &maps);
    
    return true;
}

// this is called when the player or an NPC touches something
// object will be NULL, if it's static geometry (from collision layer)
void CollisionCallback(pntr_app* app, adventure_map_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object) {
    if (object == NULL) {
        printf("%s bumped static\n", subject->name.ptr);
    } else {
        // get all properties from object
        char filename[PNTR_PATH_MAX] = {0};
        char sound[PNTR_PATH_MAX] = {0};

        for (int i = 0; i < object->property_count; i++) {
            cute_tiled_property_t* prop = &object->properties[i];
            if (prop->type == CUTE_TILED_PROPERTY_STRING) {
                if (PNTR_STRCMP("text", prop->name.ptr) == 0) {
                    dialogText[0] = 0;
                    PNTR_STRCAT(dialogText, prop->data.string.ptr);
                }
                else if (PNTR_STRCMP("filename", prop->name.ptr) == 0) {
                    PNTR_STRCAT(filename, "assets/");
                    PNTR_STRCAT(filename, prop->data.string.ptr);
                }
                else if (PNTR_STRCMP("sound", prop->name.ptr) == 0) {
                    PNTR_STRCAT(filename, "assets/rfx/");
                    PNTR_STRCAT(filename, prop->data.string.ptr);
                }
            }
        }

        if (PNTR_STRCMP(subject->name.ptr, "player") == 0) {
            if (PNTR_STRCMP(object->type.ptr, "portal") == 0) {
                if (filename[0] != 0) {
                    PNTR_STRCAT(filename, ".tmj");
                    currentMap = adventure_load(filename, &maps);
                } else {
                    pntr_app_log(PNTR_APP_LOG_WARNING, "No filename set for portal.");
                }
            }
        }

        if (sound[0] != 0) {
            PNTR_STRCAT(sound, ".rfx");
            // TODO: play the sound from sound LL
        }
    }
}

// we can derive the correct tile (specific to my spritesheet layout)
// since each row (12 tiles) is a character, broken into 3 frames per direction
// with the 2nd frame as the indicator for "walking animation"
// this will work with my spritesheet, but you can adjust for yours, if it's differnt
static void set_gid(cute_tiled_object_t* character, int gid_direction, int gid_walking) {
    int gid_character = character->gid/12;
    character->gid = (gid_character*12) + 1 + gid_walking + (gid_direction*3);
}


bool Update(pntr_app* app, pntr_image* screen) {
    // if dialog is open, just render that once
    if (dialogText[0] != 0) {
        printf("SIGN: %s\n", dialogText);
        return true;
    }

    if (currentMap != NULL) {
        float dt = pntr_app_delta_time(app);
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
    } else {
        pntr_app_log(PNTR_APP_LOG_WARNING, "No map loaded.");
    }

    return true;
}

// this is not used directly, but I define it, because it seems needed for web
void Event(pntr_app* app, pntr_app_event* event) {}


void Close(pntr_app* app) {
    while(maps != NULL) {
       adventure_unload(&maps);
    }
}

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
#define PNTR_APP_IMPLEMENTATION
#define PNTR_ENABLE_VARGS
#define PNTR_ENABLE_DEFAULT_FONT
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#include "pntr_tiled.h"

#include "adventure.h"

// default font for dialogs
static pntr_font* font;
static adventure_map_t* maps;

// eventually, I could get these from the map somehow
static float player_speed = 200;

// on a 16x16 map, it's at the "body" of a charater tile
static pntr_rectangle player_hitbox = {4, 8, 8, 8};


bool Init(pntr_app* app) {
    font = pntr_load_font_default();
    maps = adventure_load("assets/main.tmj");
    return true;
}

// this is called when the player or an NPC touches something
// object will be null, if it's static geometry (from collision layer)
void CollisionCallback(pntr_app* app, adventure_map_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object) {
    if (object == NULL) {
        printf("%s bumped static\n", subject->name.ptr);
    } else {
        printf("%s bumped %s\n", subject->name.ptr, object->name.ptr);
    }
}


bool Update(pntr_app* app, pntr_image* screen) {
    float dt = pntr_app_delta_time(app);

    pntr_vector req = {0};
   
    if (pntr_app_key_down(app, PNTR_APP_KEY_LEFT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_LEFT)) {
        req.x -= player_speed * dt;
    }
    else if (pntr_app_key_down(app, PNTR_APP_KEY_RIGHT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_RIGHT)) {
        req.x += player_speed * dt;
    }
    if (pntr_app_key_down(app, PNTR_APP_KEY_UP) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_UP)) {
        req.y -= player_speed * dt;
    }
    else if (pntr_app_key_down(app, PNTR_APP_KEY_DOWN) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_DOWN)) {
        req.y += player_speed * dt;
    }

    // this requests the new position (but collisions or map bounds might deny)
    adventure_handle_input(app, maps, &req, &player_hitbox, &CollisionCallback);

    pntr_vector camera = {0};

    if (maps != NULL) {
        if (maps->player != NULL) {
            adventure_camera_look_at(&camera, screen, maps->map, maps->player);
        }
        pntr_update_tiled(maps->map,  dt);
        pntr_clear_background(screen, maps->map->backgroundcolor ? pntr_tiled_color(maps->map->backgroundcolor) : PNTR_BLACK);
        pntr_draw_tiled(screen, maps->map, camera.x, camera.y, PNTR_WHITE);
    }

    return true;
}

// this is not used directly, but I define it, because it seems needed for web
void Event(pntr_app* app, pntr_app_event* event) {}


void Close(pntr_app* app) {
    // TODO: cleanup
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
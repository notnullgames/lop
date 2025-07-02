#define PNTR_APP_IMPLEMENTATION
#define PNTR_ENABLE_DEFAULT_FONT
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#include "pntr_tiled.h"

#include "adventure.h"

static pntr_font* font;
static bool showTitle = true;
static char* signText = NULL;
static char* currentMapName = "welcome_island";
static adventure_maps_t* maps;

// called by adventure engine when player or object touches another object or a wall
void HandleCollision(pntr_app* app, adventure_maps_t* mapContainer, cute_tiled_object_t* object, cute_tiled_object_t* subject) {
    if (PNTR_STRCMP(object->name.ptr, "player") == 0) {
        printf("player touched %s\n", subject->name.ptr);
    } else {
    }
}

bool Init(pntr_app* app) {
    font = pntr_load_font_default();
    maps = adventure_load_all_maps("assets/");
    if (!maps) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "No maps found!");
        return false;
    }
    return true;
}

void Event(pntr_app* app, pntr_app_event* event) {}

bool Update(pntr_app* app, pntr_image* screen) {
    adventure_maps_t* currentMap;

    pntr_vector camera = {0};

    // choose map
    if (showTitle) {
        currentMap = adventure_find_map(maps, "title");
    }
    else if (signText != NULL) {
        currentMap = adventure_find_map(maps, "dialog");
    }
    else {
        currentMap = adventure_find_map(maps, currentMapName);
        adventure_update_rpg(app, currentMap, HandleCollision);
        adventure_camera_look_at(screen, currentMap, currentMap->player, &camera);
    }

    // draw map
    if (currentMap != NULL) {
        pntr_update_tiled(currentMap->map,  pntr_app_delta_time(app));
        pntr_clear_background(screen, currentMap->map->backgroundcolor ? pntr_tiled_color(currentMap->map->backgroundcolor) : PNTR_BLACK);
        pntr_draw_tiled(screen, currentMap->map, camera.x, camera.y, PNTR_WHITE);
    } else {
        pntr_app_log(PNTR_APP_LOG_ERROR, "No map!");
        return false;
    }

    // draw over map
    if (showTitle) {
        pntr_draw_text(screen, font, "The Legend of Pntr", 100, 100, PNTR_RAYWHITE);
        if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
            showTitle = false;
        }
    }
    else if (signText != NULL) {
        pntr_draw_text_wrapped(screen, font, signText, 20, 180, 280, PNTR_RAYWHITE);
    }

    return true;
}

void Close(pntr_app* app) {
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
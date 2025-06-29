#define PNTR_APP_IMPLEMENTATION
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#define PNTR_TILED_EXTERNAL_TILESETS
#include "pntr_tiled.h"

#include "adventure_map.h"

static pntr_image* appScreen;

// implement these game-specific functions, as sort of callbacks

// called when object (player, enemy, thing) needs to be drawn
void adventure_map_object_draw(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* object, AdventureDirection direction, bool walking, bool collided, int camera_x, int camera_y) {
  pntr_draw_tiled_tile(appScreen, map, object->gid, object->x - camera_x, object->y - camera_y, PNTR_WHITE);
}

// called when you player touches an object
void adventure_map_object_touch(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* player, cute_tiled_object_t* other) {
  printf("player touching %s:%s\n", other->type.ptr, other->name.ptr);
}

bool Init(pntr_app* app) {
  adventure_map_load("assets/welcome_island.tmj", NULL, ADVENTURE_DIRECTION_NONE);
  return true;
}

bool Update(pntr_app* app, pntr_image* screen) {
  appScreen = screen;
  adventure_map_update(app, screen);
  return true;
}

void Close(pntr_app* app) {
  adventure_map_unload();
}

pntr_app Main(int argc, char* argv[]) {
  return (pntr_app) {
    .width = 640,
    .height = 480,
    .title = "map test",
    .init = Init,
    .update = Update,
    .close = Close,
    .fps = 60
  };
}
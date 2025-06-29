#define PNTR_APP_IMPLEMENTATION
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#define PNTR_TILED_EXTERNAL_TILESETS
#include "pntr_tiled.h"

#include "adventure_map.h"

static pntr_vector position;

bool Init(pntr_app* app) {
  position = (pntr_vector) {.x=0, .y=0};
  adventure_map_load("assets/welcome_island.tmj", &position);
  return true;
}

bool Update(pntr_app* app, pntr_image* screen) {
  pntr_clear_background(screen, PNTR_BLACK);
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
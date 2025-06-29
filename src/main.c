#define PNTR_APP_IMPLEMENTATION
#define PNTR_ENABLE_DEFAULT_FONT
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#define PNTR_TILED_EXTERNAL_TILESETS
#include "pntr_tiled.h"

#include "adventure_map.h"


static pntr_app* myApp;
static pntr_vector portalPosition = {0};
static char* signText; 
static pntr_font* font;

// 20x15 map for making dialog background
static cute_tiled_map_t* dialog;

// implement these game-specific functions, as sort of callbacks

// called when object (player, enemy, thing) needs to be drawn
void adventure_map_object_draw(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* object, AdventureDirection direction, bool walking, bool collided, int camera_x, int camera_y) {
  pntr_draw_tiled_tile(myApp->screen, map, object->gid, object->x - camera_x, object->y - camera_y-16, PNTR_WHITE);
}

// called when you player touches an object
void adventure_map_object_touch(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* player, cute_tiled_object_t* other) {
  if (strcmp(other->type.ptr, "portal") == 0) {
    char filename[256];
    snprintf(filename, sizeof(filename), "assets/%s.tmj", other->name.ptr);
    portalPosition.x =  pntr_tiled_object_get_int(other, "pos_x", 0);
    portalPosition.y = pntr_tiled_object_get_int(other, "pos_y", 0);
    printf("transporting: %s %dx%d\n", other->name.ptr, portalPosition.x, portalPosition.y);
    adventure_map_load(filename, &portalPosition, ADVENTURE_DIRECTION_NONE);
    return;
  }

  // these might be seeprate later
  if (strcmp(other->type.ptr, "sign") == 0 || strcmp(other->type.ptr, "musing") == 0) {
    signText = pntr_tiled_object_get_string(other, "text");
  }
}

bool Init(pntr_app* app) {
  myApp = app;
  dialog =  pntr_load_tiled("assets/dialog.tmj");
  font = pntr_load_font_default();
  adventure_map_load("assets/welcome_island.tmj", NULL, ADVENTURE_DIRECTION_NONE);
  return true;
}

bool Update(pntr_app* app, pntr_image* screen) {
  if (signText != NULL) {
    pntr_draw_tiled(screen, dialog, 0, 0, PNTR_WHITE);
    pntr_draw_text(screen, font, signText, 20, 180, PNTR_RAYWHITE);
    if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
      signText = NULL;
    }
  } else {
    adventure_map_update(app, screen);
  }

  return true;
}

void Close(pntr_app* app) {
  adventure_map_unload();
  cute_tiled_free_map(dialog);
}

pntr_app Main(int argc, char* argv[]) {
  return (pntr_app) {
    .width = 320,
    .height = 240,
    .title = "the legend of pntr",
    .init = Init,
    .update = Update,
    .close = Close,
    .fps = 60
  };
}
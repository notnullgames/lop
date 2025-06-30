#define PNTR_APP_IMPLEMENTATION
#define PNTR_ENABLE_DEFAULT_FONT
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#define PNTR_TILED_EXTERNAL_TILESETS
#include "pntr_tiled.h"

#define PNTR_APP_SFX_IMPLEMENTATION
#include "pntr_app_sfx.h"

#include "adventure_map.h"


static pntr_app* myApp;
static pntr_vector portalPosition = {0};
static char* signText; 
static pntr_font* font;

// 20x15 map for making dialog background
static cute_tiled_map_t* dialog;

static pntr_sound* soundActivate;
static pntr_sound* soundCoin;
static pntr_sound* soundHurt;


// I use this to simplify interactive animations for non-characters
// they have 2 frames, each at 200ms
static int animWheel = 0;

static int gemCount = 0;
static bool title = true;

// this is used to make player jump back
static AdventureDirection playerDirection = ADVENTURE_DIRECTION_NONE;

// implement these game-specific functions, as sort of callbacks

// called when object (player, enemy, thing) needs to be drawn
void adventure_map_object_draw(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* object, AdventureDirection direction, bool walking, bool collided, int camera_x, int camera_y) {
  if (object->visible) {
    // handle player direction & animation
    // TODO: I could put this in a general function for players & enemies
    if (strcmp(object->name.ptr, "player") == 0) {
      playerDirection = direction;
      if (direction == ADVENTURE_DIRECTION_SOUTH) {
        if (walking) {
          object->gid = 2;
        }else {
          object->gid = 1;
        }
      } else if (direction == ADVENTURE_DIRECTION_NORTH) {
        if (walking) {
          object->gid = 5;
        } else {
          object->gid = 4;
        }
      }  else if (direction == ADVENTURE_DIRECTION_EAST) {
        if (walking) {
          object->gid = 8;
        } else {
          object->gid = 7;
        }
      } else if (direction == ADVENTURE_DIRECTION_WEST) {
        if (walking) {
          object->gid = 11;
        }else {
          object->gid = 10;
        }
      } else {
        object->gid = 1;
      }
    }

    // stop interactive animations in ~2 animaton-frames
    if (animWheel++ > 120) {
      switch(object->gid) {
        // traps
        case 158: object->gid = 157; break;
        case 161: object->gid = 160; break;
        case 164: object->gid = 163; break;
        case 167: object->gid = 166; break;
      }
    }
    pntr_draw_tiled_tile(myApp->screen, map, object->gid, object->x - camera_x, object->y - camera_y-16, PNTR_WHITE);
  }
}

// called when you player touches an object
void adventure_map_object_touch(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* player, cute_tiled_object_t* object) {
  if (strcmp(object->type.ptr, "portal") == 0) {
    char filename[256];
    snprintf(filename, sizeof(filename), "assets/%s.tmj", object->name.ptr);
    portalPosition.x =  pntr_tiled_object_get_int(object, "pos_x", 0);
    portalPosition.y = pntr_tiled_object_get_int(object, "pos_y", 0);
    // printf("transporting: %s %dx%d\n", object->name.ptr, portalPosition.x, portalPosition.y);
    pntr_play_sound(soundActivate, false);
    adventure_map_load(filename, &portalPosition, ADVENTURE_DIRECTION_NONE);
    return;
  }

  // these might be seperate later
  if (strcmp(object->type.ptr, "sign") == 0 || strcmp(object->type.ptr, "musing") == 0) {
    signText = pntr_tiled_object_get_string(object, "text");
    pntr_play_sound(soundActivate, false);
  }

  if (strcmp(object->type.ptr, "loot") == 0) {
    // You could use properties or name here to determine what the loot does, I am just hiding it
    pntr_play_sound(soundCoin, false);
    object->visible = false;
    gemCount++;
  }
  

  // TRAPS
  // look at gid to determine if it's been sprung
  if (object->gid == 157 || object->gid == 160 || object->gid == 163 || object->gid == 166){
    printf("trap! %s - %d\n", object->name.ptr, object->gid);
    pntr_play_sound(soundHurt, false);
    gemCount--;
  }
  
  // turn static sprite into anim-sprite
  switch(object->gid) {
    case 157: object->gid = 158; break;
    case 160: object->gid = 161; break;
    case 163: object->gid = 164; break;
    case 166: object->gid = 167; break;
  }

  // all touches reset anim-wheeal
  animWheel = 0;
}

bool Init(pntr_app* app) {
  myApp = app;
  dialog =  pntr_load_tiled("assets/dialog.tmj");
  font = pntr_load_font_default();
  adventure_map_load("assets/welcome_island.tmj", NULL, ADVENTURE_DIRECTION_NONE);

  // load some SFX from https://raylibtech.itch.io/rfxgen
  SfxParams sfx = {0};
  pntr_app_sfx_load_params(&sfx, "assets/rfx/activate.rfx");
  soundActivate =  pntr_app_sfx_sound(app, &sfx);

  pntr_app_sfx_load_params(&sfx, "assets/rfx/coin.rfx");
  soundCoin = pntr_app_sfx_sound(app, &sfx);

  pntr_app_sfx_load_params(&sfx, "assets/rfx/hurt.rfx");
  soundHurt =  pntr_app_sfx_sound(app, &sfx);

  // it was too loud
  // this can also be done in settings when you save the sound
  pntr_set_volume(soundCoin, 0.25);

  return true;
}

void Event(pntr_app* app, pntr_app_event* event) {
  // needs event handler to fire key events, but it's unused
}

bool Update(pntr_app* app, pntr_image* screen) {
  // pntr_app_key_pressed would be better here, but it was not working on web

  if (title) {
    pntr_clear_background(screen, PNTR_BLUE);
    pntr_draw_text(screen, font, "THE LEGEND", 120, 120, PNTR_RAYWHITE);
    pntr_draw_text(screen, font, "OF PNTR", 130, 130, PNTR_RAYWHITE);
    if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
      title = false;
    }
    return true;
  }

  // very simple death-system
  if (gemCount < 0) {
    pntr_clear_background(screen, PNTR_RED);
    pntr_draw_text(screen, font, "YOU DIED!", 120, 120, PNTR_BLACK);
    if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
      gemCount = 0;
      adventure_map_load("assets/welcome_island.tmj", NULL, ADVENTURE_DIRECTION_NONE);
    }
    return true;
  }

  if (signText != NULL) {
    pntr_draw_tiled(screen, dialog, 0, 0, PNTR_WHITE);
    pntr_draw_text(screen, font, signText, 20, 180, PNTR_RAYWHITE);

    if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
      signText = NULL;
    }
  } else {
    adventure_map_update(app, screen);
  }

  char gemText[10];
  sprintf(gemText, "GEMS: %d", gemCount);
  pntr_draw_text(screen, font, gemText, 10, 10, PNTR_RAYWHITE);
  

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
    .event = Event,
    .fps = 60
  };
}
// This wraps  map with a few helpers for a specific game-type (top-down roguelike adventure)
// The idea is that all the objects in the map get an intiial-state on load, then 

#include <math.h>

// a direction a player can face
typedef enum {
  ADVENTURE_DIRECTION_NONE,
  ADVENTURE_DIRECTION_NORTH,
  ADVENTURE_DIRECTION_SOUTH,
  ADVENTURE_DIRECTION_EAST,
  ADVENTURE_DIRECTION_WEST
} AdventureDirection;

typedef struct {
  cute_tiled_map_t* map;
  cute_tiled_layer_t* collision;
  cute_tiled_layer_t* objects;
  AdventureDirection direction;
  cute_tiled_object_t* player;
  pntr_rectangle player_hitbox;
  bool walking;
  bool collided;
} AdventureMap;

// implement these game-specific functions, as sort of callbacks

// called when object (player, enemy, thing) needs to be drawn
void adventure_map_object_draw(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* object, AdventureDirection direction, bool walking, bool collided, int camera_x, int camera_y);

// called when you player touches an object
void adventure_map_object_touch(cute_tiled_map_t* map, cute_tiled_layer_t* objects, cute_tiled_object_t* player, cute_tiled_object_t* other);


static AdventureMap* currentMap;

// this is the speed the player walks, and is determined by player's animation-speed
static float walkSpeed = 2.0f;


// General helper to find an object by name on an object-layer
cute_tiled_object_t* pntr_tiled_object(cute_tiled_layer_t* objects_layer, const char* name) {
  if (objects_layer == NULL || objects_layer->objects == NULL) {
    return NULL;
  }
  cute_tiled_object_t* current = objects_layer->objects;
  while (current != NULL) {
    if (current->name.ptr != NULL && strcmp(current->name.ptr, name) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

// General helper to get a string-property from a tiled object
const char*  pntr_tiled_object_get_string(cute_tiled_object_t* object, const char* property_name) {
  if (object == NULL || property_name == NULL) {
    return NULL;
  }
  for (int i = 0; i < object->property_count; i++) {
    cute_tiled_property_t* prop = &object->properties[i];
    if (prop->name.ptr != NULL && strcmp(prop->name.ptr, property_name) == 0) {
      if (prop->type == CUTE_TILED_PROPERTY_STRING) {
        return prop->data.string.ptr;
      }
    }
  }
  printf("string prop not found: %s->%s\n", object->name.ptr, property_name);
  return NULL;
}

// General helper to get a float-property from a tiled object
float pntr_tiled_object_get_float(cute_tiled_object_t* object, const char* property_name, float default_value) {
    if (object == NULL || property_name == NULL) {
        return default_value;
    }
    for (int i = 0; i < object->property_count; i++) {
        cute_tiled_property_t* prop = &object->properties[i];
        
        if (prop->name.ptr != NULL && strcmp(prop->name.ptr, property_name) == 0) {
            if (prop->type == CUTE_TILED_PROPERTY_FLOAT) {
                return prop->data.floating;
            }
        }
    }
    printf("float prop not found: %s->%s\n", object->name.ptr, property_name);
    return default_value;
}

// General helper to get a int-property from a tiled object
float pntr_tiled_object_get_int(cute_tiled_object_t* object, const char* property_name, int default_value) {
    if (object == NULL || property_name == NULL) {
        return default_value;
    }
    for (int i = 0; i < object->property_count; i++) {
        cute_tiled_property_t* prop = &object->properties[i];
        
        if (prop->name.ptr != NULL && strcmp(prop->name.ptr, property_name) == 0) {
            if (prop->type == CUTE_TILED_PROPERTY_INT) {
                return prop->data.integer;
            }
        }
    }
    printf("int prop not found: %s->%s\n", object->name.ptr, property_name);
    return default_value;
}

// Get animations frames info from a gid
cute_tiled_frame_t* pntr_tiled_animation_frames(cute_tiled_map_t* map, int gid, int* frame_count) {
    if (gid <= 0 || !map || !frame_count) {
        *frame_count = 0;
        return NULL;
    }
    
    // Get the pntr_tiled internal tile data
    pntr_tiled_tile* tiles = (pntr_tiled_tile*)map->tiledversion.ptr;
    pntr_tiled_tile* tile = tiles + gid - 1;
    
    // Check if this tile has a descriptor with animation data
    if (tile->descriptor && tile->descriptor->frame_count > 0) {
        *frame_count = tile->descriptor->frame_count;
        return tile->descriptor->animation; // This is the cute_tiled_frame_t array
    }
    
    *frame_count = 0;
    return NULL;
}

// General helper to get a pntr_color from a tiled color
pntr_color pntr_tiled_color(uint32_t color) {
    if (color > 0xFFFFFF) {
        // Has alpha channel
        return pntr_new_color((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
    } else {
        // No alpha, default to fully opaque
        return pntr_new_color((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF);
    }
}

// Unload the current map
void adventure_map_unload() {
  if (currentMap != NULL) {
    cute_tiled_free_map(currentMap->map);
    pntr_unload_memory((void*)currentMap);
  }
}

// Load a tiled map and set things up
void adventure_map_load(const char* filename, pntr_vector* position, AdventureDirection direction) {
  if (currentMap != NULL) {
    adventure_map_unload();
  }
  currentMap = pntr_load_memory(sizeof(AdventureMap));
  currentMap->map = pntr_load_tiled(filename);

  currentMap->walking = false;
  currentMap->collided = false;

  // preset named layers that do extra stuff
  currentMap->collision = pntr_tiled_layer(currentMap->map, "collision");
  currentMap->objects = pntr_tiled_layer(currentMap->map, "objects");

  currentMap->player = pntr_tiled_object(currentMap->objects, "player");

  if (currentMap->player != NULL) {
    // if direction is not set, use player-object in map
    if (direction == ADVENTURE_DIRECTION_NONE) {
      const char* directionStr = pntr_tiled_object_get_string(currentMap->player, "direction");
      if (directionStr != NULL) {
        if (strcmp(directionStr, "NORTH") == 0) {
          direction = ADVENTURE_DIRECTION_NORTH;
        }else if (strcmp(directionStr, "SOUTH") == 0) {
          direction = ADVENTURE_DIRECTION_SOUTH;
        } else if (strcmp(directionStr, "EAST") == 0) {
          direction = ADVENTURE_DIRECTION_EAST;
        } else if (strcmp(directionStr, "WEST") == 0) {
          direction = ADVENTURE_DIRECTION_WEST;
        }
      }
    }
    // if position param is set, set player-object in map
    if (position != NULL) {
      currentMap->player->x = position->x;
      currentMap->player->y = position->y;
    }

    // set player hitbox from props or defaults
    currentMap->player_hitbox = (pntr_rectangle) {
      .x = pntr_tiled_object_get_float(currentMap->player, "hit_x", 4),
      .y = pntr_tiled_object_get_float(currentMap->player, "hit_y", 8),
      .width = pntr_tiled_object_get_float(currentMap->player, "hit_width", 8),
      .height = pntr_tiled_object_get_float(currentMap->player, "hit_height", 8),
    };

    // try to figure out speed from first frame of animation
    int frame_count = 0;
    cute_tiled_frame_t* playerFrame = pntr_tiled_animation_frames(currentMap->map, currentMap->player->gid, &frame_count);
    if (playerFrame != NULL) {
      // printf("player frames: %dms * %d frames\n", playerFrame->duration, frame_count);
      walkSpeed = playerFrame->duration/frame_count;
    }
  }

  currentMap->direction = direction;

  // don't show "collision" layer
  if (currentMap->collision != NULL) {
    currentMap->collision->opacity = 0;
  }
}

// check for collisions with "collision" layer
bool adventure_map_check_collision(float x, float y, pntr_rectangle hitbox) {
    if (currentMap->collision == NULL) {
        return false;
    }

    // Calculate hitbox bounds at the given position
    float hitbox_left = x + hitbox.x;
    float hitbox_right = x + hitbox.x + hitbox.width;
    float hitbox_top = y + hitbox.y;
    float hitbox_bottom = y + hitbox.y + hitbox.height;

    // Convert pixel coordinates to tile coordinates with offset correction
    int tile_left = (int)floor(hitbox_left / currentMap->map->tilewidth);
    int tile_right = (int)floor((hitbox_right - 1) / currentMap->map->tilewidth);
    int tile_top = (int)floor(hitbox_top / currentMap->map->tileheight) - 1;  // Subtract 1 tile
    int tile_bottom = (int)floor((hitbox_bottom - 1) / currentMap->map->tileheight) - 1;  // Subtract 1 tile

    // Check bounds to avoid accessing invalid tiles
    if (tile_left < 0) tile_left = 0;
    if (tile_top < 0) tile_top = 0;
    if (tile_right >= currentMap->map->width) tile_right = currentMap->map->width - 1;
    if (tile_bottom >= currentMap->map->height) tile_bottom = currentMap->map->height - 1;

    // Check all tiles that the hitbox overlaps
    for (int ty = tile_top; ty <= tile_bottom; ty++) {
        for (int tx = tile_left; tx <= tile_right; tx++) {
            int tile_index = ty * currentMap->map->width + tx;
            
            // Check if tile has collision (non-zero tile ID)
            if (currentMap->collision->data[tile_index] != 0) {
                return true;
            }
        }
    }

    return false;
}

// check for objects collision on "objects" layer
cute_tiled_object_t* adventure_map_check_object_collision(float x, float y, pntr_rectangle hitbox, const char* originatingName) {
  if (currentMap->objects == NULL) {
      return false;
  }
  cute_tiled_object_t* obj = currentMap->objects->objects;
  while (obj != NULL) {
    if (strcmp(originatingName, obj->name.ptr) != 0 && obj->visible) {
      // Calculate object bounds
      float obj_left = obj->x;
      float obj_right = obj->x + obj->width;
      float obj_top = obj->y;
      float obj_bottom = obj->y + obj->height;

      // Calculate hitbox bounds at the given position
      float hitbox_left = x + hitbox.x;
      float hitbox_right = x + hitbox.x + hitbox.width;
      float hitbox_top = y + hitbox.y;
      float hitbox_bottom = y + hitbox.y + hitbox.height;

      // Check for rectangle overlap
      bool collision = !(hitbox_right <= obj_left || hitbox_left >= obj_right || hitbox_bottom <= obj_top || hitbox_top >= obj_bottom);

      if (collision) {
        return obj;
      }
    }
    obj = obj->next;
  }
  return NULL;
}


// try to move in a direction, obey collisions, trigger object-touch events
void adventure_game_try_to_move(AdventureDirection direction, pntr_app* app) {
  if (currentMap->player != NULL && direction != ADVENTURE_DIRECTION_NONE) {
    float x = currentMap->player->x;
    float y = currentMap->player->y;
    float newx = x;
    float newy = y;
    if (direction == ADVENTURE_DIRECTION_NORTH) {
      newy -= walkSpeed * pntr_app_delta_time(app);
    } else if (direction == ADVENTURE_DIRECTION_SOUTH) {
      newy += walkSpeed * pntr_app_delta_time(app);
    } else if (direction == ADVENTURE_DIRECTION_EAST) {
      newx += walkSpeed * pntr_app_delta_time(app);
    } else if (direction == ADVENTURE_DIRECTION_WEST) {
      newx -= walkSpeed * pntr_app_delta_time(app);
    }

    currentMap->walking = false;
    currentMap->collided = true;
    currentMap->direction = direction;

    bool wallCollision = adventure_map_check_collision(newx, newy, currentMap->player_hitbox);
    cute_tiled_object_t* colidedObject =  adventure_map_check_object_collision(newx, newy, currentMap->player_hitbox, "player");

    // no collision, go ahead
    if (!wallCollision && colidedObject == NULL) {
      currentMap->player->x = newx;
      currentMap->player->y = newy;
      currentMap->walking = true;
      currentMap->collided = false;
    }

    // trigger user's callback for object
    if (colidedObject != NULL) {
      adventure_map_object_touch(currentMap->map, currentMap->objects, currentMap->player, colidedObject);
    }
  }
}

// get the current map
AdventureMap* adventure_map_get() {
  return &currentMap;
}

void adventure_map_update(pntr_app* app, pntr_image* screen) {
  pntr_update_tiled(currentMap->map,  pntr_app_delta_time(app));

  if (currentMap->player != NULL) {
    bool keyLeft = pntr_app_key_down(app, PNTR_APP_KEY_LEFT);
    bool keyRight = pntr_app_key_down(app, PNTR_APP_KEY_RIGHT);
    bool keyUp = pntr_app_key_down(app, PNTR_APP_KEY_UP);
    bool keyDown = pntr_app_key_down(app, PNTR_APP_KEY_DOWN);

    if (keyRight) {
      adventure_game_try_to_move(ADVENTURE_DIRECTION_EAST, app);
    }
    if (keyLeft) {
      adventure_game_try_to_move(ADVENTURE_DIRECTION_WEST, app);
    }
    if (keyDown) {
      adventure_game_try_to_move(ADVENTURE_DIRECTION_SOUTH, app);
    }
    if (keyUp) {
      adventure_game_try_to_move(ADVENTURE_DIRECTION_NORTH, app);
    }

    if (!keyLeft && !keyRight && !keyUp && !keyDown) {
      currentMap->walking = false;
    }

    int camera_x = (int)(currentMap->player->x - screen->width / 2);
    int camera_y = (int)(currentMap->player->y - screen->height / 2);
    
    if (currentMap->map->backgroundcolor) {
      pntr_clear_background(screen, pntr_tiled_color(currentMap->map->backgroundcolor));
    } else {
      pntr_clear_background(screen, PNTR_BLACK);
    }

    pntr_draw_tiled(screen, currentMap->map, -camera_x, -camera_y, PNTR_WHITE);

    if (currentMap && currentMap->map && currentMap->objects && currentMap->player) {
    cute_tiled_object_t* obj = currentMap->objects->objects;
      while (obj != NULL) {
        adventure_map_object_draw(currentMap->map,currentMap->objects, obj, currentMap->direction, currentMap->walking, currentMap->collided, camera_x, camera_y);
        obj = obj->next;
      }
    }
  }
}


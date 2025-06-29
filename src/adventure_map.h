// This wraps  map with a few helpers for a specific game-type


typedef struct {
  cute_tiled_map_t* map;
  pntr_vector* position;
} AdventureMap;

static AdventureMap* currentMap;

// Helper function for recursive search through layers
cute_tiled_layer_t* cute_tiled_find_layer(cute_tiled_layer_t* layers, const char* layer_name) {
  if (layers == NULL || layer_name == NULL) {
    return NULL;
  }
  cute_tiled_layer_t* current_layer = layers;
  while (current_layer != NULL) {
    if (current_layer->name.ptr != NULL && strcmp(current_layer->name.ptr, layer_name) == 0) {
      return current_layer;
    }
    if (current_layer->layers != NULL) {
      cute_tiled_layer_t* found = cute_tiled_find_layer(current_layer->layers, layer_name);
      if (found != NULL) {
        return found;
      }
    }
    current_layer = current_layer->next;
  }
  return NULL;
}


void adventure_map_load(const char* filename, pntr_vector* position) {
  if (currentMap != NULL) {
  pntr_unload_memory((void*)currentMap);
  position->x = 0;
  position->y = 0;
  }
  currentMap = pntr_load_memory(sizeof(AdventureMap));
  currentMap->map = pntr_load_tiled(filename);
  currentMap->position = position;

  // don't show "collision" layer
  cute_tiled_layer_t* collisions = cute_tiled_find_layer(currentMap->map->layers, "collision");
  if (collisions != NULL) {
    collisions->opacity = 0;
  }
}

void adventure_map_update(pntr_app* app, pntr_image* screen) {
  pntr_update_tiled(currentMap->map, pntr_app_delta_time(app));
  pntr_draw_tiled(screen, currentMap->map, currentMap->position->x, currentMap->position->y, PNTR_WHITE);
}

void adventure_map_unload() {
  pntr_unload_memory((void*)currentMap);
}
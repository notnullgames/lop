typedef enum {
    DIRECTION_SOUTH,
    DIRECTION_NORTH,
    DIRECTION_EAST,
    DIRECTION_WEST,
} AdventureDirection;

typedef struct {
    int walking[4];
    int still[4];
} AdventureGidMap;

typedef void (*AdventureCollisionCallback)(cute_tiled_object_t* moving, cute_tiled_object_t* touching);

typedef struct {
    cute_tiled_map_t* map;
    cute_tiled_layer_t* collision;
    cute_tiled_layer_t* objects;
    cute_tiled_object_t* player;
    float speed;
    AdventureDirection direction;
    bool walking;
    AdventureGidMap gids;
    pntr_rectangle hitbox;
    AdventureCollisionCallback onCollide;
} AdventureInfo;

#define MIN(a,b) (a < b) ? a : b;
#define MAX(a,b) (a > b) ? a : b;

// this loads a tilemap and sets up an adventure object
AdventureInfo* adventure_load(char* filename, AdventureGidMap gids) {
    AdventureInfo* adventure = pntr_load_memory(sizeof(AdventureInfo));
    adventure->map = pntr_load_tiled(filename);
    if (!adventure->map) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Adventure: could not load map.");
        return NULL;
    }

    // sprite-set covers S/N/E/W walking/still gids
    adventure->gids = gids;

    // standard 16x16 characters, this puts it over their body
    // adjust adventure->hitbox in your game, if you need to
    adventure->hitbox = (pntr_rectangle) { 4, 8, 8, 8 };

    cute_tiled_layer_t* layer = adventure->map->layers;
    while (layer) {
        if (PNTR_STRCMP(layer->name.ptr, "objects") == 0) {
            adventure->objects = layer;
        }
        if (PNTR_STRCMP(layer->name.ptr, "collision") == 0) {
            adventure->collision = layer;
        }
        layer = layer->next;
    }

    if (adventure->objects == NULL) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Could not load objects.");
        return NULL;
    }

    if (adventure->collision == NULL) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Could not load collision.");
        return NULL;
    }

    cute_tiled_object_t* current = adventure->objects->objects;
    while (current != NULL) {
        if (current->name.ptr != NULL && strcmp(current->name.ptr, "player") == 0) {
            adventure->player = current;
        }
        current = current->next;
    }


    if (adventure->player == NULL) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Could not find player.");
        return NULL;
    }

    adventure->collision->opacity = 0;

    // this could also come from player animation
    adventure->speed = 100;

    return adventure;
}

// AABB collision of positions of 2 objects
// visible means "it must be visible to collide"
// id is the unique-id (not gid) of the object, so you can make sure not to collide with the the thing doing the colliding
cute_tiled_object_t* adventure_object_collides(cute_tiled_map_t* map, cute_tiled_layer_t* collision_layer, pntr_rectangle hitbox, int id, bool visible) {
    cute_tiled_object_t* obj = collision_layer->objects;

    while (obj) {
        if ((!visible || obj->visible) && id != obj->id) {
            float objY = obj->y; // Default y for non-tile objects

            float hitY = hitbox.y;

            // Adjust y for tile objects vs shapes
            if (obj->gid == 0) {
                objY += map->tileheight; 
            } else {
                objY -= map->tileheight;
            }

            if (hitbox.x < obj->x + obj->width &&
                hitbox.x + hitbox.width > obj->x &&
                hitY < objY + obj->height &&
                hitY + hitbox.height > objY) {
                printf("object collision: %d\n", obj->gid);
                return obj;
            }
        }
        obj = obj->next;
    }
    return NULL;
}


// tries to move an object, obeying collisions
void adventure_move_thing(AdventureInfo* adventure, cute_tiled_object_t* thing, pntr_rectangle hitbox, float dx, float dy) {
    // TODO: check map-bounds (things cannot leave map)
    pntr_rectangle h = {thing->x+dx+hitbox.x, thing->y+dy+hitbox.y + adventure->map->tileheight, hitbox.width, hitbox.height};
    
    cute_tiled_object_t* obj = adventure_object_collides(adventure->map, adventure->objects, h, adventure->player->id, true);
    cute_tiled_object_t* wall = adventure_object_collides(adventure->map, adventure->collision, h, adventure->player->id, false);
    if (wall == NULL) {
        thing->x += dx;
        thing->y += dy;
    }
    if (adventure->onCollide) {
        if (obj != NULL){
            adventure->onCollide(adventure->player, obj);
        }
        if (wall != NULL){
            adventure->onCollide(adventure->player, wall);
        }
    }
}

// show rectangles for all object bounding boxes
void adventure_debug_bounding(pntr_image* screen, cute_tiled_map_t* map, cute_tiled_layer_t* layer, int camera_x, int camera_y, pntr_color color) {
    cute_tiled_object_t* obj = layer->objects;
    while (obj) {
        if (obj->gid) {
            pntr_draw_rectangle_fill(screen, obj->x - camera_x, obj->y - camera_y - map->tileheight, obj->width, obj->height, color);
        }else {
            pntr_draw_rectangle_fill(screen, obj->x - camera_x, obj->y - camera_y, obj->width, obj->height, color);
        }
        obj = obj->next;
    }
}

// this draws everything using camera (zelda-style)
void adventure_update_rpg(pntr_app* app, pntr_image* screen, AdventureInfo* adventure, bool debug) {
    float dt = pntr_app_delta_time(app);

    // Keyboard/Gamepad
    adventure->walking=false;
    if (pntr_app_key_down(app, PNTR_APP_KEY_LEFT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_LEFT)) {
        adventure_move_thing(adventure, adventure->player, adventure->hitbox, -adventure->speed * dt, 0);
        adventure->walking=true;
        adventure->direction = DIRECTION_WEST;
    }
    else if (pntr_app_key_down(app, PNTR_APP_KEY_RIGHT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_RIGHT)) {
        adventure_move_thing(adventure, adventure->player, adventure->hitbox, adventure->speed * dt, 0);
        adventure->walking=true;
        adventure->direction = DIRECTION_EAST;
    }
    if (pntr_app_key_down(app, PNTR_APP_KEY_UP) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_UP)) {
        adventure_move_thing(adventure, adventure->player, adventure->hitbox, 0, -adventure->speed * dt);
        adventure->walking=true;
        adventure->direction = DIRECTION_NORTH;
    }
    else if (pntr_app_key_down(app, PNTR_APP_KEY_DOWN) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_DOWN)) {
        adventure_move_thing(adventure, adventure->player, adventure->hitbox, 0, adventure->speed * dt);
        adventure->walking=true;
        adventure->direction = DIRECTION_SOUTH;
    }

    // choose animated/still in correct direction
    adventure->player->gid = adventure->walking ? adventure->gids.walking[adventure->direction] : adventure->gids.still[adventure->direction];

    pntr_update_tiled(adventure->map,  pntr_app_delta_time(app));
    int camera_x = MAX(0, adventure->player->x - screen->width / 2);
    int camera_y = MAX(0, adventure->player->y - screen->height / 2);
    camera_x = MIN(camera_x, (adventure->map->width * adventure->map->tilewidth) - screen->width);
    camera_y = MIN(camera_y, (adventure->map->height * adventure->map->tileheight) - screen->height);

    // draw all layers of map
    pntr_draw_tiled(screen, adventure->map, -camera_x, -camera_y, PNTR_WHITE);

    if (debug) {
        // draw player hitbox
        pntr_draw_rectangle_fill(screen, adventure->hitbox.x + adventure->player->x-camera_x, adventure->hitbox.y - camera_y - adventure->map->tileheight, adventure->hitbox.width, adventure->hitbox.height, pntr_new_color(200, 122, 255, 160));

        // draw object bounding-boxes
        adventure_debug_bounding(screen, adventure->map, adventure->objects, camera_x, camera_y, pntr_new_color(102, 191, 255, 160));

        // draw collision bounding-boxes
        adventure_debug_bounding(screen, adventure->map, adventure->collision, camera_x, camera_y, pntr_new_color(0, 158, 47, 160));
    }
}



// call this when you finish with an adventure object
void adventure_unload(AdventureInfo* adventure) {
    if (adventure != NULL) {
        cute_tiled_free_map(adventure->map);
    }
}
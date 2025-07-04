#include "math.h"

// return least of 2 numbers
#ifndef MIN
#define MIN(a,b) ((a < b) ? a : b)
#endif

// return greatest of 2 numbers
#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif

// get absolute value (distance from 0)
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

// pop from front of LL
#ifndef LL_POP
#define LL_POP(head, node) do { (node) = (head); if (head) (head) = (head)->next; } while(0)
#endif

// push to front of LL
#ifndef LL_PUSH
#define LL_PUSH(head, node) do { (node)->next = (head); (head) = (node); } while(0)
#endif

// simple collision
#ifndef RECTS_OVERLAP
#define RECTS_OVERLAP(ax, ay, aw, ah, bx, by, bw, bh) ((ax) < (bx) + (bw) && (ax) + (aw) > (bx) && (ay) < (by) + (bh) && (ay) + (ah) > (by))
#endif

// linked list
// this allows you to use to use as a single-map or list of preloaded maps
typedef struct adventure_map_t {
    cute_tiled_map_t* map;
    cute_tiled_object_t* player;
    cute_tiled_layer_t* layer_objects;
    cute_tiled_layer_t* layer_collisions;
    struct adventure_map_t* next;
    char* filename;
} adventure_map_t;

// called when anythign touches wall or other object
typedef void (*AdventureCollisionCallback)(pntr_app* app, adventure_map_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object);


// check tiles (on a tile layer) around an object
// any tile on collision-layer will trigger a hit
bool adventure_check_static_collision(cute_tiled_map_t* map, cute_tiled_layer_t* layer, const pntr_rectangle* rect){
    if (map == NULL || layer == NULL || rect == NULL ) {
        return false;
    }
    int tile_x0 = MAX((int)(rect->x) / map->tilewidth, 0);
    int tile_y0 = MAX((int)(rect->y) / map->tileheight, 0);
    int tile_x1 = MIN((int)((rect->x + rect->width  - 1)) / map->tilewidth, layer->width - 1);
    int tile_y1 = MIN((int)((rect->y + rect->height - 1)) / map->tileheight, layer->height);
    for (int ty = tile_y0; ty <= tile_y1; ++ty) {
        for (int tx = tile_x0; tx <= tile_x1; ++tx) {
            int idx = (ty-1) * layer->width + tx;
            if (idx >= 0 && idx < layer->data_count && layer->data[idx] != 0) {
                return true;
            }
        }
    }
    return false;
}


// check if any objects (on a layer) collide with an object & retiurn first that does
cute_tiled_object_t* adventure_check_object_collision(cute_tiled_layer_t* layer, const pntr_rectangle* rect, cute_tiled_object_t* subject){
    if (subject == NULL || rect == NULL || layer == NULL) {
        return NULL;
    }
    for (cute_tiled_object_t* obj = layer->objects; obj; obj = obj->next) {
        if (obj->visible && obj->id != subject->id && RECTS_OVERLAP(rect->x, rect->y, rect->width, rect->height, obj->x, obj->y, obj->width, obj->height)) {
            return obj;
        }
    }
    return NULL;
}


// Shared helper: computes movement direction (+1, -1, or 0) for one axis
static float compute_axis_step(float obj_pos, float target_pos, float speed, int towards) {
    float delta = target_pos - obj_pos;
    if (fabsf(delta) > 0.01f) {
        float dir = (delta > 0) ? 1.0f : -1.0f;
        return towards ? dir * speed : -dir * speed;
    }
    return 0.0f;
}

// move towards/away from object
void adventure_move_object_relative_to_object(
    cute_tiled_map_t* map,
    cute_tiled_layer_t* collision_layer,
    cute_tiled_object_t* obj,
    float player_x,
    float player_y,
    float speed,        // pixels per frame
    int towards         // 1 = move towards, 0 = move away
) {
    float dx = player_x - obj->x;
    float dy = player_y - obj->y;

    float move_x = 0, move_y = 0;

    // Move in the axis with the greatest absolute distance
    if (fabsf(dx) > fabsf(dy)) {
        move_x = compute_axis_step(obj->x, player_x, speed, towards);
    } else if (fabsf(dy) > 0) {
        move_y = compute_axis_step(obj->y, player_y, speed, towards);
    }

    // Try to move in the chosen direction
    pntr_rectangle rect = { obj->x + move_x, obj->y + move_y, obj->width, obj->height };
    if (!adventure_check_static_collision(map, collision_layer, &rect)) {
        obj->x += move_x;
        obj->y += move_y;
    } else {
        // Try moving only in x
        pntr_rectangle rect_x = { obj->x + move_x, obj->y, obj->width, obj->height };
        if (!adventure_check_static_collision(map, collision_layer, &rect_x)) {
            obj->x += move_x;
        } else {
            // Try moving only in y
            pntr_rectangle rect_y = { obj->x, obj->y + move_y, obj->width, obj->height };
            if (!adventure_check_static_collision(map, collision_layer, &rect_y)) {
                obj->y += move_y;
            }
        }
    }
}

// same as adventure_move_object_relative_to_object, but has an awareness radius
void adventure_move_object_relative_to_close_object(
    cute_tiled_map_t* map,
    cute_tiled_layer_t* collision_layer,
    cute_tiled_object_t* obj,
    float player_x,
    float player_y,
    float speed,
    int towards,         // 1 = move towards, 0 = move away
    int awareness        // radius in tiles
) {
    // Calculate tile positions
    int obj_tile_x = (int)(obj->x / map->tilewidth);
    int obj_tile_y = (int)(obj->y / map->tileheight);
    int player_tile_x = (int)(player_x / map->tilewidth);
    int player_tile_y = (int)(player_y / map->tileheight);

    // Manhattan distance in tiles
    int dx = abs(player_tile_x - obj_tile_x);
    int dy = abs(player_tile_y - obj_tile_y);

    if ((dx + dy) <= awareness) {
        // Move only if within awareness radius
        adventure_move_object_relative_to_object(
            map,
            collision_layer,
            obj,
            player_x,
            player_y,
            speed,
            towards
        );
    }
}


// take a request for movement (after reading input) and fire callback on collision
void adventure_try_to_move_player(pntr_app* app, adventure_map_t* maps, pntr_vector* req, pntr_rectangle* hitbox, AdventureCollisionCallback callback) {
    if (maps == NULL || app == NULL) {
        return;
    }

    float dt = pntr_app_delta_time(app);

    // hitbox + position for collision
    pntr_rectangle pos = {
        req->x + maps->player->x + hitbox->x,
        req->y +  maps->player->y + hitbox->y,
        hitbox->width,
        hitbox->height
    };

    bool collision_static = false;;
    bool collision_objects = false;

    if (maps->layer_collisions != NULL){
        collision_static = adventure_check_static_collision(maps->map, maps->layer_collisions, &pos);
        if (collision_static && callback != NULL) {
            callback(app, maps, maps->player, NULL);
        }
    }

    if (maps->layer_objects != NULL){
        cute_tiled_object_t* subject = adventure_check_object_collision(maps->layer_objects, &pos, maps->player);
        if (subject != NULL) {
            collision_objects = true;
            if (callback != NULL) {
                callback(app, maps, maps->player, subject);
            }
        }
    }

    if (!collision_static && !collision_objects) {
        maps->player->x += req->x;
        maps->player->y += req->y;
    }
}

// load a single map into linked-list
// if it's already loaded, return that
adventure_map_t* adventure_load(char* filename, adventure_map_t** maps) {
    // search for same name
    adventure_map_t* found = (*maps);
    while(found != NULL) {
        if (PNTR_STRCMP(found->filename, filename) == 0) {
            // pntr_app_log_ex(PNTR_APP_LOG_DEBUG, "Adventure: found '%s' (preloaded.)", filename);
            break;
        }
        found = found->next;
    }

    if (found != NULL) {
        return found;
    }
    // pntr_app_log_ex(PNTR_APP_LOG_DEBUG, "Adventure: loading '%s' (not preloaded.)", filename);

    adventure_map_t* current = pntr_load_memory(sizeof(adventure_map_t));
    current->map = pntr_load_tiled(filename);
    current->filename = strdup(filename);

    cute_tiled_layer_t* layer = current->map->layers;
    while(layer != NULL) {
        if (PNTR_STRCMP("objects", layer->name.ptr) == 0) {
            current->layer_objects = layer;
            cute_tiled_object_t* obj = layer->objects;
            while(obj != NULL) {
                if (PNTR_STRCMP(obj->name.ptr, "player") == 0) {
                    current->player = obj;
                }
                obj  = obj->next;
            }
        }
        else if (PNTR_STRCMP("collisions", layer->name.ptr) == 0) {
            layer->visible = false;
            current->layer_collisions = layer;
        }
        layer = layer->next;
    }

    LL_PUSH(*maps, current);
    return current;
}

// free the current head of the list
void adventure_unload(adventure_map_t** map) {
    if (map && *map) {
        adventure_map_t* to_free = *map;
        cute_tiled_free_map((*map)->map);
        *map = (*map)->next;
        free(to_free);
    }
}

// set the current camera, based on screen/map size & lookAt position
void adventure_camera_look_at(pntr_vector* camera, pntr_image* screen, cute_tiled_map_t* map, cute_tiled_object_t* lookAt) {
    if (screen == NULL || map == NULL || lookAt == NULL ||  camera == NULL) {
        return;
    }
    camera->x = MAX(0, lookAt->x - screen->width / 2);
    camera->y = MAX(0, lookAt->y - screen->height / 2);
    camera->x = -1 * MIN(camera->x, (map->width * map->tilewidth) - screen->width);
    camera->y = -1 * MIN(camera->y , (map->height * map->tileheight) - screen->height);
}

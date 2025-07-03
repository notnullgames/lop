#include <dirent.h>

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

typedef struct adventure_map {
    char* name;
    cute_tiled_map_t* map;
    cute_tiled_object_t* player;
    cute_tiled_layer_t* layer_objects;
    cute_tiled_layer_t* layer_collisions;
    struct adventure_map* next;
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
    int tile_y1 = MIN((int)((rect->y + rect->height - 1)) / map->tileheight, layer->height - 1);
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
        if (obj->id != subject->id && RECTS_OVERLAP(rect->x, rect->y, rect->width, rect->height, obj->x, obj->y, obj->width, obj->height)) {
            return obj;
        }
    }
    return NULL;
}

// zelda-style input handler
void adventure_handle_input(pntr_app* app, adventure_map_t* maps, pntr_vector* req, pntr_rectangle* hitbox, AdventureCollisionCallback callback) {
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

adventure_map_t* adventure_load(char* filename) {
    adventure_map_t* maps = NULL;

    adventure_map_t* current = pntr_load_memory(sizeof(adventure_map_t));
    current->map = pntr_load_tiled(filename);

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

    LL_PUSH(maps, current);
    return maps;
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

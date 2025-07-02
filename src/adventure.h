#include <dirent.h>

// return least of 2 numbers
#ifndef MIN
#define MIN(a,b) ((a < b) ? a : b)
#endif

// return greatest of 2 numbers
#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif

// simple AABB collision for 2 rectangle-like things
#ifndef INTERSECT
#define INTERSECT(rect1, rect2) ((rect1)->x < (rect2)->x + (rect2)->width && (rect1)->x + (rect1)->width > (rect2)->x && (rect1)->y < (rect2)->y + (rect2)->height && (rect1)->y + (rect1)->height > (rect2)->y)
#endif

// pop from front of LL
#ifndef LL_POP
#define LL_POP(head, node) do { (node) = (head); if (head) (head) = (head)->next; } while(0)
#endif

// push to front of LL
#ifndef LL_PUSH
#define LL_PUSH(head, node) do { (node)->next = (head); (head) = (node); } while(0)
#endif

typedef enum {
    ADVENTURE_DIRECTION_NONE = 0,
    ADVENTURE_DIRECTION_SOUTH = 1,
    ADVENTURE_DIRECTION_NORTH = 2,
    ADVENTURE_DIRECTION_EAST = 3,
    ADVENTURE_DIRECTION_WEST = 4,
} AdventureDirection;

typedef struct adventure_maps {
    struct adventure_maps* next;
    char* name;
    cute_tiled_map_t* map;
    cute_tiled_object_t* player;
    cute_tiled_layer_t* objects;
} adventure_maps_t;

typedef void (*AdventureCollisionCallback)(pntr_app* app, adventure_maps_t* mapContainer, cute_tiled_object_t* object, cute_tiled_object_t* subject);

// find a map-container by name
adventure_maps_t* adventure_find_map(adventure_maps_t* maps, char* name) {
    adventure_maps_t* m = maps;
    while(m) {
        if (PNTR_STRCMP(name, m->name) == 0) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}

// set the current camera, based on screen/map size & lookAt position
void adventure_camera_look_at(pntr_image* screen, adventure_maps_t* mapContainer, cute_tiled_object_t* lookAt, pntr_vector* camera) {
    if (!lookAt || !mapContainer || !screen) {
        return;
    }
    camera->x = MAX(0, lookAt->x - screen->width / 2);
    camera->y = MAX(0, lookAt->y - screen->height / 2);
    camera->x = -1 * MIN(camera->x, (mapContainer->map->width * mapContainer->map->tilewidth) - screen->width);
    camera->y = -1 * MIN(camera->y , (mapContainer->map->height * mapContainer->map->tileheight) - screen->height);
}

// preload all maps in a dir
adventure_maps_t* adventure_load_all_maps(char* dirName) {
    struct dirent *dir;
    DIR *d = opendir(dirName);
    if (d) {
        adventure_maps_t* out = pntr_load_memory(sizeof(adventure_maps_t));
        while ((dir = readdir(d)) != NULL) {
            if (PNTR_STRSTR(dir->d_name, ".tmj") || PNTR_STRSTR(dir->d_name, ".TMJ")) {
                size_t len = PNTR_STRLEN(dir->d_name);
                char* dot = PNTR_STRCHR(dir->d_name, '.');
                if (dot) {
                    size_t basename_len = (size_t)(dot - dir->d_name);
                    char* basename = (char*)malloc(basename_len + 1);
                    if (!basename) continue; // allocation failed, skip
                    for (size_t i = 0; i < basename_len; i++) {
                        basename[i] = dir->d_name[i];
                    }
                    basename[basename_len] = '\0';
                    char fullPath[PNTR_PATH_MAX];
                    fullPath[0] = '\0';
                    PNTR_STRCAT(fullPath, dirName);
                    PNTR_STRCAT(fullPath, dir->d_name);
                    adventure_maps_t* current = pntr_load_memory(sizeof(adventure_maps_t));
                    current->map =  pntr_load_tiled(fullPath);
                    current->name = basename;
                    
                    // find objects layer
                    cute_tiled_layer_t* layer = current->map->layers;
                    while(layer) {
                        if (PNTR_STRCMP("objects", layer->name.ptr) == 0) {
                            current->objects = layer;
                            break; // nothing else needed
                        }
                        layer = layer->next;
                    }

                    // find player-object
                    if (current->objects) {
                        cute_tiled_object_t* obj = current->objects->objects;
                        while(obj) {
                            if (obj->name.ptr != NULL && PNTR_STRCMP(obj->name.ptr, "player") == 0) {
                                current->player = obj;
                                break; // nothing else needed
                            }
                            obj  = obj->next;
                        }
                    }

                    LL_PUSH(out, current);
                }
            }
        }
        closedir(d);
        return out;
    }
    return NULL;   
}



// process input, fire events and return player
void adventure_update_rpg(pntr_app* app, adventure_maps_t* mapContainer, AdventureCollisionCallback callback) {
    if (mapContainer->objects && mapContainer->player) {
        float speed = 200;
        float dt = pntr_app_delta_time(app);

        int px = mapContainer->player->x;
        int py = mapContainer->player->y;

        if (pntr_app_key_down(app, PNTR_APP_KEY_LEFT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_LEFT)) {
            mapContainer->player->x -= speed * dt;
        }
        else if (pntr_app_key_down(app, PNTR_APP_KEY_RIGHT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_RIGHT)) {
            mapContainer->player->x += speed * dt;
        }
        if (pntr_app_key_down(app, PNTR_APP_KEY_UP) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_UP)) {
            mapContainer->player->y -= speed * dt;
        }
        else if (pntr_app_key_down(app, PNTR_APP_KEY_DOWN) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_DOWN)) {
            mapContainer->player->y += speed * dt;
        }

        // check collision, call user's function
        // TODO: I think I could do all collisions (even non-mapContainer->player) like this, like loop every object, check 
        pntr_rectangle hitbox = {mapContainer->player->x + 4, mapContainer->player->y + 8, 8, 8 };
        cute_tiled_object_t* obj = mapContainer->objects->objects;
        while(obj) {
            if (obj->id != mapContainer->player->id && INTERSECT(&hitbox, obj)) {
                callback(app, mapContainer, mapContainer->player, obj);
                mapContainer->player->x = px;
                mapContainer->player->y = py;
            }
            obj = obj->next;
        }
    }
    
}


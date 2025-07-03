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
    AdventureDirection player_direction;
    float player_speed;
    bool player_walking;
    cute_tiled_layer_t* objects;
    cute_tiled_layer_t* collisions;
} adventure_maps_t;

// callbackks

// called when anythign touches wall or other object
typedef void (*AdventureCollisionCallback)(pntr_app* app, adventure_maps_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object);

// called on every object, on every frame
typedef void (*AdventureUpdateCallback)(pntr_app* app, adventure_maps_t* mapContainer, cute_tiled_object_t* object);

// called when the engine wants to draw an NPC walking/standing somewhere
typedef void (*AdventureSetTileCallback)(cute_tiled_object_t* object, AdventureDirection direction, bool walking);

// take a case-insensitive string with a directional (like from a prop) and turn it into an enum-directional
AdventureDirection adventure_direction_from_string(const char * s) {
    if (s[0] == 'S' || s[0] == 's' || s[0] == 'D' || s[0] == 'd') {
        return ADVENTURE_DIRECTION_SOUTH;
    }
    if (s[0] == 'N' || s[0] == 'n' || s[0] == 'U' || s[0] == 'u') {
        return ADVENTURE_DIRECTION_NORTH;
    }
    if (s[0] == 'E' || s[0] == 'e' || s[0] == 'R' || s[0] == 'r') {
        return ADVENTURE_DIRECTION_EAST;
    }
    if (s[0] == 'W' || s[0] == 'w' || s[0] == 'L' || s[0] == 'l') {
        return ADVENTURE_DIRECTION_WEST;
    }
    return ADVENTURE_DIRECTION_NONE;
}

// get text for a direction
char* adventure_direction_to_string(AdventureDirection direction) {
    if (direction == ADVENTURE_DIRECTION_SOUTH) {
        return "SOUTH";
    }
    else if (direction == ADVENTURE_DIRECTION_NORTH) {
        return "NORTH";
    }
    else if (direction == ADVENTURE_DIRECTION_EAST) {
        return "EAST";
    }
    else if (direction == ADVENTURE_DIRECTION_WEST) {
        return "WEST";
    }
    return "NONE";
}

// get the opposite direction from a direction
AdventureDirection adventure_get_opposite_direction(AdventureDirection direction) {
    if (direction == ADVENTURE_DIRECTION_SOUTH) {
        return ADVENTURE_DIRECTION_NORTH;
    }
    else if (direction == ADVENTURE_DIRECTION_NORTH) {
        return ADVENTURE_DIRECTION_SOUTH;
    }
    else if (direction == ADVENTURE_DIRECTION_EAST) {
        return ADVENTURE_DIRECTION_WEST;
    }
    else if (direction == ADVENTURE_DIRECTION_WEST) {
        return ADVENTURE_DIRECTION_EAST;
    }
    return ADVENTURE_DIRECTION_NONE;
}

// get direction to make an object face another object
AdventureDirection adventure_get_direction(cute_tiled_object_t* object, cute_tiled_object_t* subject) {
    int dx = subject->x - object->x;
    int dy = subject->y - object->y;
    if (dx == 0 && dy == 0) {
        return ADVENTURE_DIRECTION_NONE;
    }
    if (ABS(dx) > ABS(dy)) {
        return dx > 0 ? ADVENTURE_DIRECTION_EAST : ADVENTURE_DIRECTION_WEST;
    } else {
        return dy > 0 ? ADVENTURE_DIRECTION_SOUTH : ADVENTURE_DIRECTION_NORTH;
    }
}


// find a map-container by name
// it's recommend to do your own loop, but this can be easier for 1-off
adventure_maps_t* adventure_find_map(adventure_maps_t* maps, const char* name) {
    // printf("Looking for map: '%s'\n", name);
    if (maps == NULL || maps->name == NULL) {
        return NULL;
    }
    adventure_maps_t* m = maps;
    while (m) {
        // printf("Map in list: '%s'\n", m->name ? m->name : "(null)");
        if (PNTR_STRCMP(name, m->name) == 0) {
             return m;
        }
        m = m->next;
    }
    return NULL;
}

// set the current camera, based on screen/map size & lookAt position
void adventure_camera_look_at(pntr_image* screen, adventure_maps_t* mapContainer, cute_tiled_object_t* lookAt, pntr_vector* camera) {
    if (lookAt == NULL || mapContainer == NULL || screen == NULL || camera == NULL) {
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
        adventure_maps_t* out = NULL;
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
                    current->next = NULL;

                    current->map =  pntr_load_tiled(fullPath);
                    current->name = basename;
                    
                    // find objects layer
                    cute_tiled_layer_t* layer = current->map->layers;
                    while(layer) {
                        if (PNTR_STRCMP("objects", layer->name.ptr) == 0) {
                            current->objects = layer;
                        }
                        if (PNTR_STRCMP("collisions", layer->name.ptr) == 0) {
                            current->collisions = layer;
                            current->collisions->visible = false;
                        }
                        layer = layer->next;
                    }

                    // find player-object
                    if (current->objects) {
                        cute_tiled_object_t* obj = current->objects->objects;
                        while(obj) {
                            if (obj->name.ptr != NULL && PNTR_STRCMP(obj->name.ptr, "player") == 0) {
                                current->player = obj;
                                // TODO: get this from animation or map/player prop  (or maybe even collided tiles for concept of terrain-changes)
                                current->player_speed = 200;
                                current->player_direction = ADVENTURE_DIRECTION_SOUTH;
                                
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


// move an object towards an object
static void adventure_move_towards(adventure_maps_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object, AdventureSetTileCallback map_walk) {
    map_walk(object, adventure_get_direction(object, subject), true);
}

// move an object away from an object
static void adventure_move_away(adventure_maps_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object, AdventureSetTileCallback map_walk) {
    map_walk(object, adventure_get_opposite_direction(adventure_get_direction(object, subject)), true);
}


// process input, fire events
void adventure_update(pntr_app* app, adventure_maps_t* mapContainer, AdventureCollisionCallback handleCollision, AdventureUpdateCallback handleUpdate) {
    if (mapContainer->objects && mapContainer->player) {

        // check collision, call user's function
        pntr_rectangle hitbox = {mapContainer->player->x + 4, mapContainer->player->y + 8, 8, 8 };
        
        cute_tiled_object_t* obj = mapContainer->objects->objects;
        while(obj) {
            if (obj->visible && obj->id != mapContainer->player->id && INTERSECT(&hitbox, obj)) {
                handleCollision(app, mapContainer, mapContainer->player, obj);
            }
            handleUpdate(app, mapContainer, obj);
            obj = obj->next;
        }

        // I was having issues with object collisions (especially vector-shapes) so I just used tiles
        // TODO: this seems broke
        // for (int y = 0; y <  mapContainer->collisions->height; ++y) {
        //     for (int x = 0; x <  mapContainer->collisions->width; ++x) {
        //         int index = y *  mapContainer->collisions->width + x;
        //         int gid =  mapContainer->collisions->data[index];
        //         if (gid) {
        //             pntr_rectangle check = {
        //                 x * mapContainer->map->tilewidth,
        //                 y * mapContainer->map->tileheight,
        //                 mapContainer->map->width * mapContainer->map->tilewidth,
        //                 mapContainer->map->height * mapContainer->map->tileheight
        //             };
        //             if (INTERSECT(&hitbox, &check)) {
        //                 handleCollision(app, mapContainer, mapContainer->player, NULL);
        //             }
        //         }
        //     }
        // }
    }
}


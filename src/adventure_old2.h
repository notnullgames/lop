// Generic adventure-engine for pntr_tiled

#include <dirent.h>

// total maps that can be laoded
#define ADVENTURE_MAX_MAPS 32 

// max enemies that can be loaded, per map
#define ADVENTURE_MAX_ENEMIES 32

// max objects that can be loaded, per map
#define ADVENTURE_MAX_OBJECTS 32

#ifndef MIN
#define MIN(a,b) ((a < b) ? a : b)
#endif
#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif
#ifndef INTERSECT
#define INTERSECT(rect1, rect2) (rect1.x < rect2.x + rect2.width && rect1.x + rect1.width > rect2.x && rect1.y < rect2.y + rect2.height && rect1.y + rect1.height > rect2.y)
#endif

typedef enum {
    ADVENTURE_DIRECTION_NONE = 0,
    ADVENTURE_DIRECTION_SOUTH = 1,
    ADVENTURE_DIRECTION_NORTH = 2,
    ADVENTURE_DIRECTION_EAST = 3,
    ADVENTURE_DIRECTION_WEST = 4,
} AdventureDirection;

typedef struct {
    cute_tiled_object_t* object;
    AdventureDirection direction;
    bool walking;
    int speed;
    bool followsPlayer;
    bool avoidsPlayer;
    pntr_rectangle hitbox;
    int gidWalking[4];
    int gidStill[4];
} AdventureCharacter;

typedef struct {
    char* name;
    cute_tiled_map_t* map;
    cute_tiled_layer_t* objects;
    cute_tiled_layer_t* collisions;
    
    AdventureCharacter player;
    
    AdventureCharacter enemies[ADVENTURE_MAX_ENEMIES];
    int enemiesCount;

    cute_tiled_object_t* things[ADVENTURE_MAX_OBJECTS];
    int thingsCount;
} AdventureMapInfo;

typedef void (*AdventureCollisionCallback)(AdventureMapInfo* info, AdventureCharacter* moving, AdventureCharacter* touching);

static int mapCount = 0;
static AdventureMapInfo maps[ADVENTURE_MAX_MAPS] = {0};

// find a map in an array by name
cute_tiled_map_t* adventure_find_map(const char* name) {
    for (int i = 0; i < mapCount; i++) {
        if (PNTR_STRSTR(maps[i].name, name)) {
            return maps[i].map;
        }
    }
    return NULL;
}

// find a map-info in an array by name
AdventureMapInfo* adventure_find_map_info(const char* name) {
    for (int i = 0; i < mapCount; i++) {
        if (PNTR_STRSTR(maps[i].name, name)) {
            return &maps[i];
        }
    }
    return NULL;
}

// Get the direction of a character from map, based on the assumption that each character is a row, and each row has 12 tiles, with 3 tiles for each direction
static AdventureDirection _adventure_get_tile_direction(cute_tiled_object_t* object) {
    if (!object->gid) {
        return ADVENTURE_DIRECTION_NONE;
    }
    return (AdventureDirection) ((object->gid % 12) / 4) + 1;
}

// load a player (or NPC) 
static AdventureCharacter _adventure_load_character(cute_tiled_object_t* object) {
    AdventureCharacter out = {
        .object = object,
        .direction = _adventure_get_tile_direction(object),
        .speed = 200, // TODO: get this from animation
        .hitbox = (pntr_rectangle) {4,8,8,8} // TODO: get this from map
    };

    // use int-rouding to get first tile, then build other tiles
    int startGid = ((object->gid/12) * 12) + 1;
    for (int i=0;i<4;i++) {
        out.gidStill[i] = startGid + (i*3);
        out.gidWalking[i] = out.gidStill[i] + 1;
    }
    
    // get followsPlayer/avoidsPlayer from object-properties
    for (int i = 0; i < object->property_count; i++) {
        cute_tiled_property_t* prop = &object->properties[i];
        if (prop->name.ptr != NULL) {
            if (prop->type == CUTE_TILED_PROPERTY_BOOL) {
                if (PNTR_STRCMP(prop->name.ptr, "follows") == 0) {
                    out.followsPlayer = prop->data.boolean;
                }
                if (PNTR_STRCMP(prop->name.ptr, "avoids") == 0) {
                    out.avoidsPlayer = prop->data.boolean;
                }
            }
        }
    }
        

    return out;
}


// sets up some initial things for a map
static AdventureMapInfo _adventure_setup_map(char* basename, char* fullPath) {
    AdventureMapInfo info = {
        .name = basename,
        .map = pntr_load_tiled(fullPath)
    };

    if (info.map == NULL) {
        return info;
    }

    cute_tiled_layer_t* layer = info.map->layers;
    while (layer) {
        if (PNTR_STRCMP(layer->name.ptr, "objects") == 0) {
            info.objects = layer;
            cute_tiled_object_t* current = layer->objects;
            while (current != NULL) {
                if (current->name.ptr != NULL && PNTR_STRCMP(current->name.ptr, "player") == 0) {
                    info.player = _adventure_load_character(current);
                }
                else if (current->type.ptr != NULL && PNTR_STRCMP(current->type.ptr, "enemy") == 0) {
                    info.enemies[info.enemiesCount] = _adventure_load_character(current);
                    info.enemiesCount++;
                }
                else {
                    info.things[info.thingsCount] = current;
                    info.thingsCount++;
                }
                current = current->next;
            }
        }
        else if (PNTR_STRCMP(layer->name.ptr, "collisions") == 0) {
            info.collisions = layer;
        }
        layer = layer->next;
    }

    return info;
}

// set the current camera, based on screen/map size & player position
void adventure_set_camera(pntr_image* screen, AdventureMapInfo* info, pntr_vector* camera) {
    camera->x = MAX(0, info->player.object->x - screen->width / 2);
    camera->y = MAX(0, info->player.object->y - screen->height / 2);
    camera->x = -1 * MIN(camera->x, (info->map->width * info->map->tilewidth) - screen->width);
    camera->y = -1 * MIN(camera->y , (info->map->height * info->map->tileheight) - screen->height);
}

// preload all maps in a dir
int adventure_load_all_maps(char* dirName) {
    mapCount = 0;
    struct dirent *dir;
    DIR *d = opendir(dirName);
    if (d) {
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
                    if (mapCount < ADVENTURE_MAX_MAPS) {
                        char fullPath[PNTR_PATH_MAX];
                        fullPath[0] = '\0';
                        PNTR_STRCAT(fullPath, dirName);
                        PNTR_STRCAT(fullPath, dir->d_name);
                        maps[mapCount++] = _adventure_setup_map(basename, fullPath);
                    } else {
                        free(basename);
                    }
                }
            }
        }
        closedir(d);
        return mapCount;
    }
    return 0;
}

// switch to a differnt preloaded-map
void adventure_portal(char* name, cute_tiled_object_t* portal) {

}


// try to move a character in current direction
void adventure_move_character_rpg(AdventureMapInfo* info, AdventureCharacter character, float dt , AdventureCollisionCallback callback) {
    // update sprite/animation for current state
    character.object->gid = character.walking ? character.gidWalking[character.direction-1] : character.gidStill[character.direction-1];

    if (character.direction == ADVENTURE_DIRECTION_NONE || !character.walking) {
        return;
    }

    pntr_rectangle wants = {
        character.object->x + character.hitbox.x,
        character.object->y + character.hitbox.y,
        character.hitbox.width,
        character.hitbox.height
    };

    if (character.direction == ADVENTURE_DIRECTION_SOUTH) {
        wants.y += character.speed * dt;
    }
    if (character.direction == ADVENTURE_DIRECTION_NORTH) {
        wants.y -= character.speed * dt;
    }
    if (character.direction == ADVENTURE_DIRECTION_EAST) {
        wants.x += character.speed * dt;
    }
    if (character.direction == ADVENTURE_DIRECTION_WEST) {
        wants.x -= character.speed * dt;
    }

    // check collision with character hitbox

}

// update game-state for a zelda-like game (but will work for lots of styles)
void adventure_update_rpg(pntr_app* app, AdventureMapInfo* info, AdventureCollisionCallback callback) {
    float dt = pntr_app_delta_time(app);

    // Keyboard/Gamepad
    info->player.walking = false;
    
    if (pntr_app_key_down(app, PNTR_APP_KEY_LEFT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_LEFT)) {
        info->player.walking=true;
        info->player.direction = ADVENTURE_DIRECTION_WEST;
        adventure_move_character_rpg(info, info->player, dt, callback);
    }
    else if (pntr_app_key_down(app, PNTR_APP_KEY_RIGHT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_RIGHT)) {
        info->player.walking=true;
        info->player.direction = ADVENTURE_DIRECTION_EAST;
        adventure_move_character_rpg(info, info->player, dt, callback);
    }
    if (pntr_app_key_down(app, PNTR_APP_KEY_UP) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_UP)) {
        info->player.walking=true;
        info->player.direction = ADVENTURE_DIRECTION_NORTH;
        adventure_move_character_rpg(info, info->player, dt, callback);
    }
    else if (pntr_app_key_down(app, PNTR_APP_KEY_DOWN) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_DOWN)) {
        info->player.walking=true;
        info->player.direction = ADVENTURE_DIRECTION_SOUTH;
        adventure_move_character_rpg(info, info->player, dt, callback);
    }

    for (int i=0;i<info->enemiesCount;i++) {
        info->enemies[i].direction = pntr_app_random(app, 1, 4);
        info->enemies[i].walking = pntr_app_random(app, 1, 8) < 2;
        if (info->enemies[i].walking) {
            adventure_move_character_rpg(info, info->enemies[i], dt, callback);
        }
    }

}


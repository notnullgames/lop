#define PNTR_APP_IMPLEMENTATION
#define PNTR_ENABLE_VARGS
#define PNTR_ENABLE_DEFAULT_FONT
#include "pntr_app.h"
#define PNTR_TILED_IMPLEMENTATION
#include "pntr_tiled.h"

#include "adventure.h"

// default font for dialogs
static pntr_font* font;

// show the title-screen, initially
static bool showTitle = true;

// set first char to '\0' to disable, trigggers dialog
static char signText[500] = {0};

// main title map
static adventure_maps_t* titleMap = NULL;

// a map that shows a dialog (rendered over playMap)
static adventure_maps_t* dialogMap = NULL;

// the current map for the actual game
static adventure_maps_t* playMap = NULL;

// the current foreground-map
static adventure_maps_t* currentMap = NULL;

// all maps
static adventure_maps_t* maps;

// player's monies
static int gemsCount = 0;

typedef struct sound_holder_t {
    struct sound_holder_t* next;
    char* name;
} sound_holder_t;


// using gid, get the current direction (each row has 12, 1 character per row, 3 tiles per direction)
AdventureDirection map_get_current_direction(cute_tiled_object_t* object) {
    if (object == NULL) {
        return ADVENTURE_DIRECTION_NONE;
    }
    return (AdventureDirection) ((object->gid % 12) % 4) + 1;
}

// set object to correct sprite for the direction
static void map_walk(cute_tiled_object_t* object, AdventureDirection direction, bool walking) {
    if (object == NULL) {
        return;
    }
    // figure out which row is the current character. this is very specifc to my tileset layout
    int character = (object->gid-1) / 12;
    object->gid = (character*12) + ((int) (direction-1) * 3) + 1;
    if (walking) {
        object->gid++;
    }
}

// move the object in the opposite direction they are facing, facing the same way
static void map_bump_back(pntr_app* app, cute_tiled_object_t* object) {
    AdventureDirection direction = map_get_current_direction(object);
    float dt = pntr_app_delta_time(app);

    // TODO: I am just using player-speed, but should figure out speed for every object
    float speed = 200;
    if (currentMap != NULL) {
        speed=currentMap->player_speed;
    }
    
    if (direction == ADVENTURE_DIRECTION_NONE) {
        return;
    }
    else if (direction == ADVENTURE_DIRECTION_SOUTH) {
       object->y += speed * dt;
    }
    else if (direction == ADVENTURE_DIRECTION_NORTH) {
        object->y -= speed * dt;
    }
    else if (direction == ADVENTURE_DIRECTION_EAST) {
        object->x += speed * dt;
    }
    else if (direction == ADVENTURE_DIRECTION_WEST) {
        object->x -= speed * dt;
    }
    map_walk(object, direction, false);
}



// called by adventure engine when player or object touches another object or a wall
void HandleCollision(pntr_app* app, adventure_maps_t* mapContainer, cute_tiled_object_t* subject, cute_tiled_object_t* object) {
    // wall
    if (object == NULL) {
        map_bump_back(app, subject);
    }
    else if (subject->id == mapContainer->player->id) {
        printf("player touched '%s' (%llu - %s : %llu)\n", object->name.ptr, object->name.hash_id, object->type.ptr, object->type.hash_id);

        int roopiesAmount = 1;
        int damage = 1;
        pntr_vector pos = {0};
        AdventureDirection direction = ADVENTURE_DIRECTION_NONE;

        for (int i = 0; i < object->property_count; i++) {
            cute_tiled_property_t* prop = &object->properties[i];
            
            if (prop->type == CUTE_TILED_PROPERTY_STRING) {
                // if it has "text" prop, just set signText
                if (PNTR_STRCMP("text", prop->name.ptr) == 0) {
                    signText[0] = '\0';
                    PNTR_STRCAT(signText, prop->data.string.ptr);
                }
                else if (PNTR_STRCMP("facing", prop->name.ptr) == 0) {
                    direction = adventure_direction_from_string(prop->data.string.ptr);
                }
            }
            else if (prop->type == CUTE_TILED_PROPERTY_INT) {
                if (PNTR_STRCMP("pos_x", prop->name.ptr) == 0) {
                    pos.x = prop->data.integer;
                }
                else if (PNTR_STRCMP("pos_y", prop->name.ptr) == 0) {
                    pos.y = prop->data.integer;
                }
                else if (PNTR_STRCMP("roopees", prop->name.ptr) == 0) {
                    roopiesAmount = prop->data.integer;
                }
                else if (PNTR_STRCMP("damage", prop->name.ptr) == 0) {
                    damage = prop->data.integer;
                }
            }
        }

        if (PNTR_STRCMP("portal", object->type.ptr) == 0) {
            if (direction != ADVENTURE_DIRECTION_NONE || (pos.x != 0 && pos.y != 0)) {
                currentMap = adventure_find_map(maps, object->name.ptr);
                playMap = currentMap;
                if (currentMap != NULL && currentMap->player != NULL) {
                    if (pos.x != 0 && pos.y != 0) {
                        currentMap->player->x = pos.x;
                        currentMap->player->y = pos.y;
                    }
                    if (direction != ADVENTURE_DIRECTION_NONE) {
                        currentMap->player_direction = direction;
                    }
                }
            }
        }
        else if (PNTR_STRCMP("loot", object->type.ptr) == 0) {
            gemsCount += roopiesAmount;
            object->visible = false;
        }
        else if (PNTR_STRCMP("trap", object->type.ptr) == 0) {
            gemsCount -= damage;
            map_bump_back(app, subject);;
        }
        else if (PNTR_STRCMP("enemy", object->type.ptr) == 0) {
            gemsCount -= damage;
            map_bump_back(app, subject);
        } else {
            map_bump_back(app, subject);
        }
    }
}

// called by adventure-engine when any object needs an update
void HandleUpdate(pntr_app* app, adventure_maps_t* mapContainer, cute_tiled_object_t* object) {
    bool follow = false;
    bool avoid = false;
    bool millabout = false;

    for (int i = 0; i < object->property_count; i++) {
        cute_tiled_property_t* prop = &object->properties[i];
        
        if (prop->type == CUTE_TILED_PROPERTY_BOOL) {
            if (PNTR_STRCMP("follow", prop->name.ptr) == 0) {
                follow = prop->data.boolean;
            }
            else if (PNTR_STRCMP("avoid", prop->name.ptr) == 0) {
                avoid = prop->data.boolean;
            }
            else if (PNTR_STRCMP("millabout", prop->name.ptr) == 0) {
                millabout = prop->data.boolean;
            }
        }

    }
    
    if (follow) {
        adventure_move_towards(mapContainer, object, mapContainer->player, map_walk);
    }
    else if (avoid) {
        adventure_move_away(mapContainer, object, mapContainer->player, map_walk);
    }
    else if (millabout) {
       // TODO
    }
    else {
        map_walk(mapContainer->player, mapContainer->player_direction, mapContainer->player_walking);
    }

    if (object->id == mapContainer->player->id) {
        float dt = pntr_app_delta_time(app);

        int px = mapContainer->player->x;
        int py = mapContainer->player->y;
        mapContainer->player_walking = false;

        if (pntr_app_key_down(app, PNTR_APP_KEY_LEFT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_LEFT)) {
            mapContainer->player->x -= mapContainer->player_speed * dt;
            mapContainer->player_walking = true;
            mapContainer->player_direction = ADVENTURE_DIRECTION_WEST;
        }
        else if (pntr_app_key_down(app, PNTR_APP_KEY_RIGHT) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_RIGHT)) {
            mapContainer->player->x += mapContainer->player_speed * dt;
            mapContainer->player_walking = true;
            mapContainer->player_direction = ADVENTURE_DIRECTION_EAST;
        }
        if (pntr_app_key_down(app, PNTR_APP_KEY_UP) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_UP)) {
            mapContainer->player->y -= mapContainer->player_speed * dt;
            mapContainer->player_walking = true;
            mapContainer->player_direction = ADVENTURE_DIRECTION_NORTH;
        }
        else if (pntr_app_key_down(app, PNTR_APP_KEY_DOWN) || pntr_app_gamepad_button_down(app, 0, PNTR_APP_GAMEPAD_BUTTON_DOWN)) {
            mapContainer->player->y += mapContainer->player_speed * dt;
            mapContainer->player_walking = true;
            mapContainer->player_direction = ADVENTURE_DIRECTION_SOUTH;
        }

        // update player-animation
        // TODO: I don't really need to track direction/speed/walking in mapContainer
        map_walk(mapContainer->player, mapContainer->player_direction, mapContainer->player_walking);
    }
}

bool Init(pntr_app* app) {
    font = pntr_load_font_default();
    maps = adventure_load_all_maps("assets/");
    if (!maps) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "No maps found!");
        return false;
    }

    adventure_maps_t* m = maps;
    // not sure why I need to check name, too
    while(m && m->name != NULL) {
        if (PNTR_STRCMP("dialog", m->name) == 0) {
            dialogMap = m;
        }
        if (PNTR_STRCMP("title", m->name) == 0) {
            titleMap = m;
        }
        if (PNTR_STRCMP("main", m->name) == 0) {
            currentMap = m;
        }
        m = m->next;
    }

    if (currentMap == NULL) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Make a map called main.");
        return false;
    }

    if (dialogMap == NULL) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Make a map called dialog.");
        return false;
    }
    if (titleMap == NULL) {
        pntr_app_log(PNTR_APP_LOG_ERROR, "Make a map called title.");
        return false;
    }

    playMap = currentMap;

    return true;
}

void Event(pntr_app* app, pntr_app_event* event) {
    // not used, but seems needed for web
}

bool Update(pntr_app* app, pntr_image* screen) {
     pntr_vector camera = {0};

    // choose map
    if (showTitle) {
        currentMap = titleMap;
    }
    else if (signText[0] != 0) {
        currentMap = dialogMap;
    }
    else {
        if (playMap!= NULL && playMap->player != NULL) {
            currentMap = playMap;
            adventure_update(app, currentMap, HandleCollision, HandleUpdate);
            adventure_camera_look_at(screen, currentMap, currentMap->player, &camera);
        }
    }

    // draw map
    if (currentMap != NULL) {
        pntr_update_tiled(currentMap->map,  pntr_app_delta_time(app));
        if (!signText[0]){
            pntr_clear_background(screen, currentMap->map->backgroundcolor ? pntr_tiled_color(currentMap->map->backgroundcolor) : PNTR_BLACK);
        }
        pntr_draw_tiled(screen, currentMap->map, camera.x, camera.y, PNTR_WHITE);
    } else {
        pntr_app_log(PNTR_APP_LOG_ERROR, "No map!");
        return false;
    }

    // draw over map
    if (showTitle) {
        pntr_draw_text(screen, font, "The Legend of Pntr", 100, 100, PNTR_RAYWHITE);
        if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
            showTitle = false;
        }
    }
    else if (signText[0] != 0) {
        pntr_draw_text_wrapped(screen, font, signText, 20, 180, 280, PNTR_RAYWHITE);
        if (pntr_app_key_down(app, PNTR_APP_KEY_SPACE)) {
            signText[0] = 0;
            if (currentMap->player!= NULL) {
                map_bump_back(app, currentMap->player);
            }
        }
    }
    else {
        pntr_draw_text_ex(screen, font, 10, 10, PNTR_RAYWHITE, "GEMS: %d", gemsCount);
    }

    return true;
}

void Close(pntr_app* app) {
    // TODO: cleanup
}

pntr_app Main(int argc, char* argv[]) {
#ifdef PNTR_APP_RAYLIB
    SetTraceLogLevel(LOG_ERROR);
#endif

    return (pntr_app) {
        .width = 320,
        .height = 240,
        .title = "legend of pntr",
        .init = Init,
        .update = Update,
        .close = Close,
        .event = Event,
        .fps = 60
    };
}
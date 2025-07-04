// this is a linked-list for animation-timeouts (pntr_tiled)
// you can schedule a change to a gid for later

// push to front of LL
#ifndef LL_PUSH
#define LL_PUSH(head, node) do { (node)->next = (head); (head) = (node); } while(0)
#endif

typedef struct animation_queue_t {
    struct animation_queue_t* next;
    cute_tiled_object_t* object;
    int gid;
    float time; // in seconds
    pntr_vector* position;
} animation_queue_t;

static float queue_time = 0;

// add an animation to queue
void animation_queue_add(animation_queue_t** animations, cute_tiled_object_t* object, int gid, float time, pntr_vector* position) {
    animation_queue_t* current = pntr_load_memory(sizeof(animation_queue_t));
    current->object = object;
    current->gid = gid;
    current->time = queue_time + time;
    current->position = position;
    LL_PUSH(*animations, current);
}


// run any pending animations (called every frame)
void animation_queue_run(animation_queue_t** animations, float dt) {
    queue_time += dt;
    animation_queue_t* current = *animations;
    animation_queue_t* prev = NULL;
    while (current != NULL) {
        if (queue_time >= current->time) {
            if (current->gid != 0) {
                current->object->gid = current->gid;
            }
            if (current->position != NULL) {
                current->object->x = current->position->x;
                current->object->y = current->position->y;
            }
            animation_queue_t* to_delete = current;
            if (prev == NULL) {
                *animations = current->next;
                current = *animations;
            } else {
                prev->next = current->next;
                current = prev->next;
            }
            free(to_delete);
        } else {
            prev = current;
            current = current->next;
        }
    }
}

// free the current head of the list
void animation_queue_unload(animation_queue_t** animations) {
    if (animations && *animations) {
        animation_queue_t* to_free = *animations;
        *animations = (*animations)->next;
        pntr_unload_memory(to_free);
    }
}

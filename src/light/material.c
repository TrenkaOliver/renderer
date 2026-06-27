#include <string.h>

#include "scene/scene.h"

size_t get_material_id(char *s, DynArray *arr) {
    size_t i;
    MaterialEntry *entry;

    for (i = 0; i < arr->count; i++) {
        entry = get_element(i, arr);
        if (strcmp(entry->name, s) == 0) {
            return entry->id;
        }
    }

    return (size_t)-1;
}

size_t add_material(char *s, DynArray *arr, Scene *scene) {
    size_t id, i;
    MaterialEntry *entry;

    id = get_material_id(s, arr);

    if (id != (size_t)-1) return id;

    i = grow_dyn_array(arr);
    id = grow_dyn_array(&scene->materials);
    
    entry = get_element(i, arr);
    entry->id = id;
    strcpy(entry->name, s);

    return id;
}
#include <stdlib.h>

typedef struct DynArray {
    void *ptr;
    size_t size;
    size_t count;
    size_t capacity;
} DynArray;

DynArray create_dyn_array(size_t size, size_t capacity);
size_t grow_dyn_array(DynArray *arr);
void *get_element(size_t i, DynArray *arr);

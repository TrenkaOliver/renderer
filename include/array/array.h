#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

typedef struct DynArray {
    void *ptr;
    size_t size;
    size_t count;
    size_t capacity;
} DynArray;

static inline DynArray create_dyn_array(const size_t size, const size_t capacity) {
    DynArray arr;

    arr.ptr = calloc(capacity, size);
    arr.size = size;
    arr.count = 0;
    arr.capacity = capacity;

    return arr;
}

static inline size_t grow_dyn_array(DynArray *arr) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        arr->ptr = realloc(arr->ptr, arr->capacity * arr->size);
    }

    return arr->count++;
}

static inline size_t grow_n_dyn_array(DynArray *arr, const size_t n) {
    const size_t next = arr->count;
    arr->count += n;
        if (arr->count > arr->capacity) {
        arr->capacity += arr->count - arr->capacity;
        arr->ptr = realloc(arr->ptr, arr->capacity * arr->size);
    }

    return next;
}

static inline void *get_element(const size_t i, const DynArray *arr) {
    return (char *)arr->ptr + i * arr->size;
}

static inline void delete_array(DynArray *arr, const int free_elemets) {
    if (free_elemets) {
        for (size_t i = 0; i < arr->count; i++) {
            free(*(void **)get_element(i, arr));
        }
    }

    free(arr->ptr);
}


#endif
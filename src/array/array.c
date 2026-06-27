#include <stdlib.h>
#include <stdio.h>
#include "array/array.h"

DynArray create_dyn_array(size_t size, size_t capacity) {
    DynArray arr;

    arr.ptr = calloc(capacity, size);
    arr.size = size;
    arr.count = 0;
    arr.capacity = capacity;

    return arr;
}

size_t grow_dyn_array(DynArray *arr) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        arr->ptr = realloc(arr->ptr, arr->capacity * arr->size);
    }

    return arr->count++;
}

void *get_element(size_t i, DynArray *arr) {
    return (char *)arr->ptr + i * arr->size;
}
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

size_t grow_n_dyn_array(DynArray *arr, size_t n) {
    size_t next = arr->count;
    arr->count += n;
        if (arr->count > arr->capacity) {
        arr->capacity += arr->count - arr->capacity;
        arr->ptr = realloc(arr->ptr, arr->capacity * arr->size);
    }

    return next;
}

void *get_element(size_t i, DynArray *arr) {
    return (char *)arr->ptr + i * arr->size;
}
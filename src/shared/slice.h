#pragma once

#include "stddef.h"

typedef struct Slice {
    size_t len;
    void *ptr;
} Slice;

#define MakeStackSlice(name, type, alen)\
    typedef struct name{\
        size_t len;\
        type ptr[alen];\
    } name;\

Slice slice_from_str(const char *value);
Slice slice_from_arr(void *ptr, size_t len);

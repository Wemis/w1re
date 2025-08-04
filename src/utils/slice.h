#pragma once

#include "stddef.h"

typedef struct Slice {
    size_t len;
    const void *ptr;
} Slice;

Slice slice_from_str(const char *value);
Slice slice_from_arr(void *ptr, size_t len);

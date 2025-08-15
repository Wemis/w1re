#include "slice.h"
#include <string.h>

Slice slice_from_str(const char *value) {
    return (Slice){.len = strlen(value), .ptr = value};
}

Slice slice_from_arr(void *ptr, size_t len) {
    return (Slice){.len = len, .ptr = ptr};
}

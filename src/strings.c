#include "stddef.h"
#include "string.h"

typedef struct Slice {
    size_t len;
    void* ptr;
} Slice;

inline Slice slice_from_str(char* value) {
    return (Slice){.len=strlen(value), .ptr=value};
}

inline Slice slice_from_arr(void* ptr, size_t len) {
    return (Slice){.len=len, .ptr=ptr};
}

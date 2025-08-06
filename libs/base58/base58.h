#ifndef BASE58_H
#define BASE58_H
#include <stdint.h>
#include <string.h>

char* base58encode(const uint8_t* input, size_t len);
int base58decode(const char* input, uint8_t output[10]);

#endif
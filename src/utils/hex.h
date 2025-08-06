#ifndef HEX_H
#define HEX_H

#include <stdint.h>
#include <stdio.h>

size_t hex_to_bytes(const char* hexStr, uint8_t* outBytes, size_t maxLen);
void print_hex(uint8_t* hex, int size);

#endif
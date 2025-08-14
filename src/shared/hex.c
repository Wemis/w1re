#include <stdio.h>
#include <string.h>
#include <stdint.h>


uint8_t hexCharToValue(char c) {
    if ('0' <= c && c <= '9') return c - '0' ;
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return 0xFF;
}

size_t hex_to_bytes(const char* hexStr, uint8_t* outBytes, size_t maxLen) {
    size_t len = strlen(hexStr);
    if (len % 2 != 0) {
        return -1;
    }

    size_t byteCount = len / 2;
    if (byteCount > maxLen) {
        return -1;
    }

    for (size_t i = 0; i < byteCount; ++i) {
        uint8_t high = hexCharToValue(hexStr[2 * i]);
        uint8_t low  = hexCharToValue(hexStr[2 * i + 1]);

        if (high == 0xFF || low == 0xFF) {
            return -1;
        }

        outBytes[i] = (high << 4) | low;
    }

    return byteCount;
}


void print_hex(uint8_t* hex, int size) {
    for (int i = 0; i < size; i++) {
        printf("%02x", hex[i]);
    }
    printf("\n");
}
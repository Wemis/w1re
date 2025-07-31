#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>

typedef struct {
    char name[32];
    uint8_t privkey[32];
    uint8_t sign_pubkey[64];
    uint8_t encr_pubkey[32];
    uint8_t signature[64];
} User;

#endif
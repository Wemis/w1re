#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5330

typedef struct {
    char name[32];
    char id[27];
    uint8_t privkey[32];
    uint8_t pubkey_sign[32];
    uint8_t pubkey_encr[32];
    uint8_t signature[64];
} User;

typedef struct {
    char* from;
    char to[32];
    char text[1024];
    int size;
} Message;

#endif
#ifndef COMMON_H
#define COMMON_H
#include <stddef.h>
#include <stdint.h>
#include <sodium/crypto_box.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5330

typedef struct {
    uint8_t name[64];
    uint8_t id[32]; // 16 + 1 + 14 + \0
    uint8_t privkey[32];
    uint8_t pubkey_sign[32];
    uint8_t pubkey_encr[32];
    uint8_t signature[64];
} User;

typedef struct {
    uint8_t from[32];
    uint8_t to[32];
    uint8_t nonce[crypto_box_NONCEBYTES];
    uint8_t sender_pubkey[crypto_box_PUBLICKEYBYTES];
    uint8_t* ciphertext;
    size_t size;
} Message;

#endif
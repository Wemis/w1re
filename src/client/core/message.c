#include "../../../include/common.h"
#include <sodium/crypto_box.h>
#include <sodium/randombytes.h>
#include <sodium.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>


Message build_msg(char * text, const uint8_t from[32], const uint8_t to[32],
    uint8_t rc_pubkey[crypto_box_PUBLICKEYBYTES],
    uint8_t sender_pubkey[crypto_box_PUBLICKEYBYTES],
    uint8_t seed[crypto_box_SECRETKEYBYTES]) {

    Message msg = {0};
    uint8_t privkey[crypto_box_SECRETKEYBYTES];
    uint8_t temp[32];

    randombytes_buf(msg.nonce, sizeof msg.nonce);
    crypto_box_seed_keypair(temp, privkey, seed);

    msg.message.ptr = malloc(strlen(text) + crypto_box_MACBYTES);
    msg.message.len = strlen(text) + crypto_box_MACBYTES;

    crypto_box_easy(msg.message.ptr, (unsigned char *)text, strlen(text), msg.nonce, rc_pubkey, privkey);
    memcpy(msg.sender_pubkey, sender_pubkey, crypto_box_PUBLICKEYBYTES);
    memcpy(msg.from, from, strlen(from));
    memcpy(msg.to, to, strlen(to));
    
    return msg;
}

void open_msg(Message* msg, uint8_t seed[32], char* decrypted_text) {
    if (sodium_init() < 0) {
        printf("[-] Sodium init error");
        return;
    }
    uint8_t privkey[crypto_box_SECRETKEYBYTES];
    uint8_t temp[32];

    crypto_box_seed_keypair(temp, privkey, seed);

    crypto_box_open_easy((unsigned char *)decrypted_text, msg->message.ptr, msg->message.len, msg->nonce, msg->sender_pubkey, privkey);
    decrypted_text[msg->message.len - crypto_box_MACBYTES] = '\0';

}
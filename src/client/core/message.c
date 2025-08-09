#include "../../../include/common.h"
#include "../../../libs/cjson/cJSON.h"
#include "../../utils/hex.h"
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


Message build_msg(char * text, char from[32], char to[32],
    uint8_t rc_pubkey[crypto_box_PUBLICKEYBYTES],
    uint8_t sender_pubkey[crypto_box_PUBLICKEYBYTES],
    uint8_t seed[crypto_box_SECRETKEYBYTES]) {

    Message msg = {0};
    uint8_t privkey[crypto_box_SECRETKEYBYTES];
    uint8_t temp[32];

    randombytes_buf(msg.nonce, sizeof msg.nonce);
    crypto_box_seed_keypair(temp, privkey, seed);

    msg.ciphertext = malloc(strlen(text) + crypto_box_MACBYTES);
    msg.size = strlen(text) + crypto_box_MACBYTES;

    crypto_box_easy(msg.ciphertext, (unsigned char *)text, strlen(text), msg.nonce, rc_pubkey, privkey);
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

    crypto_box_open_easy((unsigned char *)decrypted_text, msg->ciphertext, msg->size, msg->nonce, msg->sender_pubkey, privkey);
    decrypted_text[msg->size - crypto_box_MACBYTES] = '\0';

}
#ifndef MESSAGE_H
#define MESSAGE_H
#include "common.h"

Message build_msg(char* text, const uint8_t from[32], const uint8_t to[32], uint8_t rc_pubkey[crypto_box_PUBLICKEYBYTES], uint8_t sender_pubkey[crypto_box_PUBLICKEYBYTES], uint8_t seed[crypto_box_SECRETKEYBYTES]);
void open_msg(Message* msg, uint8_t seed[32], char* decrypted_text);

#endif
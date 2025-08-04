#include "common.h"
#include <sodium.h>
#include <sodium/crypto_box.h>
#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_sign.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "base58/base58.h"



void sign_user_info(User user) {
    if (sodium_init() == -1) {
        printf("[-] Sodium init error");
        return;
    }

    uint8_t buffer[256];
    uint8_t hash[32];
    int offest = 0;
    memcpy(buffer + offest, user.id, strlen(user.id)); offest += strlen(user.id);
    memcpy(buffer + offest, user.name, strlen(user.name)); offest += strlen(user.name);
    memcpy(buffer + offest, user.pubkey_encr, 32); offest += 32;
    memcpy(buffer + offest, user.pubkey_sign, 32); offest += 32;

    crypto_hash_sha256(hash, buffer, offest);

    uint8_t full_key[64];
    memcpy(full_key, user.privkey, 32);
    memcpy(full_key + 32, user.pubkey_sign, 32);

    unsigned long long siglen_p = 0;
    crypto_sign_detached(user.signature, &siglen_p, hash, 32, full_key);
}


User get_account(uint8_t privkey[32], char username[17], char name[32]) {
    User user;
    memset(&user, 0, sizeof(User));
    if (sodium_init() == -1) {
        printf("[-] Sodium init error");
        return user;
    }

    uint8_t full_key[64];

    if (privkey == NULL) {
        crypto_sign_keypair(user.pubkey_sign, full_key);
        for (int i = 0; i < 32; i++) {
            user.privkey[i] = full_key[i];
        }
    } else {
        for (int i = 0; i < 32; i++) {
            user.privkey[i] = privkey[i];
        }
        crypto_sign_seed_keypair(user.pubkey_sign, full_key, user.privkey);
    }

    memset(full_key, 0, 64);
    crypto_box_seed_keypair(user.pubkey_encr, full_key, user.privkey);


    uint8_t both_pubkeys[64];
    memcpy(both_pubkeys, user.pubkey_encr, 32);
    memcpy(both_pubkeys + 32, user.pubkey_sign, 32);

    /*
    printf("\n");
    for (int i = 0; i < 64; i++) {
        printf("%02x", both_pubkeys[i]);
        if (i == 31) {printf(" ");}
    }
    printf("\n");
    */

    uint8_t hash[32];
    crypto_hash_sha256(hash, both_pubkeys, 64);

    uint8_t user_id[27];

    int count = 0;
    for (int i = 0; i < 17; i++) {
        if (username[i] == '\0') {break;}
        user_id[i] = username[i];
        count++;
    }

    user_id[count] = '#';
    count++;

    for (int i = 0; i < 8; i++) {
        user_id[count + i] = hash[i];
    }

    uint8_t checksum[32];
    crypto_hash_sha256(checksum, user_id, count + 8);

    uint8_t tag[10];
    for (int i = 0; i < 8; i++) {
        tag[i] = hash[i];
    }

    tag[8] = checksum[0];
    tag[9] = checksum[1];

    char* base58_tag = base58encode(tag, 10);

    for (int i = 0; i < 14; i++) {
        user_id[count + i] = base58_tag[i];
    }
    count += 14;

    for (int i = 0; i < count; i++) {
        user.id[i] = user_id[i];
    }

    user.id[count] = '\0';

    for (int i = 0; i < 32; i++) {
        user.name[i] = name[i];
        if (name[i] == '\0') {break;}
    }

    sign_user_info(user);

    return user;
}
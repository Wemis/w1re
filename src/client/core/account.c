#include "../../shared/common.h"
#include "../../shared/hex.h"
#include "../../../libs/base58/base58.h"
#include <sodium.h>
#include <sodium/crypto_box.h>
#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_sign.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void sign_user_info(User *user) {
    if (sodium_init() < 0) {
        printf("[-] Sodium init error");
        return;
    }

    uint8_t buffer[256] = {0};
    uint8_t hash[32];
    int offset = 0;
    memcpy(buffer + offset, user->id, strlen(user->id));
    offset += strlen(user->id);
    memcpy(buffer + offset, user->name, strlen(user->name));
    offset += strlen(user->name);
    memcpy(buffer + offset, user->pubkey_encr, 32);
    offset += 32;
    memcpy(buffer + offset, user->pubkey_sign, 32);
    offset += 32;

    crypto_hash_sha256(hash, buffer, offset);

    uint8_t full_key[64];
    memcpy(full_key, user->privkey, 32);
    memcpy(full_key + 32, user->pubkey_sign, 32);

    unsigned long long siglen_p = 0;
    crypto_sign_detached(user->signature, &siglen_p, hash, 32, full_key);
}

User get_account(uint8_t privkey[32], char username[17], char name[32]) {
    User user = {0};
    if (sodium_init() == -1) {
        printf("[-] Sodium init error");
        return user;
    }

    uint8_t full_key[crypto_sign_SECRETKEYBYTES];

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

    memset(full_key, 0, crypto_sign_SECRETKEYBYTES);
    crypto_box_seed_keypair(user.pubkey_encr, full_key, user.privkey);

    uint8_t both_pubkeys[2 * crypto_sign_PUBLICKEYBYTES];
    memcpy(both_pubkeys, user.pubkey_encr, crypto_sign_PUBLICKEYBYTES);
    memcpy(both_pubkeys + crypto_sign_PUBLICKEYBYTES, user.pubkey_sign,
           crypto_sign_PUBLICKEYBYTES);

    uint8_t hash[32];
    crypto_hash_sha256(hash, both_pubkeys, 64);

    uint8_t user_id[27];

    int count = 0;
    for (int i = 0; i < 17; i++) {
        if (username[i] == '\0') {
            break;
        }
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
    memcpy(tag, hash, 8);
    memcpy(tag + 8, checksum, 2);

    const char *base58_tag = base58encode(tag, 10);

    for (int i = 0; i < 14; i++) {
        user_id[count + i] = base58_tag[i];
    }
    count += 14;

    memcpy(user.id, user_id, count);
    user.id[count] = '\0';

    const size_t name_len = strlen(name);
    memcpy(user.name, name, name_len);
    user.name[name_len] = '\0';

    sign_user_info(&user);

    return user;
}

int verify_account(const User user) {
    if (sodium_init() < 0) {
        printf("[-] Sodium init error");
        return 0;
    }

    uint8_t buffer[256] = {0};
    uint8_t hash[32];
    int offset = 0;

    memcpy(buffer + offset, user.id, strlen(user.id));
    offset += strlen(user.id);
    memcpy(buffer + offset, user.name, strlen(user.name));
    offset += strlen(user.name);
    memcpy(buffer + offset, user.pubkey_encr, 32);
    offset += 32;
    memcpy(buffer + offset, user.pubkey_sign, 32);
    offset += 32;

    crypto_hash_sha256(hash, buffer, offset);

    if (crypto_sign_verify_detached(user.signature, hash, 32,
                                    user.pubkey_sign) == -1) {

        return 0;
    }

    int start = 0;
    for (int i = 0; i < strlen(user.id); i++) {
        if (user.id[i] == '#') {
            start = i + 1;
        }
    }

    char tag_base58[14];
    for (int i = 0; i < 14; i++) {

        tag_base58[i] = user.id[start + i];
    }

    uint8_t tag[10];
    base58decode(tag_base58, tag);
    uint8_t both_pubkeys[2 * crypto_sign_PUBLICKEYBYTES];
    memcpy(both_pubkeys, user.pubkey_encr, crypto_sign_PUBLICKEYBYTES);
    memcpy(both_pubkeys + crypto_sign_PUBLICKEYBYTES, user.pubkey_sign,
           crypto_sign_PUBLICKEYBYTES);

    crypto_hash_sha256(hash, both_pubkeys, 64);

    for (int i = 0; i < 8; i++) {
        if (tag[i] != hash[i]) {
            return 0;
        }
    }

    return 1;
}


void print_account(const User u) {
    printf("Name: %s  ", u.name);
    print_hex(u.name, strlen(u.name));
    printf("ID: %s  %d", u.id, strlen(u.id));
    print_hex(u.id, strlen(u.id));
    printf("Private Key: ");
    print_hex(u.privkey, 32);
    printf("Public Key Encr: ");
    print_hex(u.pubkey_encr, 32);
    printf("Public Key Sign: ");
    print_hex(u.pubkey_sign, 32);
    printf("Signature: ");
    print_hex(u.signature, 64);
}
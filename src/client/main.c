#include "../../include/account.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

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

int main(void) {
    char* usrname = "danylo";
    char* name = "Danylo";
    char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";

    uint8_t key[32];
    hex_to_bytes(privkey_hex, key, 32);

    User usr = get_account(key, usrname, name);

    printf("User ID: %s\n", usr.id);
    printf("Name: %s\n", usr.name);

    printf("Private key: ");
    print_hex(usr.privkey, 32);

    printf("Public key sign: ");
    print_hex(usr.pubkey_sign, 32);

    printf("Public key encr: ");
    print_hex(usr.pubkey_encr, 32);

    printf("\nSignature: ");
    print_hex(usr.signature, 64);
}
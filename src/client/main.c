#include "../../include/account.h"
#include <arpa/inet.h>
#include <stdio.h>
#include "../utils/hex.h"



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

    int r = verify_account(usr);
    printf("\nVerification result: %d\n", r);
}
#include "../../include/account.h"
#include "../../include/message.h"
#include "../utils/hex.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



int main(void) {
    char* usrname = "danylo";
    char* name = "Danylo";
    char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";
    char* rc_pubkey_hex = "fa557f6a1f95eeeeb2d30474678154bc83b577ca04787a7025be007939d6944d";
    char* sender_pubkey_hex = "2cf4572420b402ed2a7949c554b6e93c2122880f80c1d052be1611412e100d7a";

    uint8_t key[32];
    uint8_t rc_pubkey[32];
    uint8_t sender_pubkey[32];
    hex_to_bytes(privkey_hex, key, 32);
    hex_to_bytes(rc_pubkey_hex, rc_pubkey, 32);
    hex_to_bytes(sender_pubkey_hex, sender_pubkey, 32);

    User usr = get_account(key, usrname, name);

    Message msg = build_msg("hello, it's secret", usr.id, "alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);

    printf("From: %s\n", msg.from);
    printf("To: %s\n", msg.to);
    printf("Message: ");
    print_hex(msg.ciphertext, msg.size + 16);
    printf("Sender Public key: ");
    print_hex(msg.sender_pubkey, 32);

    char* alice_key_hex = "303802c6c569385ba586a11d1f18f842b79ca38aef07ab73409c017e4312a0c5";
    uint8_t alice_key[32];
    hex_to_bytes(alice_key_hex, alice_key, 32);

    open_msg(&msg, alice_key);

    printf("\nDecrypted: %s\n", msg.decrypted_text);

    free(msg.ciphertext);
    free(msg.decrypted_text);
}
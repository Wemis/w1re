#include "common.h"
#include <openssl/rand.h>


User get_account(uint8_t privkey[32]) {
    User user;
    if (privkey == NULL) {
        if (!RAND_bytes(user.privkey, 32)) {
            fprintf(stderr, "[-] Failed to generate private key\n");
            return user;
        }
    } else {
        for (int i = 0; i < 32; i++) {
            user.privkey[i] = privkey[i];
        }
    }

}
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
    msg.size = strlen(text);

    crypto_box_easy(msg.ciphertext, (unsigned char *)text, strlen(text), msg.nonce, rc_pubkey, privkey);
    memcpy(msg.sender_pubkey, sender_pubkey, crypto_box_PUBLICKEYBYTES);
    strcpy(msg.from, from);
    strcpy(msg.to, to);
    
    return msg;
}

void open_msg(Message* msg, uint8_t seed[32]) {
    if (sodium_init() < 0) {
        printf("[-] Sodium init error");
        return;
    }
    uint8_t privkey[crypto_box_SECRETKEYBYTES];
    uint8_t temp[32];

    crypto_box_seed_keypair(temp, privkey, seed);

    msg->decrypted_text = malloc(msg->size + 1);

    crypto_box_open_easy((unsigned char *)msg->decrypted_text, msg->ciphertext, msg->size + crypto_box_MACBYTES, msg->nonce, msg->sender_pubkey, privkey);
    msg->decrypted_text[msg->size] = '\0';

}
/*
int send_msg(Message msg) {
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "command", "send");
    cJSON_AddStringToObject(json, "message", msg.text);
    cJSON_AddStringToObject(json, "rc_user_id", msg.to);

    char* data = cJSON_Print(json);
    cJSON_Delete(json);

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-] Can't create socket");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("[-] Invalid address");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[-] Connection failed");
        return 1;
    }

    ssize_t sent = send(sock, data, strlen(data) + 1, 0);
    if (sent < 0) {
        perror("send");
    } else {
        printf("[*] Sent %zd bytes\n", sent);
    }

    close(sock);
    free(data);
    return 0;
}
*/
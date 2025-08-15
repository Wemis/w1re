#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "../../include/common.h"
#include "../../libs/cjson/cJSON.h"
#include "account.h"

int send_msg(const Message msg, struct bufferevent *bev) {
    cJSON* json = cJSON_CreateObject();
    cJSON* nonce = cJSON_CreateArray();
    cJSON* pubkey = cJSON_CreateArray();
    cJSON* message = cJSON_CreateObject();
    cJSON* content = cJSON_CreateArray();

    cJSON_AddStringToObject(json, "command", "send");
    cJSON_AddStringToObject(json, "rc_user_id", (const char*)msg.to);
    cJSON_AddStringToObject(json, "from_user_id", (const char*)msg.from);

    for (int i = 0; i < 24; i++) {
        cJSON_AddItemToArray(nonce, cJSON_CreateNumber(msg.nonce[i]));
    }

    for (int i = 0; i < 32; i++) {
        cJSON_AddItemToArray(pubkey, cJSON_CreateNumber(msg.sender_pubkey[i]));
    }

    for (size_t i = 0; i < msg.message.len; i++) {
        cJSON_AddItemToArray(content, cJSON_CreateNumber(((double*)msg.message.ptr)[i]));
    }

    cJSON_AddItemToObject(json, "nonce", nonce);
    cJSON_AddItemToObject(json, "sender_pubkey", pubkey);

    cJSON_AddItemToObject(message, "content", content);
    cJSON_AddNumberToObject(message, "size", msg.message.len);

    cJSON_AddItemToObject(json, "message", message);

    const char * json_payload = cJSON_Print(json);
    cJSON_Delete(json);

    uint8_t header[4];
    const uint32_t len = strlen(json_payload);
    const uint32_t len_be = htonl(len);

    memcpy(header, &len_be, 4);

    uint8_t packet[4 + strlen(json_payload)];
    memcpy(packet, header, 4);
    memcpy(packet + 4, json_payload, len);

    bufferevent_write(bev, json_payload, strlen(json_payload));
    /*
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
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[-] Connection failed");
        close(sock);
        return 1;
    }

    const ssize_t packet_size = 4 + strlen(json_payload);
    ssize_t total_sent = 0;
    while (total_sent < packet_size) {
        const ssize_t sent = send(sock, packet + total_sent, packet_size - total_sent, 0);
        if (sent <= 0) {
            perror("[-] Data not sent");
            close(sock);
            return 1;
        }
        total_sent += sent;
    }

    printf("[*] Sent %zu bytes\n", total_sent);
    close(sock);
    */
    return 0;
}

int send_msg_binary(const Message msg) {
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "command", "send");
    cJSON_AddStringToObject(json, "rc_user_id", msg.to);
    cJSON_AddStringToObject(json, "from_user_id", msg.from);
    cJSON_AddNumberToObject(json, "message_size", msg.message.len);

    char* json_payload = cJSON_Print(json);
    cJSON_Delete(json);

    uint8_t bytes_payload[24 + 32 + msg.message.len];

    memcpy(bytes_payload, msg.nonce, 24);
    memcpy(bytes_payload + 24, msg.sender_pubkey, 32);
    memcpy(bytes_payload + 24 + 32, msg.message.ptr, msg.message.len);

    uint8_t header[8];
    const uint32_t len = strlen(json_payload) + 24 + 32 + msg.message.len;
    const uint32_t json_len = strlen(json_payload);

    const uint32_t len_be = htonl(len);
    const uint32_t json_len_be = htonl(json_len);

    memcpy(header, &len_be, 4);
    memcpy(header + 4, &json_len_be, 4);

    uint8_t packet[8 + strlen(json_payload) + 24 + 32 + msg.message.len];
    memcpy(packet, header, 8);
    memcpy(packet + 8, json_payload, json_len);
    memcpy(packet + 8 + json_len, bytes_payload, 24 + 32 + msg.message.len);

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
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[-] Connection failed");
        close(sock);
        return 1;
    }

    const ssize_t packet_size = 8 + strlen(json_payload) + 24 + 32 + msg.message.len;
    ssize_t total_sent = 0;
    while (total_sent < packet_size) {
        const ssize_t sent = send(sock, packet + total_sent, packet_size - total_sent, 0);
        if (sent <= 0) {
            perror("[-] Data not sent");
            close(sock);
            return 1;
        }
        total_sent += sent;
    }

    printf("[*] Sent %zu bytes\n", total_sent);
    close(sock);
    return 0;
}


void login(uint8_t key[32], uint8_t username[17], uint8_t name[64], struct bufferevent *bev) {
    User u = get_account(key, username, name);
    cJSON* json = cJSON_CreateObject();
    cJSON* user = cJSON_CreateObject();
    cJSON* pubkey_s = cJSON_CreateArray();
    cJSON* pubkey_e = cJSON_CreateArray();
    cJSON* signature = cJSON_CreateArray();

    cJSON_AddStringToObject(json, "command", "login");

    cJSON_AddStringToObject(user, "id", u.id);
    cJSON_AddStringToObject(user, "name", u.name);

    for (int i = 0; i < 32; i++) {
        cJSON_AddItemToArray(pubkey_s, cJSON_CreateNumber(u.pubkey_sign[i]));
    }

    for (int i = 0; i < 32; i++) {
        cJSON_AddItemToArray(pubkey_e, cJSON_CreateNumber(u.pubkey_encr[i]));
    }

    for (int i = 0; i < 64; i++) {
        cJSON_AddItemToArray(signature, cJSON_CreateNumber(u.signature[i]));
    }

    cJSON_AddItemToObject(user, "pubkey_s", pubkey_s);
    cJSON_AddItemToObject(user, "pubkey_e", pubkey_e);
    cJSON_AddItemToObject(user, "signature", signature);

    cJSON_AddItemToObject(json, "user", user);

    const char* json_string = cJSON_Print(json);
    cJSON_Delete(json);

    uint8_t header[4];
    const uint32_t len = strlen(json_string);
    const uint32_t len_be = htonl(len);

    memcpy(header, &len_be, 4);

    uint8_t packet[4 + strlen(json_string)];
    memcpy(packet, header, 4);
    memcpy(packet + 4, json_string, len);

    bufferevent_write(bev, json_string, strlen(json_string));
}
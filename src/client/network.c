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
#include "../shared/common.h"
#include "../../libs/cjson/cJSON.h"
#include "../shared/account.h"
#include "../shared/logger.h"
#include "../shared/serializer.h"

int send_msg(const Message msg, struct bufferevent *bev) {
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "command", "send");
    message_to_json(json, &msg);
    
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
    return 0;
}

int send_msg_binary(const Message msg) {
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "command", "send");
    cJSON_AddStringToObject(json, "rc_user_id", msg.to);
    cJSON_AddStringToObject(json, "from_user_id", msg.from);
    cJSON_AddNumberToObject(json, "message_size", msg.content.len);

    char* json_payload = cJSON_Print(json);
    cJSON_Delete(json);

    uint8_t bytes_payload[24 + 32 + msg.content.len];

    memcpy(bytes_payload, msg.nonce, 24);
    memcpy(bytes_payload + 24, msg.sender_pubkey, 32);
    memcpy(bytes_payload + 24 + 32, msg.content.ptr, msg.content.len);

    uint8_t header[8];
    const uint32_t len = strlen(json_payload) + 24 + 32 + msg.content.len;
    const uint32_t json_len = strlen(json_payload);

    const uint32_t len_be = htonl(len);
    const uint32_t json_len_be = htonl(json_len);

    memcpy(header, &len_be, 4);
    memcpy(header + 4, &json_len_be, 4);

    uint8_t packet[8 + strlen(json_payload) + 24 + 32 + msg.content.len];
    memcpy(packet, header, 8);
    memcpy(packet + 8, json_payload, json_len);
    memcpy(packet + 8 + json_len, bytes_payload, 24 + 32 + msg.content.len);

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

    const ssize_t packet_size = 8 + strlen(json_payload) + 24 + 32 + msg.content.len;
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


void login(User u, struct bufferevent *bev) {
    memset(u.privkey, 0, 32);
    cJSON* json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "command", "login");

    user_to_json(json, &u);

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
#include "common.h"
#include "../libs/cjson/cJSON.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>


Message build_msg(char text[1024], char* to, int size) {
    Message msg;
    memset(&msg, 0, sizeof(Message));
    strcpy(msg.text, text);
    strcpy(msg.to, to);
    msg.size = size;
    
    return msg;
}

int send_msg(Message msg) {
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "command", "send");
    cJSON_AddStringToObject(json, "message", msg.text);
    cJSON_AddStringToObject(json, "to", msg.to);

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
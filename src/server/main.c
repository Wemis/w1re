#include "../strings.c"
#include "assert.h"
#include "stdio.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HANDLE_ERROR(err)                                                      \
    {                                                                          \
        perror(err);                                                           \
        exit(-1);                                                              \
    };

int main(void) {
    const char sock_path[] = "127.0.0.1";

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
        HANDLE_ERROR("sock");
    struct sockaddr_in serv_addr, client_addr;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(5330);
    serv_addr.sin_family = AF_INET;
    int err = bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err == -1)
        HANDLE_ERROR("bind");
    if (listen(sock, 50) == -1)
        HANDLE_ERROR("listen");
    unsigned int client_addr_size = sizeof(client_addr);
    int cl_sock = accept(sock, (struct sockaddr *)&client_addr, &client_addr_size);
    if (cl_sock == -1)
        HANDLE_ERROR("accept");

    if (send(cl_sock, "hello world", 11, MSG_NOSIGNAL) == -1)
        HANDLE_ERROR("send");

    close(sock);
    return 0;
}

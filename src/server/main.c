#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../slice.h"
#include "assert.h"
#include "stdio.h"
#include "threads.h"
#include <execinfo.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

KHASH_MAP_INIT_STR(STR_INT, int)

#define HANDLE_ERROR(err)                                                      \
    {                                                                          \
        perror(err);                                                           \
        exit(-1);                                                              \
    };

#define ADDRESS INADDR_ANY
#define PORT 5330

typedef struct ServerCtx {
    khash_t(STR_INT) * clients;
} Server;

typedef struct ClientParams {
    Server *serv;
    int sock;
} ClientParams;

void crash_handler(int sig) {
    void *array[10];
    size_t size = backtrace(array, 10);
    write(2, "Stack trace:\n", 13);
    backtrace_symbols_fd(array, size, 2);
    _exit(1);
}

int send_message(Server *self, const Slice receiver, const Slice message) {
    int sock = kh_get(STR_INT, self->clients, receiver.ptr);
    return send(sock, message.ptr, message.len, 0);
}

int process_command(Server *self, const Slice buf, int sock) {
    cJSON *json = cJSON_Parse((const char *)buf.ptr);
    const char *command =
        cJSON_GetObjectItemCaseSensitive(json, "command")->valuestring;
    const int cmd_len = strlen(command);
    if (!strncmp(command, "reg", 3)) {
        const char *user_id =
            cJSON_GetObjectItemCaseSensitive(json, "user_id")->valuestring;
        int res;
        khiter_t k = kh_put(STR_INT, self->clients, user_id, &res);
        kh_value(self->clients, k) = sock;
    } else if (!strncmp(command, "send", 4)) {
        const char *receiver_user_id =
            cJSON_GetObjectItemCaseSensitive(json, "rc_user_id")->valuestring;
        const char *message =
            cJSON_GetObjectItemCaseSensitive(json, "message")->valuestring;
        send_message(self, slice_from_str(receiver_user_id),
                     slice_from_str(message));
    }
    return 0;
}

int serve_client(void *args) {
    ClientParams *ctx = (ClientParams *)args;
    while (1) {
        char msg_buf[1024];
        int bytes_received;

        if ((bytes_received = read(ctx->sock, msg_buf, 1024)) == -1) {
            HANDLE_ERROR("read")
        } else if (bytes_received == 0)
            break;

        process_command(ctx->serv, slice_from_arr(msg_buf, bytes_received),
                        ctx->sock);
    }
    close(ctx->sock);
    free(ctx);
    return 0;
}

int main(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGPIPE, crash_handler);
    signal(SIGABRT, crash_handler);
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
        HANDLE_ERROR("sock");
    struct sockaddr_in serv_addr, client_addr;
    serv_addr.sin_addr.s_addr = ADDRESS;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_family = AF_INET;
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }
    int err = bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (err == -1)
        HANDLE_ERROR("bind");
    if (listen(sock, 50) == -1)
        HANDLE_ERROR("listen");

    unsigned int client_addr_size = sizeof(client_addr);
    Server *serv = malloc(sizeof(Server));
    *serv = (Server){.clients = kh_init(STR_INT)};
    while (1) {
        int cl_sock =
            accept(sock, (struct sockaddr *)&client_addr, &client_addr_size);
        if (cl_sock == -1)
            HANDLE_ERROR("accept");
        thrd_t thread;
        ClientParams *params = malloc(sizeof(ClientParams));
        *params = (ClientParams){.serv = serv, .sock = cl_sock};
        if (thrd_create(&thread, serve_client, (void *)params) < 0) {
            HANDLE_ERROR("thread");
        }
    }
    kh_destroy_STR_INT(serv->clients);
    free(serv);
    close(sock);
    return 0;
}

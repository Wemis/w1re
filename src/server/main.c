#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../slice.h"
#include "commands.h"
#include "server.h"
#include <arpa/inet.h>
#include <execinfo.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <threads.h>
#include <unistd.h>

#define HANDLE_ERROR(msg)                                                      \
    do {                                                                       \
        perror(msg);                                                           \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

#define ADDRESS INADDR_ANY
#define PORT 5330
#define BACKLOG 50
#define BUFFER_SIZE 1024

typedef struct ClientParams {
    Server *server;
    int sock;
} ClientParams;

void crash_handler(int _) {
    void *array[10];
    const size_t size = backtrace(array, 10);
    write(STDERR_FILENO, "Stack trace:\n", 13);
    backtrace_symbols_fd(array, (int)size, STDERR_FILENO);
    exit(1);
}

void init_signals(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGPIPE, crash_handler);
    signal(SIGABRT, crash_handler);
}

void server_init(Server *server) {
    server->clients = kh_init(STR_INT);
    pthread_mutex_init(&server->clients_mutex, nullptr);
}

void server_free(Server *server) {
    kh_destroy_STR_INT(server->clients);
    pthread_mutex_destroy(&server->clients_mutex);
}

int process_command(Server *server, const Slice buf, const int sock) {
    cJSON *json = cJSON_Parse(buf.ptr);
    if (!json) {
        fprintf(stderr, "process_command: failed to parse JSON\n");
        return -1;
    }

    const cJSON *cmd_item = cJSON_GetObjectItemCaseSensitive(json, "command");
    if (!cJSON_IsString(cmd_item) || !cmd_item->valuestring) {
        fprintf(stderr, "process_command: missing command field\n");
        cJSON_Delete(json);
        return -1;
    }

    const char *command = cmd_item->valuestring;
    int result = 0;

    if (!strncmp(command, "reg", 3)) {
        result = command_register(server, json, sock);
    } else if (!strncmp(command, "send", 4)) {
        result = command_send(server, json);
    } else {
        fprintf(stderr, "process_command: unknown command '%s'\n", command);
        result = -1;
    }

    cJSON_Delete(json);
    return result;
}

int setup_socket(void) {
    const int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
        HANDLE_ERROR("socket");

    constexpr int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        HANDLE_ERROR("setsockopt");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ADDRESS;
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        HANDLE_ERROR("bind");

    return sock;
}

int serve_client(void *arg) {
    const auto params = (ClientParams *)arg;
    pthread_detach(pthread_self());

    char buffer[BUFFER_SIZE];

    while (1) {
        const size_t received = read(params->sock, buffer, sizeof(buffer));
        if (received == -1) {
            perror("read");
            break;
        }
        if (received == 0) {
            break;
        }
        process_command(params->server, slice_from_arr(buffer, received),
                        params->sock);
    }

    close(params->sock);
    free(params);
    return 0;
}

int server_listen(Server* server, const int sock) {
    if (listen(sock, BACKLOG) < 0)
        HANDLE_ERROR("listen");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        const int client_sock =
            accept(sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        ClientParams *params = malloc(sizeof(ClientParams));
        if (!params) {
            close(client_sock);
            HANDLE_ERROR("malloc");
        }
        params->server = server;
        params->sock = client_sock;

        thrd_t thread;
        if (thrd_create(&thread, serve_client, params) < 0)
            HANDLE_ERROR("thread");
    }
}

int main(void) {
    init_signals();

    const int server_sock = setup_socket();

    Server *server = malloc(sizeof(Server));
    if (!server)
        HANDLE_ERROR("malloc");
    server_init(server);

    server_listen(server, server_sock);

    server_free(server);
    free(server);
    close(server_sock);
    return 0;
}

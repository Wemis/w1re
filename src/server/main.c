#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../utils/logger.h"
#include "../utils/slice.h"
#include "commands.h"
#include "server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <threads.h>
#include <unistd.h>

#define ADDRESS INADDR_ANY
#define PORT 5330
#define BACKLOG 50
#define BUFFER_SIZE 1024

typedef struct ClientParams {
    Server *server;
    int sock;
} ClientParams;

void crash_handler(const int sig) {
    LOG_ERRORF("%d", sig);
    exit(1);
}

void init_signals(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGPIPE, crash_handler);
    signal(SIGABRT, crash_handler);
}

void server_init(Server *server) {
    if (server) {
        server->clients = kh_init(STR_INT);
        pthread_mutex_init(&server->clients_mutex, nullptr);
    }
}

void server_free(Server *server) {
    if (server) {
        kh_destroy_STR_INT(server->clients);
        pthread_mutex_destroy(&server->clients_mutex);
    }
}

int process_command(Server *server, const Slice buf, const int sock) {
    LOG_INFO("process_command");
    cJSON *json = cJSON_Parse(buf.ptr);
    if (!json) {
        LOG_ERROR("JSON Parse Error");
        return -1;
    }

    const cJSON *cmd_item = cJSON_GetObjectItemCaseSensitive(json, "command");
    if (!cJSON_IsString(cmd_item) || !cmd_item->valuestring) {
        LOG_ERROR("JSON parsing error. Expected string value");
        cJSON_Delete(json);
        return -1;
    }

    const char *command = cmd_item->valuestring;
    int result = 0;

    if (!strncmp(command, "reg", 3)) {
        LOG_INFO("Registering client");
        result = command_register(server, json, sock);
    } else if (!strncmp(command, "send", 4)) {
        LOG_INFO("Sending message");
        result = command_send(server, json);
    } else {
        LOG_ERROR("unknown command");
        result = -1;
    }

    LOG_INFO("freeing resources");
    cJSON_Delete(json);
    return result;
}

int setup_socket(void) {
    const int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1)
        LOG_ERROR("socket");

    constexpr int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        LOG_ERROR("setsockopt");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ADDRESS;
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        LOG_ERROR("bind");

    return sock;
}

int serve_client(void *arg) {
    const auto params = (ClientParams *)arg;
    pthread_detach(pthread_self());

    char buffer[BUFFER_SIZE];
    while (1) {
        const size_t received = read(params->sock, buffer, sizeof(buffer));
        LOG_INFOF("%lu", received);
        if (received == -1) {
            LOG_ERROR("read");
            break;
        }
        if (received == 0) {
            LOG_INFO("connection closed");
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
    if (listen(sock, BACKLOG) < 0) {
        LOG_ERROR("listen");
        return -1;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        const int client_sock =
            accept(sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == -1) {
            LOG_ERROR("accept");
            continue;
        }

        ClientParams *params = malloc(sizeof(ClientParams));
        if (!params) {
            close(client_sock);
            LOG_ERROR("malloc failed");
            continue;
        }
        params->server = server;
        params->sock = client_sock;

        thrd_t thread;
        if (thrd_create(&thread, serve_client, params) < 0) {
            free(params);
            LOG_ERROR("thread");
        }
    }
}

int main(void) {
    init_signals();

    const int server_sock = setup_socket();

    Server *server = malloc(sizeof(Server));
    if (!server)
        LOG_ERROR("malloc failed");
    server_init(server);

    server_listen(server, server_sock);

    server_free(server);
    free(server);
    close(server_sock);
    return 0;
}

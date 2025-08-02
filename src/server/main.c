#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../slice.h"
#include <assert.h>
#include <execinfo.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#define HANDLE_ERROR(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define ADDRESS INADDR_ANY
#define PORT 5330
#define BACKLOG 50
#define BUFFER_SIZE 1024

KHASH_MAP_INIT_STR(STR_INT, int)

typedef struct ServerCtx {
    khash_t(STR_INT) *clients;
    pthread_mutex_t clients_mutex;
} Server;

typedef struct ClientParams {
    Server *server;
    int sock;
} ClientParams;

void crash_handler(int sig) {
    void *array[10];
    size_t size = backtrace(array, 10);
    write(STDERR_FILENO, "Stack trace:\n", 13);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    _exit(1);
}

void init_signals(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGPIPE, crash_handler);
    signal(SIGABRT, crash_handler);
}

void init_server(Server *server) {
    server->clients = kh_init(STR_INT);
    pthread_mutex_init(&server->clients_mutex, NULL);
}

void cleanup_server(Server *server) {
    kh_destroy_STR_INT(server->clients);
    pthread_mutex_destroy(&server->clients_mutex);
}

int send_message(Server *server, const Slice receiver, const Slice message) {
    pthread_mutex_lock(&server->clients_mutex);
    khiter_t k = kh_get(STR_INT, server->clients, receiver.ptr);
    if (k == kh_end(server->clients)) {
        pthread_mutex_unlock(&server->clients_mutex);
        fprintf(stderr, "send_message: user '%.*s' not found\n",
                (int)receiver.len, (const char *)receiver.ptr);
        return -1;
    }
    int sock = kh_value(server->clients, k);
    pthread_mutex_unlock(&server->clients_mutex);
    return send(sock, message.ptr, message.len, 0);
}

int register_user(Server *server, const char *user_id, int sock) {
    pthread_mutex_lock(&server->clients_mutex);

    char *user_id_copy = strdup(user_id);
    if (!user_id_copy) {
        perror("strdup");
        pthread_mutex_unlock(&server->clients_mutex);
        return -1;
    }

    int ret;
    khiter_t k = kh_put(STR_INT, server->clients, user_id_copy, &ret);
    if (ret == 0) {
        free(user_id_copy);
    }
    kh_value(server->clients, k) = sock;
    pthread_mutex_unlock(&server->clients_mutex);
    printf("Registered user '%s' with sock %d\n", user_id, sock);
    return 0;
}

int handle_register(Server *server, cJSON *json, int sock) {
    cJSON *user_id_item = cJSON_GetObjectItemCaseSensitive(json, "user_id");
    if (!cJSON_IsString(user_id_item) || !user_id_item->valuestring) {
        fprintf(stderr, "register: invalid user_id\n");
        return -1;
    }
    return register_user(server, user_id_item->valuestring, sock);
}

int handle_send(Server *server, cJSON *json, int sock) {
    cJSON *receiver_item = cJSON_GetObjectItemCaseSensitive(json, "rc_user_id");
    cJSON *message_item = cJSON_GetObjectItemCaseSensitive(json, "message");

    if (!cJSON_IsString(receiver_item) || !cJSON_IsString(message_item)) {
        fprintf(stderr, "send: invalid receiver or message\n");
        return -1;
    }

    return send_message(server, slice_from_str(receiver_item->valuestring),
                        slice_from_str(message_item->valuestring));
}

int process_command(Server *server, const Slice buf, int sock) {
    cJSON *json = cJSON_Parse((const char *)buf.ptr);
    if (!json) {
        fprintf(stderr, "process_command: failed to parse JSON\n");
        return -1;
    }

    cJSON *cmd_item = cJSON_GetObjectItemCaseSensitive(json, "command");
    if (!cJSON_IsString(cmd_item) || !cmd_item->valuestring) {
        fprintf(stderr, "process_command: missing command field\n");
        cJSON_Delete(json);
        return -1;
    }

    const char *command = cmd_item->valuestring;
    int result = 0;

    if (!strncmp(command, "reg", 3)) {
        result = handle_register(server, json, sock);
    } else if (!strncmp(command, "send", 4)) {
        result = handle_send(server, json, sock);
    } else {
        fprintf(stderr, "process_command: unknown command '%s'\n", command);
        result = -1;
    }

    cJSON_Delete(json);
    return result;
}

int serve_client(void *arg) {
    ClientParams *params = (ClientParams *)arg;
    pthread_detach(pthread_self());

    char buffer[BUFFER_SIZE];

    while (1) {
        int received = read(params->sock, buffer, sizeof(buffer));
        if (received == -1) {
            perror("read");
            break;
        } else if (received == 0) {
            break; // client closed connection
        }
        process_command(params->server, slice_from_arr(buffer, received), params->sock);
    }

    close(params->sock);
    free(params);
    return 0;
}

int setup_socket(void) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) HANDLE_ERROR("socket");

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        HANDLE_ERROR("setsockopt");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ADDRESS;
    addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        HANDLE_ERROR("bind");

    if (listen(sock, BACKLOG) < 0)
        HANDLE_ERROR("listen");

    return sock;
}

int main(void) {
    init_signals();

    int server_sock = setup_socket();

    Server *server = malloc(sizeof(Server));
    if (!server) HANDLE_ERROR("malloc");
    init_server(server);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        ClientParams *params = malloc(sizeof(ClientParams));
        if (!params) {
            perror("malloc");
            close(client_sock);
            continue;
        }
        params->server = server;
        params->sock = client_sock;

        thrd_t thread;
        if (thrd_create(&thread, serve_client, params) < 0)
            HANDLE_ERROR("thread");
    }

    cleanup_server(server);
    free(server);
    close(server_sock);
    return 0;
}

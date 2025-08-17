#include "../shared/logger.h"
#include "../shared/slice.h"
#include "commands.h"
#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT     5330
#define MAX_EVENTS      64
#define BUFFER_SIZE     4096

static int set_nonblocking(const int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void crash_handler(const int sig) {
    LOG_ERROR("Caught signal %d, exiting...", sig);
    _exit(1);
}

void init_signals(void) {
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGPIPE, crash_handler);
}

static int create_listen_socket(void) {
    const int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
        return -1;

    const int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (set_nonblocking(listen_fd) < 0) {
        close(listen_fd);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(listen_fd);
        return -1;
    }
    if (listen(listen_fd, SOMAXCONN) < 0) {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}

int server_init(Server *srv) {
    if (!srv) return -1;

    srv->sock = create_listen_socket();
    if (srv->sock < 0) {
        LOG_ERROR("Failed to create listen socket");
        return -1;
    }

    srv->epoll_fd = epoll_create1(0);
    if (srv->epoll_fd < 0) {
        close(srv->sock);
        return -1;
    }
    return 0;
}

static int handle_protocol(const Server *srv, const char *data, size_t len, int sock) {
    cJSON *json = cJSON_ParseWithLength(data, len);
    if (!json) {
        LOG_ERROR("Invalid JSON from client %d", sock);
        return -1;
    }

    const cJSON *cmd_item = cJSON_GetObjectItemCaseSensitive(json, "command");
    if (!cJSON_IsString(cmd_item)) {
        LOG_ERROR("Missing 'command' field");
        cJSON_Delete(json);
        return -1;
    }

    const char *command = cmd_item->valuestring;
    int result = 0;

    if (strcmp(command, "login") == 0) {
        result = command_login(srv, json, sock);
    } else if (strcmp(command, "send") == 0) {
        result = command_send(srv, json);
    } else {
        LOG_ERROR("Unknown command: %s", command);
        result = -1;
    }

    cJSON_Delete(json);
    return result;
}

static void on_new_client(const Server *srv) {
    while (1) {
        const int client_fd = accept(srv->sock, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // всі клієнти прийняті
            LOG_ERROR("accept() failed");
            break;
        }

        set_nonblocking(client_fd);
        struct epoll_event ev = {0};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

        LOG_INFO("New client connected: %d", client_fd);
    }
}

static void on_client_data(const Server *srv, int fd) {
    char buffer[BUFFER_SIZE];
    const ssize_t count = read(fd, buffer, sizeof(buffer));
    if (count <= 0) {
        LOG_INFO("Client %d disconnected", fd);
        epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return;
    }

    if (handle_protocol(srv, buffer, (size_t)count, fd) < 0) {
        LOG_ERROR("Protocol error from client %d", fd);
    }
}

void server_run(const Server *srv) {
    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = srv->sock;
    epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, srv->sock, &ev);

    while (1) {
        const int n = epoll_wait(srv->epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0 && errno != EINTR) {
            LOG_ERROR("epoll_wait() failed");
            break;
        }

        for (int i = 0; i < n; i++) {
            const int fd = events[i].data.fd;
            if (fd == srv->sock) {
                on_new_client(srv);
            } else if (events[i].events & EPOLLIN) {
                on_client_data(srv, fd);
            }
        }
    }
}

void server_shutdown(Server *srv) {
    if (!srv) return;
    kh_destroy_STR_INT(srv->clients);
    close(srv->sock);
    close(srv->epoll_fd);
}

int main(void) {
    log_init();
    init_signals();

    Server srv = {NULL};
    srv.clients = kh_init_STR_INT();

    if (server_init(&srv) != 0) return 1;

    LOG_INFO("Server listening on port %d", SERVER_PORT);
    server_run(&srv);
    server_shutdown(&srv);

    return 0;
}

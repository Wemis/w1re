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

#define PORT                5330
#define MAX_EVENTS          64
#define BUFFER_SIZE         4096

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

int server_init(Server *srv) {
    if (!srv)
        return -1;
    const int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("socket()");
        return -1;
    }
    if (set_nonblocking(listen_fd) < 0) {
        LOG_ERROR("set_nonblocking()");
        close(listen_fd);
        return -1;
    }
    constexpr int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind()");
        close(listen_fd);
        return -1;
    }
    if (listen(listen_fd, SOMAXCONN) < 0) {
        LOG_ERROR("listen()");
        close(listen_fd);
        return -1;
    }
    srv->sock = listen_fd;
    srv->epoll_fd = epoll_create1(0);
    if (srv->epoll_fd < 0) {
        LOG_ERROR("epoll_create1()");
        close(listen_fd);
        return -1;
    }
    return 0;
}

int process_command(const Server *server, const Slice buf, const int sock) {
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

    if (!strncmp(command, "login", 5)) {
        LOG_INFO("Registering client");
        result = command_login(server, json, sock);
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

void server_run(const Server *srv) {
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = srv->sock;
    epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, srv->sock, &ev);

    while (1) {
        const int n = epoll_wait(srv->epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0 && errno != EINTR) {
            LOG_ERROR("epoll_wait()");
            break;
        }
        for (int i = 0; i < n; i++) {
            const int fd = events[i].data.fd;
            if (fd == srv->sock) {
                while (1) {
                    const int client_fd = accept(srv->sock, nullptr, nullptr);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        LOG_ERROR("accept()");
                        break;
                    }
                    set_nonblocking(client_fd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                    LOG_INFO("Client connected: %d", client_fd);
                }
            } else if (events[i].events & EPOLLIN) {
                char buffer[BUFFER_SIZE];
                const ssize_t count = read(fd, buffer, sizeof(buffer));
                if (count <= 0) {
                    LOG_INFO("Client %d disconnected", fd);
                    epoll_ctl(srv->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                } else {
                    const Slice slice = { .ptr = buffer, .len = (size_t)count };
                    if (process_command(srv, slice, fd) < 0) {
                        LOG_ERROR("processing client %d failed", fd);
                    }
                }
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

    Server srv;
    srv.clients = kh_init_STR_INT();
    if (server_init(&srv) != 0) return 1;

    LOG_INFO("Server listening on port %d", PORT);
    server_run(&srv);
    server_shutdown(&srv);
    return 0;
}

#pragma once

#include "../../libs/khash.h"
#include <pthread.h>


KHASH_MAP_INIT_STR(STR_INT, int)

typedef struct Server {
    khash_t(STR_INT) * clients;
    int sock;
    int epoll_fd;
} Server;

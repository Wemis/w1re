#pragma once

#include "../../libs/khash.h"
#include <pthread.h>


KHASH_MAP_INIT_STR(STR_INT, int)
KHASH_MAP_INIT_INT(INT_STR, char*)

typedef struct Server {
    khash_t(STR_INT) * clients_by_id;
    khash_t(INT_STR) * clients_by_sock;
    int sock;
    int epoll_fd;
} Server;

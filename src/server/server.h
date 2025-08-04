#pragma once

#include "../../libs/khash.h"
#include <pthread.h>


KHASH_MAP_INIT_STR(STR_INT, int)

typedef struct Server {
    khash_t(STR_INT) * clients;
    pthread_mutex_t clients_mutex;
} Server;

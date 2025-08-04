#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../utils/logger.h"
#include "../utils/slice.h"
#include "server.h"

#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

int command_register(Server *server, const cJSON *json, const int sock) {
    const cJSON *user_id_item = cJSON_GetObjectItemCaseSensitive(json, "user_id");
    if (!cJSON_IsString(user_id_item) || !user_id_item->valuestring) {
        return -1;
    }
    pthread_mutex_lock(&server->clients_mutex);

    const char *user_id = user_id_item->valuestring;

    char *user_id_copy = strdup(user_id);
    if (!user_id_copy) {
        pthread_mutex_unlock(&server->clients_mutex);
        return -1;
    }

    int ret;
    const khiter_t k = kh_put(STR_INT, server->clients, user_id_copy, &ret);
    if (ret == 0) {
        free(user_id_copy);
    }
    kh_value(server->clients, k) = sock;
    pthread_mutex_unlock(&server->clients_mutex);
    return 0;
}

int command_send(Server *server, const cJSON *json) {
    const cJSON *receiver_item =
        cJSON_GetObjectItemCaseSensitive(json, "rc_user_id");
    const cJSON *message_item = cJSON_GetObjectItemCaseSensitive(json, "message");

    if (!cJSON_IsString(receiver_item) || !cJSON_IsString(message_item)) {
        return -1;
    }

    const char *receiver = receiver_item->valuestring;
    const Slice message = {.ptr = message_item->valuestring,
                     .len = strlen(message_item->valuestring)};

    pthread_mutex_lock(&server->clients_mutex);
    const khiter_t k = kh_get(STR_INT, server->clients, receiver);
    if (k == kh_end(server->clients)) {
        pthread_mutex_unlock(&server->clients_mutex);
        return -1;
    }
    const int sock = kh_value(server->clients, k);
    pthread_mutex_unlock(&server->clients_mutex);
    return send(sock, message.ptr, message.len, 0);
}

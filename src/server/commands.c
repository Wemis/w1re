#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../shared/common.h"
#include "../shared/logger.h"
#include "../shared/serializer.h"
#include "server.h"

#include <string.h>
#include <sys/socket.h>

int command_register(const Server *server, const cJSON *json, const int sock) {
    User user;
    if (json_to_user(json, &user) < 0) {
        LOG_ERROR("User parsing error");
        return -1;
    }

    int ret;
    const khiter_t k = kh_put(STR_INT, server->clients, user.id, &ret);
    if (ret != 0) {
        LOG_ERROR("Hashtable put error");
        return -1;
    }
    kh_value(server->clients, k) = sock;
    return 0;
}

int command_send(const Server *server, const cJSON *json) {
    Message message;
    if (json_to_message(json, &message) < 0)
        return -1;

    const khiter_t k = kh_get(STR_INT, server->clients, message.from);
    if (k == kh_end(server->clients)) {
        return -1;
    }
    const int sock = kh_value(server->clients, k);

    cJSON *payload = cJSON_CreateObject();
    message_to_json(payload, &message);

    const char *resp = cJSON_Print(payload);
    cJSON_Delete(payload);
    const int resp_len = strlen(resp);

    const int sent = send(sock, resp, resp_len, 0);
    free(message.content.ptr);
    return sent;
}

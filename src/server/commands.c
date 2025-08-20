#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../shared/common.h"
#include "../shared/account.h"
#include "../shared/logger.h"
#include "../shared/serializer.h"
#include "server.h"

#include <string.h>
#include <sys/socket.h>

int command_login(const Server *server, const cJSON *json, const int sock) {
    User user = {0};
    log_init();
    if (json_to_user(json, &user) < 0) {
        LOG_ERROR("User parsing error");
        return -1;
    }

    int ret, ret2;
    char* user_id = strdup(user.id);
    const khint_t k1 = kh_put(STR_INT, server->clients_by_id, user_id, &ret);
    const khint_t k2 = kh_put(INT_STR, server->clients_by_sock, sock, &ret2);
    if (ret < 0 || ret2 < 0) {
        LOG_ERROR("Hashtable put error");
        return -1;
    }

    if (!verify_account(user)) {
        LOG_WARN("User %s does not pass verification", user.id);
        // Server can send negative response
    }

    kh_value(server->clients_by_id, k1) = sock;
    kh_value(server->clients_by_sock, k2) = user_id;
    LOG_INFO("User %s authenticated, verified: %d", user.id, verify_account(user));
    return 0;
}

int command_send(const Server *server, const cJSON *json) {
    Message message;
    log_init();
    if (json_to_message(json, &message) < 0) {
        return -1;
    }

    const khiter_t k1 = kh_get(STR_INT, server->clients_by_id, message.from);
    if (k1 == kh_end(server->clients_by_id)) {
        LOG_INFO("User %s trying to send message and not authenticated", message.from);
        // Server can send negative response
        return 1;
    }

    const khiter_t k2 = kh_get(STR_INT, server->clients_by_id, message.to);
    if (k2 == kh_end(server->clients_by_id)) {
        LOG_INFO("Receiver %s is not online, saving message to database", message.to);
        // Database recording
        return 1;
    }

    const int sock = kh_value(server->clients_by_id, k2);

    cJSON *payload = cJSON_CreateObject();
    message_to_json(payload, &message);

    const char *resp = cJSON_Print(payload);
    cJSON_Delete(payload);
    const int resp_len = strlen(resp);

    const int sent = send(sock, resp, resp_len, 0);
    LOG_INFO("Message successfully delivered to %s", message.to);
    free(message.content.ptr);
    return sent;
}

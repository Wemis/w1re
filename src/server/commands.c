#include "../../include/common.h"
#include "../../libs/cjson/cJSON.h"
#include "../../libs/khash.h"
#include "../utils/logger.h"
#include "../utils/slice.h"
#include "server.h"

#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

#define EXPECT(field, expected_type, field_type, str_type)                     \
    if (field->type != expected_type) {                                        \
        LOG_ERROR(field_type " field must be " str_type);                      \
        return -1;                                                             \
    }

#define cJSON_CopyArray(field, arr_name, arr_size)                             \
    for (size_t i = 0; i < arr_size; i++) {                                    \
        arr_name[i] = cJSON_GetArrayItem(field, i)->valueint;                  \
    }

int command_register(const Server *server, const cJSON *json, const int sock) {
    const cJSON *user_id_item =
        cJSON_GetObjectItemCaseSensitive(json, "user_id");
    if (!cJSON_IsString(user_id_item) || !user_id_item->valuestring) {
        return -1;
    }

    const char *user_id = user_id_item->valuestring;

    char *user_id_copy = strdup(user_id);
    if (!user_id_copy) {
        return -1;
    }

    int ret;
    const khiter_t k = kh_put(STR_INT, server->clients, user_id_copy, &ret);
    if (ret == 0) {
        free(user_id_copy);
    }
    kh_value(server->clients, k) = sock;
    return 0;
}

int command_send(const Server *server, const cJSON *json) {
    const cJSON *from = cJSON_GetObjectItemCaseSensitive(json, "from");
    const cJSON *to = cJSON_GetObjectItemCaseSensitive(json, "to");
    const cJSON *nonce = cJSON_GetObjectItemCaseSensitive(json, "nonce");
    const cJSON *pubkey = cJSON_GetObjectItemCaseSensitive(json, "pubkey");
    const cJSON *message = cJSON_GetObjectItemCaseSensitive(json, "message");

    if (from->type != cJSON_String) {
        LOG_ERROR("`from` field must have type string");
        return -1;
    }
    if (to->type != cJSON_String) {
        LOG_ERROR("`to` field must have type string");
        return -1;
    }
    if (nonce->type != cJSON_Array) {
        LOG_ERROR("`nonce` field must have type array");
        return -1;
    }
    if (pubkey->type != cJSON_Array) {
        LOG_ERROR("`pubkey` field must have type array");
        return -1;
    }
    if (message->type != cJSON_Object) {
        LOG_ERROR("`message` field must have type object");
        return -1;
    }

    const cJSON *message_content = cJSON_GetObjectItemCaseSensitive(message, "content");
    const cJSON *message_length = cJSON_GetObjectItemCaseSensitive(message, "length");

    Message msg = {
        .message = {.ptr = message_content->valuestring, .len = message_length->valueint}};

    memcpy(msg.from, from->valuestring, strlen(from->valuestring));
    memcpy(msg.to, to->valuestring, strlen(to->valuestring));

    msg.from[strlen(from->valuestring)] = '\0';
    msg.to[strlen(to->valuestring)] = '\0';

    if (cJSON_GetArraySize(nonce) != 24) {
        LOG_ERROR("nonce must be at exactly 24 bytes long");
        return -1;
    }
    if (cJSON_GetArraySize(pubkey) != 32) {
        LOG_ERROR("pubkey must be at exactly 24 bytes long");
        return -1;
    }
    cJSON_CopyArray(nonce, msg.nonce, 24);
    cJSON_CopyArray(pubkey, msg.sender_pubkey, 32);

    const khiter_t k = kh_get(STR_INT, server->clients, from->valuestring);
    if (k == kh_end(server->clients)) {
        return -1;
    }
    const int sock = kh_value(server->clients, k);

    cJSON *ret = cJSON_CreateObject();
    cJSON *from_r = cJSON_CreateString((const char *)msg.from);
    cJSON *to_r = cJSON_CreateString((const char *)msg.to);
    cJSON *nonce_r = cJSON_CreateArray();
    for (size_t i = 0; i < 24; i++) {
        cJSON_AddItemToArray(nonce_r, cJSON_CreateNumber(msg.nonce[i]));
    }
    cJSON *pubkey_r = cJSON_CreateArray();
    for (size_t i = 0; i < 24; i++) {
        cJSON_AddItemToArray(pubkey_r,
                             cJSON_CreateNumber(msg.sender_pubkey[i]));
    }
    cJSON *message_r = cJSON_CreateObject();
    cJSON_AddStringToObject(message_r, "content", msg.message.ptr);
    cJSON_AddNumberToObject(message_r, "length", msg.message.len);

    cJSON_AddItemReferenceToObject(ret, "from", from_r);
    cJSON_AddItemReferenceToObject(ret, "to", to_r);
    cJSON_AddItemReferenceToObject(ret, "nonce", nonce_r);
    cJSON_AddItemReferenceToObject(ret, "pubkey", pubkey_r);
    cJSON_AddItemReferenceToObject(ret, "message", message_r);

    const char *resp = cJSON_Print(ret);
    const int resp_len = strlen(resp);

    const int sent = send(sock, resp, resp_len, 0);
    return sent;
}

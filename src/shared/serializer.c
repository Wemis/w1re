#include "serializer.h"
#include "logger.h"

int json_to_message(const cJSON *json, Message *message) {
    const cJSON *from_field = cJSON_GetObjectItemCaseSensitive(json, "from");
    const cJSON *to_field = cJSON_GetObjectItemCaseSensitive(json, "to");
    const cJSON *nonce_field = cJSON_GetObjectItemCaseSensitive(json, "nonce");
    const cJSON *pubkey_field = cJSON_GetObjectItemCaseSensitive(json, "pubkey");
    const cJSON *message_field = cJSON_GetObjectItemCaseSensitive(json, "message");

    if (!cJSON_IsString(from_field)) {
        LOG_ERROR("`from` field must have type string");
        return -1;
    }
    if (!cJSON_IsString(to_field)) {
        LOG_ERROR("`to` field must have type string");
        return -1;
    }
    if (!cJSON_IsArray(nonce_field)) {
        LOG_ERROR("`nonce` field must have type array");
        return -1;
    }
    if (!cJSON_IsArray(pubkey_field)) {
        LOG_ERROR("`pubkey` field must have type array");
        return -1;
    }
    if (!cJSON_IsObject(message_field)) {
        LOG_ERROR("`message` field must have type object");
        return -1;
    }

    const cJSON *message_content = cJSON_GetObjectItemCaseSensitive(message_field, "content");
    const cJSON *message_length = cJSON_GetObjectItemCaseSensitive(message_field, "length");

    memcpy(message->from, from_field->valuestring, 32);
    memcpy(message->to, to_field->valuestring, 32);
    if (cJSON_GetArraySize(nonce_field) != 24) {
        LOG_ERROR("nonce must be at exactly 24 bytes long");
        return -1;
    }
    if (cJSON_GetArraySize(pubkey_field) != 32) {
        LOG_ERROR("pubkey must be at exactly 32 bytes long");
        return -1;
    }
    for (size_t i = 0; i < cJSON_GetArraySize(nonce_field); i++) {
        message->nonce[i] = cJSON_GetArrayItem(nonce_field, i)->valueint;
    }
    for (size_t i = 0; i < cJSON_GetArraySize(pubkey_field); i++) {
        message->sender_pubkey[i] = cJSON_GetArrayItem(pubkey_field, i)->valueint;
    }
    int* content_ptr = malloc(sizeof(int)*cJSON_GetArraySize(message_content));
    for (size_t i = 0; i < cJSON_GetArraySize(message_content); i++) {
        content_ptr[i] = cJSON_GetArrayItem(message_content, i)->valueint;
    }
    message->content.ptr = content_ptr;
    message->content.len = message_length->valueint;
    return 0;
}

void message_to_json(cJSON *json, const Message* message) {
    cJSON *from = cJSON_CreateString((const char*)&message->from);
    cJSON *to = cJSON_CreateString((const char*)&message->to);
    cJSON *nonce = cJSON_CreateArray();
    for (size_t i = 0; i < 24; i++) {
        cJSON_AddItemToArray(nonce, cJSON_CreateNumber(message->nonce[i]));
    }
    cJSON *pubkey = cJSON_CreateArray();
    for (size_t i = 0; i < 24; i++) {
        cJSON_AddItemToArray(pubkey, cJSON_CreateNumber(message->sender_pubkey[i]));
    }
    cJSON *message_r = cJSON_CreateObject();
    cJSON *message_content = cJSON_AddArrayToObject(message_r, "content");
    for (size_t i = 0; i < message->content.len; i++) {
        cJSON_AddItemToArray(message_content, cJSON_CreateNumber(((double*)message->content.ptr)[i]));
    }
    cJSON_AddNumberToObject(message_r, "length", message->content.len);

    cJSON_AddItemReferenceToObject(json, "from", from);
    cJSON_AddItemReferenceToObject(json, "to", to);
    cJSON_AddItemReferenceToObject(json, "nonce", nonce);
    cJSON_AddItemReferenceToObject(json, "pubkey", pubkey);
    cJSON_AddItemReferenceToObject(json, "message", message_r);
}

int json_to_user(const cJSON *json, User* user) {
    const cJSON *user_name = cJSON_GetObjectItemCaseSensitive(json, "user_name");
    const cJSON *user_id = cJSON_GetObjectItemCaseSensitive(json, "user_id");
    const cJSON *pubkey_sign = cJSON_GetObjectItemCaseSensitive(json, "pubkey_sign");
    const cJSON *pubkey_encr = cJSON_GetObjectItemCaseSensitive(json, "pubkey_encr");
    const cJSON *signature = cJSON_GetObjectItemCaseSensitive(json, "signature");

    if (!cJSON_IsString(user_name)) {
        LOG_ERROR("`user_name` field must be a string");
        return -1;
    }
    if (!cJSON_IsString(user_id)) {
        LOG_ERROR("`user_id` field must be a string");
        return -1;
    }
    if (!cJSON_IsArray(pubkey_sign)) {
        LOG_ERROR("`pubkey_sign` field must be an array");
        return -1;
    }
    if (!cJSON_IsArray(pubkey_encr)) {
        LOG_ERROR("`pubkey_encr` field must be an array");
        return -1;
    }
    if (!cJSON_IsArray(signature)) {
        LOG_ERROR("`signature` field must be an array");
        return -1;
    }
    memcpy(user->id, user_id->valuestring, strlen(user_id->valuestring));
    memcpy(user->name, user_name->valuestring, strlen(user_name->valuestring));
    for (size_t i = 0; i < cJSON_GetArraySize(pubkey_sign); i++) {
        user->pubkey_sign[i] = cJSON_GetArrayItem(pubkey_sign, i)->valueint;
    }
    for (size_t i = 0; i < cJSON_GetArraySize(pubkey_encr); i++) {
        user->pubkey_encr[i] = cJSON_GetArrayItem(pubkey_encr, i)->valueint;
    }
    for (size_t i = 0; i < cJSON_GetArraySize(signature); i++) {
        user->signature[i] = cJSON_GetArrayItem(signature, i)->valueint;
    }
    return 0;
}

void user_to_json(cJSON* json, const User* user) {
    cJSON *user_name = cJSON_CreateString((const char*)&user->name);
    cJSON *user_id = cJSON_CreateString((const char*)&user->id);
    cJSON *pubkey_sign = cJSON_AddArrayToObject(json, "pubkey_sign");
    cJSON *pubkey_encr = cJSON_AddArrayToObject(json, "pubkey_encr");
    cJSON *signature = cJSON_AddArrayToObject(json, "signature");
    for (size_t i = 0; i < sizeof(user->pubkey_sign); i++) {
        cJSON_AddItemToArray(pubkey_sign, cJSON_CreateNumber(user->pubkey_sign[i]));
    }
    for (size_t i = 0; i < sizeof(user->pubkey_encr); i++) {
        cJSON_AddItemToArray(pubkey_encr, cJSON_CreateNumber(user->pubkey_encr[i]));
    }
    for (size_t i = 0; i < sizeof(user->signature); i++) {
        cJSON_AddItemToArray(signature, cJSON_CreateNumber(user->signature[i]));
    }
    cJSON_AddItemToObject(json, "user_name", user_name);
    cJSON_AddItemToObject(json, "user_id", user_id);
}

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
    message->message.ptr = strdup(message_content->valuestring);
    message->message.len = message_length->valueint;
    return 0;
}

void message_to_json(cJSON *json, const Message* message) {
    cJSON *from_r = cJSON_CreateString(message->from);
    cJSON *to_r = cJSON_CreateString(message->to);
    cJSON *nonce_r = cJSON_CreateArray();
    for (size_t i = 0; i < 24; i++) {
        cJSON_AddItemToArray(nonce_r, cJSON_CreateNumber(message->nonce[i]));
    }
    cJSON *pubkey_r = cJSON_CreateArray();
    for (size_t i = 0; i < 24; i++) {
        cJSON_AddItemToArray(pubkey_r, cJSON_CreateNumber(message->sender_pubkey[i]));
    }
    cJSON *message_r = cJSON_CreateObject();
    cJSON_AddStringToObject(message_r, "content", message->message.ptr);
    cJSON_AddNumberToObject(message_r, "length", message->message.len);

    cJSON_AddItemReferenceToObject(json, "from", from_r);
    cJSON_AddItemReferenceToObject(json, "to", to_r);
    cJSON_AddItemReferenceToObject(json, "nonce", nonce_r);
    cJSON_AddItemReferenceToObject(json, "pubkey", pubkey_r);
    cJSON_AddItemReferenceToObject(json, "message", message_r);
}

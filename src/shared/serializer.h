#pragma once

#include "../../libs/cjson/cJSON.h"
#include "common.h"

int json_to_message(const cJSON *json, Message *message);
void message_to_json(cJSON *json, const Message* message);

int json_to_user(const cJSON *json, User* user);
void user_to_json(cJSON *json, const User* user);

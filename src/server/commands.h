#pragma once

#include "../../libs/cjson/cJSON.h"
#include "server.h"


int command_login(const Server *server, const cJSON *json, int sock);
int command_send(const Server *server, const cJSON *json);

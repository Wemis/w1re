#pragma once

#include "../../libs/cjson/cJSON.h"
#include "server.h"


int command_register(Server *server, const cJSON *json, int sock);
int command_send(Server *server, const cJSON *json);

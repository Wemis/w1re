#pragma once

#include "../../libs/cjson/cJSON.h"
#include "server.h"


int command_register(Server *server, cJSON *json, int sock);
int command_send(Server *server, cJSON *json);

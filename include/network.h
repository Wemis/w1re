#ifndef NETWORK_H
#define NETWORK_H
#include "common.h"
#include <event2/bufferevent.h>

int send_msg_binary(Message msg);
int send_msg(Message msg);
void login(uint8_t key[32], uint8_t username[17], uint8_t name[64], struct bufferevent *bev);

#endif
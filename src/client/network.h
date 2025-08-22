#ifndef NETWORK_H
#define NETWORK_H

#include <event2/bufferevent.h>
#include "../shared/common.h"

int send_msg_binary(Message msg);
int send_msg(Message msg, struct bufferevent *bev);
void login(User u, struct bufferevent *bev);
void *event_thread(void *arg);

#endif
#ifndef MESSAGE_H
#define MESSAGE_H
#include "common.h"

Message build_msg(char text[1024], char* to, int size);
int send_msg(Message msg);

#endif
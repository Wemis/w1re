#include "common.h"


Message build_msg(char text[1024], int size) {
    Message msg;
    for (int i = 0; i < size; i++) {
        msg.text[i] = text[i];
    }
    msg.size = size;
    
    return msg;
}
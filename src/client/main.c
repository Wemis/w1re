#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "../../include/message.h"


int main(void) {
    char* message = "hello";

    Message msg = build_msg(message, "vlad#fidsoow", 5);
    send_msg(msg);
}
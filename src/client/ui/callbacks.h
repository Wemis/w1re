#ifndef CALLBACKS_H
#define CALLBACKS_H
#include "../../shared/common.h"

#define STATUS_CONNECTED 0
#define STATUS_DISCONNECTED 1
#define STATUS_AUTHENTICATED 2
#define STATUS_TRYING_TO_AUTHENTICATE 3
#define STATUS_NOT_AUTHENTICATED 4

void ui_change_status(AppData *app, int status);

#endif
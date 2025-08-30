#include <gtk/gtk.h>
#include <stdio.h>
#include "callbacks.h"


void ui_change_status(AppData *app, int status) {
    char status_text[32];
    switch (status) {
        case STATUS_CONNECTED:
            if (app->h->status_text) {
                gtk_label_set_text(app->h->status_text, (char*)app->u->name);//"Authentication...");
            }
            break;
        
        case STATUS_DISCONNECTED:
            snprintf(status_text, 32, "Connecting... %ds", app->seconds);
            if (app->h->status_text) {
                gtk_label_set_text(app->h->status_text, status_text);
            }
            break;

        default:
            break;    
    }
}
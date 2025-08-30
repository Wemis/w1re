#include "../shared/account.h"
#include "../shared/common.h"
#include "../shared/hex.h"
#include "gtk/gtkcssprovider.h"
#include "gtk/gtkshortcut.h"
#include "message.h"
#include "network.h"
#include "dbmanager.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>


#define STYLE_PATH_APPDIR (char*)"/.local/share/w1re/resources/style.css"
#define STYLE_PATH (char*)"/w1re/src/client/resources/style.css"


static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *box;
    GtkWidget *header_bar;
    AppData *appdata = user_data;

    appdata->window = gtk_application_window_new (app);
    gtk_window_set_title(GTK_WINDOW(appdata->window), "Messenger");
    gtk_window_set_default_size(GTK_WINDOW(appdata->window), 840, 500);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_child(GTK_WINDOW(appdata->window), paned);

    appdata->chat_list_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_paned_set_start_child(GTK_PANED(paned), appdata->chat_list_container);
    gtk_widget_set_size_request(appdata->chat_list_container, 50, -1); 
    gtk_widget_add_css_class(GTK_WIDGET(appdata->chat_list_container), "chats-list-container");

    appdata->chat_view_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_paned_set_end_child(GTK_PANED(paned), appdata->chat_view_container);
    gtk_widget_set_size_request(appdata->chat_view_container, 100, -1); 
    gtk_widget_add_css_class(GTK_WIDGET(appdata->chat_view_container), "chat-view-container");

    appdata->h->header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    appdata->h->status_text = GTK_LABEL(gtk_label_new("Connecting..."));
    gtk_box_append(GTK_BOX(appdata->h->header), GTK_WIDGET(appdata->h->status_text));
    gtk_box_append(GTK_BOX(appdata->chat_list_container), appdata->h->header);
    gtk_widget_set_size_request(appdata->h->header, -1, 30);
    gtk_widget_add_css_class(GTK_WIDGET(appdata->h->header), "status-header");

    /*box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(box, "main-box");
    gtk_window_set_child(GTK_WINDOW(window), box);

    header_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(header_bar, TRUE);
    gtk_widget_set_margin_start(header_bar, 10);
    gtk_widget_set_margin_end(header_bar, 10);
    gtk_widget_set_margin_top(header_bar, 5);
    gtk_widget_set_margin_bottom(header_bar, 5);

    appdata->header = GTK_LABEL(gtk_label_new("Connecting..."));
    gtk_widget_add_css_class(GTK_WIDGET(appdata->header), "status");
    gtk_box_append(GTK_BOX(header_bar), GTK_WIDGET(appdata->header));
    gtk_box_append(GTK_BOX(box), header_bar);*/

    /*appdata->chat_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(appdata->chat_list, "chats-list");
    gtk_box_append(GTK_BOX(box), appdata->chat_list);*/
    gtk_paned_set_position(GTK_PANED(paned), 300);

    char *style_path = get_full_path(STYLE_PATH);
    FILE* style = fopen(style_path, "r");
    if (style == NULL) {
        printf(COLOR_RED "[ERR] Can't parse styles from file %s\n" COLOR_RESET, style_path);
        free(style_path);
    }

    fseek(style, 0, SEEK_END);
    long file_size = ftell(style);
    fseek(style, 0, SEEK_SET);

    char content[2048] = {0};
    size_t bytes_read = fread(content, 1, file_size, style);

    content[bytes_read] = '\0';

    GtkCssProvider *css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider, content);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);

    fclose(style);
    gtk_window_present(GTK_WINDOW(appdata->window));
}

void free_appdata(AppData *app) {
    free(app->u);
    free((void*)app->ip);
    free(app->h);
    free(app);

    app = NULL;
}


int main(int argc, char **argv) {
    init_appdir();
    User *u = malloc(sizeof(User));
    *u = (User){0};
    AppData *appdata = malloc(sizeof(AppData) * 2);
    *appdata = (AppData){0};
    appdata->u = u;
    appdata->ip = strdup(SERVER_IP);
    appdata->port = PORT;
    appdata->seconds = 0;
    appdata->u = u;
    appdata->h = malloc(sizeof(AppHeader));

    pthread_t tid;
    pthread_create(&tid, NULL, event_thread, appdata);

    uint8_t key[32] = {0};
    uint8_t name[64] = {0};
    uint8_t prefix[17] = {0};
    get_identity(key, name, prefix);

    *u = get_account(key, prefix, name);

    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.w1re.messenger", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), appdata);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);


    // Main program
    //LOG_INFO("[MAIN] Working...");

    //char *line;
    //char prompt[64];
    //strcpy(prompt, "w1re > ");

    //onst char *pkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";
    //uint8_t key[32];
    //hex_to_bytes(pkey_hex, key, 32);

    //u = get_account(key, "danylo", "Danylo");

    
    /*const char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";
    const char* rc_pubkey_hex = "fa557f6a1f95eeeeb2d30474678154bc83b577ca04787a7025be007939d6944d";
    const char* sender_pubkey_hex = "2cf4572420b402ed2a7949c554b6e93c2122880f80c1d052be1611412e100d7a";

    uint8_t key[32];
    uint8_t rc_pubkey[32];
    uint8_t sender_pubkey[32];
    hex_to_bytes(privkey_hex, key, 32);
    hex_to_bytes(rc_pubkey_hex, rc_pubkey, 32);
    hex_to_bytes(sender_pubkey_hex, sender_pubkey, 32);

    u = get_account(key, "danylo", "Danylo");

    const Message msg = build_msg("hello, it's secret", u.id, "alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);


    sleep(3);
    if (rctx.is_connected) {
        send_msg(msg, rctx.bev);
    } else {
        LOG_WARN("Can't send message: not connected");
    }
    free(msg.content.ptr);*/

    //pthread_join(tid, NULL);
    free_appdata(appdata);
    return 0;
}

void test_encr(void) {
    char* usrname = "danylo";
    char* name = "Danylo";
    const char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";
    const char* rc_pubkey_hex = "fa557f6a1f95eeeeb2d30474678154bc83b577ca04787a7025be007939d6944d";
    const char* sender_pubkey_hex = "2cf4572420b402ed2a7949c554b6e93c2122880f80c1d052be1611412e100d7a";

    uint8_t key[32];
    uint8_t rc_pubkey[32];
    uint8_t sender_pubkey[32];
    hex_to_bytes(privkey_hex, key, 32);
    hex_to_bytes(rc_pubkey_hex, rc_pubkey, 32);
    hex_to_bytes(sender_pubkey_hex, sender_pubkey, 32);

    const User usr = get_account(key, usrname, name);

    Message msg = build_msg("hello, it's secret", usr.id, (uint8_t*)"alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);

    printf("From: %s\n", (const char*)msg.from);
    printf("To: %s\n", (const char*)msg.to);
    printf("Message: ");
    print_hex(msg.content.ptr, msg.content.len);
    printf("Sender Public key: ");
    print_hex(msg.sender_pubkey, 32);

    const char * alice_key_hex = "303802c6c569385ba586a11d1f18f842b79ca38aef07ab73409c017e4312a0c5";
    uint8_t alice_key[32];
    hex_to_bytes(alice_key_hex, alice_key, 32);

    char* text = malloc(msg.content.len - crypto_box_MACBYTES + 1);;
    open_msg(&msg, alice_key, text);

    printf("\nDecrypted: %s\n", text);

    //send_msg(msg);

    free(msg.content.ptr);
    free(text);
}
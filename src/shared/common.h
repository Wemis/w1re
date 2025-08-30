#ifndef COMMON_H
#define COMMON_H
#include "gtk/gtk.h"
#include "gtk/gtkshortcut.h"
#include "slice.h"
#include <sodium/crypto_box.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

#define SERVER_IP "127.0.0.1"
#define PORT 5330

#define COLOR_GREEN "\033[32m"
#define COLOR_RED   "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"

typedef struct {
    uint8_t name[64];
    uint8_t id[32]; // 16 + 1 + 14 + \0
    uint8_t privkey[32];
    uint8_t pubkey_sign[32];
    uint8_t pubkey_encr[32];
    uint8_t signature[64];
} User;

typedef struct {
    uint8_t from[32];
    uint8_t to[32];
    uint8_t nonce[crypto_box_NONCEBYTES];
    uint8_t sender_pubkey[crypto_box_PUBLICKEYBYTES];
    Slice content;
} Message;

typedef struct {
    GtkWidget *header;
    GtkLabel *status_text;
} AppHeader;

typedef struct {
    bool is_connected;
    bool is_authenticated;
    struct event_base *base;
    struct bufferevent *bev;
    struct event *timer;
    int seconds;
    const char *ip;
    int port;
    GtkWidget *window;
    AppHeader *h;
    GtkWidget *chat_list_container;
    GtkWidget *chat_view_container;
    GtkWidget *chat_list;
    GtkWidget *chat_view;
    User *u;
} AppData;

#endif
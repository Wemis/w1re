#include "../shared/account.h"
#include "../shared/common.h"
#include "../shared/hex.h"
#include "../shared/logger.h"
#include "message.h"
#include "network.h"
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static struct event_base *base;

struct ReconnectCtx {
    bool is_connected;
    struct event_base *base;
    struct bufferevent *bev;
    struct event *timer;
    int seconds;
    const char *ip;
    int port;
};

void try_connect(evutil_socket_t fd, short what, void *arg);
void timer_cb(evutil_socket_t fd, short what, const void *arg);

void read_cb(struct bufferevent *bev, void *_) {
    char buf[4096];
    size_t n;
    struct evbuffer *input = bufferevent_get_input(bev);

    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        buf[n] = '\0';
        printf("[UPDATE] %s\n", buf);
    }
}

void event_cb(struct bufferevent *bev, const short events, void *ctx) {
    struct ReconnectCtx *rctx = ctx;
    log_init();
    
    if (events & BEV_EVENT_CONNECTED) {
        if (rctx->timer) {
            event_free(rctx->timer);
            rctx->timer = NULL;
        }
        rctx->seconds = 0;
        LOG_INFO("Connected to server");
        rctx->is_connected = true;

        const char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";

        uint8_t key[32];
        hex_to_bytes(privkey_hex, key, 32);

        login(key, (uint8_t*)"danylo", (uint8_t*)"Danylo", bev);
    }
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        bufferevent_free(bev);
        rctx->is_connected = false;
        rctx->bev = NULL;

        if (!rctx->timer) {
            rctx->timer =
                event_new(rctx->base, -1, EV_PERSIST, try_connect, rctx);
        }
        const struct timeval one_sec = {1, 0};
        event_add(rctx->timer, &one_sec);
    }
}

void timer_cb(evutil_socket_t fd, short what, const void *arg) {
    const struct ReconnectCtx *rctx = arg;
    LOG_INFO("[MAIN] Working...");
    
    const char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";
    const char* rc_pubkey_hex = "fa557f6a1f95eeeeb2d30474678154bc83b577ca04787a7025be007939d6944d";
    const char* sender_pubkey_hex = "2cf4572420b402ed2a7949c554b6e93c2122880f80c1d052be1611412e100d7a";

    uint8_t key[32];
    uint8_t rc_pubkey[32];
    uint8_t sender_pubkey[32];
    hex_to_bytes(privkey_hex, key, 32);
    hex_to_bytes(rc_pubkey_hex, rc_pubkey, 32);
    hex_to_bytes(sender_pubkey_hex, sender_pubkey, 32);

    const User usr = get_account(key, "danylo", "Danylo");

    const Message msg = build_msg("hello, it's secret", usr.id, (uint8_t*)"alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);
    
    send_msg(msg, rctx->bev);
    free(msg.content.ptr);
}

void try_connect(evutil_socket_t fd, short what, void *arg) {
    struct ReconnectCtx *rctx = arg;
    rctx->seconds++;
    log_init();
    LOG_WARN("Connecting... %d sec", rctx->seconds);

    if (!rctx->bev) {
        rctx->bev = bufferevent_socket_new(rctx->base, -1, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(rctx->bev, read_cb, NULL, event_cb, rctx);
        bufferevent_enable(rctx->bev, EV_READ | EV_WRITE);

        struct sockaddr_in sin = {0};
        sin.sin_family = AF_INET;
        sin.sin_port = htons(rctx->port);
        inet_pton(AF_INET, rctx->ip, &sin.sin_addr);

        if (bufferevent_socket_connect(rctx->bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            LOG_ERROR("[-] Immediate connect failed.\n");
            bufferevent_free(rctx->bev);
            rctx->bev = NULL;
        }
    }
}

void *event_thread(void *arg) {
    struct ReconnectCtx *rctx = arg;
    base = event_base_new();
    if (!base) {
        LOG_ERROR(stderr, "[-] libevent initialize error");
        return NULL;
    }
    rctx->base = base;
    
    try_connect(-1, 0, rctx);

    event_base_dispatch(rctx->base);
    return NULL;
}

int main() {
    log_init();
    struct ReconnectCtx rctx = {0};
    rctx.ip = SERVER_IP;
    rctx.port = PORT;
    rctx.seconds = 0;

    pthread_t tid;
    pthread_create(&tid, NULL, event_thread, &rctx);

    // Main program
    LOG_INFO("[MAIN] Working...");
    
    const char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";
    const char* rc_pubkey_hex = "fa557f6a1f95eeeeb2d30474678154bc83b577ca04787a7025be007939d6944d";
    const char* sender_pubkey_hex = "2cf4572420b402ed2a7949c554b6e93c2122880f80c1d052be1611412e100d7a";

    uint8_t key[32];
    uint8_t rc_pubkey[32];
    uint8_t sender_pubkey[32];
    hex_to_bytes(privkey_hex, key, 32);
    hex_to_bytes(rc_pubkey_hex, rc_pubkey, 32);
    hex_to_bytes(sender_pubkey_hex, sender_pubkey, 32);

    const User usr = get_account(key, "danylo", "Danylo");

    const Message msg = build_msg("hello, it's secret", usr.id, "alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);


    sleep(3);
    if (rctx.is_connected) {
        send_msg(msg, rctx.bev);
    } else {
        LOG_WARN("Can't send message: not connected");
    }
    free(msg.content.ptr);

    pthread_join(tid, NULL);
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
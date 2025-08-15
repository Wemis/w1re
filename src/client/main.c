#include "../../include/account.h"
#include "../../include/message.h"
#include "../../include/network.h"
#include "../utils/logger.h"
#include "../utils/hex.h"
#include "common.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <string.h>



static struct event_base *base;

void read_cb(struct bufferevent *bev, void *ctx) {
    char buf[4096];
    size_t n;
    size_t rc;
    struct evbuffer *input = bufferevent_get_input(bev);

    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        buf[n] = '\0';
        printf("[UPDATE] %s\n", buf);
    }
}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_CONNECTED) {
        printf("[*] Connected to server.\n");
        const char* privkey_hex = "dc8d6a2f464250e617577dcab5a99cf08613b429b1cc815ad412c47ce0ea96f1";

        uint8_t key[32];
        hex_to_bytes(privkey_hex, key, 32);

        login(key, (uint8_t*)"danylo", (uint8_t*)"Danylo", bev);
    }
    if (events & BEV_EVENT_EOF) {
        printf("[*] Connection closed.\n");
        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    }
    if (events & BEV_EVENT_ERROR) {
        printf("[*] Connection failed.\n");
        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    }
}

void timer_cb(evutil_socket_t fd, short what, void *arg) {
    printf("[MAIN LOOP] Working...\n");
    
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

    Message msg = build_msg("hello, it's secret", usr.id, "alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);
    
    send_msg(msg);
    free(msg.message.ptr);
}

int main() {
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "[-] libevent initialize error\n");
        return 1;
    }

    struct bufferevent *bev;
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    evutil_inet_pton(AF_INET, SERVER_IP, &sin.sin_addr);

    if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "[-] Connect failed.\n");
        bufferevent_free(bev);
        return 1;
    }

    struct event *timer_event;
    struct timeval one_sec = {1, 0};
    timer_event = event_new(base, -1, EV_TIMEOUT, timer_cb, base);
    event_add(timer_event, &one_sec);

    event_base_dispatch(base);

    event_free(timer_event);
    event_base_free(base);
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

    Message msg = build_msg("hello, it's secret", usr.id, "alice#3bTSGAkaDNpeS4", rc_pubkey, sender_pubkey, key);

    printf("From: %s\n", (const char*)msg.from);
    printf("To: %s\n", (const char*)msg.to);
    printf("Message: ");
    print_hex(msg.message.ptr, msg.message.len);
    printf("Sender Public key: ");
    print_hex(msg.sender_pubkey, 32);

    const char * alice_key_hex = "303802c6c569385ba586a11d1f18f842b79ca38aef07ab73409c017e4312a0c5";
    uint8_t alice_key[32];
    hex_to_bytes(alice_key_hex, alice_key, 32);

    char* text = malloc(msg.message.len - crypto_box_MACBYTES + 1);;
    open_msg(&msg, alice_key, text);

    printf("\nDecrypted: %s\n", text);

    send_msg(msg);

    free(msg.message.ptr);
    free(text);
}
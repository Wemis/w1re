#include "../shared/account.h"
#include "../shared/common.h"
#include "../shared/hex.h"
#include "message.h"
#include "network.h"
#include <SDL3/SDL_init.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

#define SDL_MAIN_USE_CALLBACKS
#include "ui/clay_renderer_SDL3.c"
#include "ui/clay-video-demo.c"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>


#define FONT_PATH (char*)"src/client/resources/SF-Pro.ttf"
#define IMAGE_PATH (char*)"src/client/resources/sample.png"


static const Uint32 FONT_ID = 0;

static const Clay_Color COLOR_ORANGE    = (Clay_Color) {225, 138, 50, 255};
static const Clay_Color COLOR_BLUE      = (Clay_Color) {111, 173, 162, 255};
static const Clay_Color COLOR_LIGHT     = (Clay_Color) {224, 215, 210, 255};

typedef struct app_state {
    SDL_Window *window;
    Clay_SDL3RendererData rendererData;
    ClayVideoDemo_Data demoData;
    User *u;
    ReconnectCtx *rctx;
    pthread_t network_tid;
} AppState;


SDL_Texture *sample_image;
bool show_demo = true;


static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData) {
    TTF_Font **fonts = userData;
    TTF_Font *font = fonts[config->fontId];
    int width, height;

    TTF_SetFontSize(font, config->fontSize);
    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return (Clay_Dimensions) { (float) width, (float) height };
}

void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

Clay_RenderCommandArray ClayImageSample_CreateLayout() {
    Clay_BeginLayout();

    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(0),
        .height = CLAY_SIZING_GROW(0)
    };

    CLAY({ .id = CLAY_ID("OuterContainer"),
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = layoutExpand,
            .padding = CLAY_PADDING_ALL(16),
            .childGap = 16
        }
    }) {
        CLAY({
            .id = CLAY_ID("SampleImage"),
            .layout = {
                .sizing = layoutExpand
            },
            .aspectRatio = { 23.0 / 42.0 },
            .image = {
                .imageData = sample_image,
            }
        });
    }

    return Clay_EndLayout();
}


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    User *u = malloc(sizeof(User));
    *u = (User){0};
    ReconnectCtx *rctx = malloc(sizeof(ReconnectCtx));
    *rctx = (ReconnectCtx){0};
    rctx->u = u;
    rctx->ip = strdup(SERVER_IP);
    rctx->port = PORT;
    rctx->seconds = 0;

    pthread_t tid;
    pthread_create(&tid, NULL, event_thread, rctx);

    (void) argc;
    (void) argv;

    if (!TTF_Init()) {
        return SDL_APP_FAILURE;
    }

    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    state->rctx = rctx;
    state->network_tid = tid;
    state->u = u;

    if (!SDL_CreateWindowAndRenderer("Messenger", 640, 480, 0, &state->window, &state->rendererData.renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowResizable(state->window, true);

    state->rendererData.textEngine = TTF_CreateRendererTextEngine(state->rendererData.renderer);
    if (!state->rendererData.textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts = SDL_calloc(1, sizeof(TTF_Font *));
    if (!state->rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    TTF_Font *font = TTF_OpenFont(FONT_PATH, 24);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->rendererData.fonts[FONT_ID] = font;

    sample_image = IMG_LoadTexture(state->rendererData.renderer, IMAGE_PATH);
    if (!sample_image) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load image: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Initialize Clay */
    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .memory = SDL_malloc(totalMemorySize),
        .capacity = totalMemorySize
    };

    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);
    Clay_Initialize(clayMemory, (Clay_Dimensions) { (float) width, (float) height }, (Clay_ErrorHandler) { HandleClayErrors });
    Clay_SetMeasureTextFunction(SDL_MeasureText, state->rendererData.fonts);

    state->demoData = ClayVideoDemo_Initialize();

    *appstate = state;
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    SDL_AppResult ret_val = SDL_APP_CONTINUE;

    switch (event->type) {
        case SDL_EVENT_QUIT:
            ret_val = SDL_APP_SUCCESS;
            break;
        case SDL_EVENT_KEY_UP:
            if (event->key.scancode == SDL_SCANCODE_SPACE) {
                show_demo = !show_demo;
            }
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            Clay_SetLayoutDimensions((Clay_Dimensions) { (float) event->window.data1, (float) event->window.data2 });
            break;
        case SDL_EVENT_MOUSE_MOTION:
            Clay_SetPointerState((Clay_Vector2) { event->motion.x, event->motion.y },
                                 event->motion.state & SDL_BUTTON_LMASK);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            Clay_SetPointerState((Clay_Vector2) { event->button.x, event->button.y },
                                 event->button.button == SDL_BUTTON_LEFT);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            Clay_UpdateScrollContainers(true, (Clay_Vector2) { event->wheel.x, event->wheel.y }, 0.01f);
            break;
        default:
            break;
    };

    return ret_val;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *state = appstate;

    Clay_RenderCommandArray render_commands = (show_demo
        ? ClayVideoDemo_CreateLayout(&state->demoData)
        : ClayImageSample_CreateLayout()
    );

    SDL_SetRenderDrawColor(state->rendererData.renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->rendererData.renderer);

    SDL_Clay_RenderClayCommands(&state->rendererData, &render_commands);

    SDL_RenderPresent(state->rendererData.renderer);

    return SDL_APP_CONTINUE;
}



void reconnect_ctx_free(ReconnectCtx *ctx) {
    if (!ctx) return;

    if (ctx->timer) event_free(ctx->timer);
    if (ctx->bev) bufferevent_free(ctx->bev);
    if (ctx->u) free(ctx->u);
    if (ctx->ip) free((char*)ctx->ip);

    free(ctx);
}



void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void) result;

    if (result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    AppState *state = appstate;
    reconnect_ctx_free(state->rctx);
    printf("\nGoodbye\n");

    if (sample_image) {
        SDL_DestroyTexture(sample_image);
    }

    if (state) {
        if (state->rendererData.renderer)
            SDL_DestroyRenderer(state->rendererData.renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        if (state->rendererData.fonts) {
            for(size_t i = 0; i < sizeof(state->rendererData.fonts) / sizeof(*state->rendererData.fonts); i++) {
                TTF_CloseFont(state->rendererData.fonts[i]);
            }

            SDL_free(state->rendererData.fonts);
        }

        if (state->rendererData.textEngine)
            TTF_DestroyRendererTextEngine(state->rendererData.textEngine);

        SDL_free(state);
    }
    TTF_Quit();
}



int main_old() {
    User u = {0};
    ReconnectCtx rctx = {0};
    rctx.u = &u;
    rctx.ip = SERVER_IP;
    rctx.port = PORT;
    rctx.seconds = 0;

    pthread_t tid;
    pthread_create(&tid, NULL, event_thread, &rctx);



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
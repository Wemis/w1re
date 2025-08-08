#pragma once

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __unix__
#include <execinfo.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#define LOG_QUEUE_SIZE 1024
#define LOG_MSG_MAXLEN 512

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

static const char* log_level_str[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static const char* log_level_color[] = {
    "\033[90m", // DEBUG - Gray
    "\033[36m", // INFO  - Cyan
    "\033[33m", // WARN  - Yellow
    "\033[31m"  // ERROR - Red
};

typedef struct {
    char messages[LOG_QUEUE_SIZE][LOG_MSG_MAXLEN];
    LogLevel levels[LOG_QUEUE_SIZE];
    const char* files[LOG_QUEUE_SIZE];
    int lines[LOG_QUEUE_SIZE];
    int head;
    int tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} log_queue_t;

static log_queue_t log_queue;

static void set_console_color(LogLevel level) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
    WORD color;
    switch (level) {
        case LOG_LEVEL_DEBUG: color = 8; break;  // Gray
        case LOG_LEVEL_INFO:  color = 11; break; // Cyan
        case LOG_LEVEL_WARN:  color = 14; break; // Yellow
        case LOG_LEVEL_ERROR: color = 12; break; // Red
        default: color = 7;
    }
    SetConsoleTextAttribute(hConsole, color);
#else
    fprintf(stderr, "%s", log_level_color[level]);
#endif
}

static void reset_console_color() {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 7);
#else
    fprintf(stderr, "\033[0m");
#endif
}

static void* log_thread_func(void* arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&log_queue.mutex);
        while (log_queue.head == log_queue.tail) {
            pthread_cond_wait(&log_queue.cond, &log_queue.mutex);
        }

        char msg[LOG_MSG_MAXLEN];
        LogLevel level = log_queue.levels[log_queue.head];
        const char* file = log_queue.files[log_queue.head];
        int line = log_queue.lines[log_queue.head];

        strncpy(msg, log_queue.messages[log_queue.head], LOG_MSG_MAXLEN - 1);
        msg[LOG_MSG_MAXLEN - 1] = '\0';

        log_queue.head = (log_queue.head + 1) % LOG_QUEUE_SIZE;

        pthread_mutex_unlock(&log_queue.mutex);

        // Форматуємо і виводимо повідомлення
        time_t t = time(NULL);
        struct tm lt;
#ifdef _WIN32
        localtime_s(&lt, &t);
#else
        localtime_r(&t, &lt);
#endif
        char time_buf[20];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &lt);

        set_console_color(level);
        fprintf(stderr, "[%s] [%s] %s:%d: %s\n", time_buf, log_level_str[level], file, line, msg);
        reset_console_color();
        fflush(stderr);
    }
    return NULL;
}

static int log_enqueue(LogLevel level, const char* file, int line, const char* fmt, ...) {
    char buffer[LOG_MSG_MAXLEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    pthread_mutex_lock(&log_queue.mutex);
    int next_tail = (log_queue.tail + 1) % LOG_QUEUE_SIZE;
    if (next_tail == log_queue.head) {
        // Черга повна — пропускаємо повідомлення
        pthread_mutex_unlock(&log_queue.mutex);
        return -1;
    }

    strncpy(log_queue.messages[log_queue.tail], buffer, LOG_MSG_MAXLEN - 1);
    log_queue.messages[log_queue.tail][LOG_MSG_MAXLEN - 1] = '\0';
    log_queue.levels[log_queue.tail] = level;
    log_queue.files[log_queue.tail] = file;
    log_queue.lines[log_queue.tail] = line;

    log_queue.tail = next_tail;
    pthread_cond_signal(&log_queue.cond);
    pthread_mutex_unlock(&log_queue.mutex);

    return 0;
}

static void log_fatal_error(const char* file, int line, const char* fmt, ...) {
    char buffer[512];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Синхронно виводимо помилку, щоб бути впевненим, що логи не загубляться
    fprintf(stderr, "[FATAL] %s:%d: %s\n", file, line, buffer);

#ifdef __unix__
    if (errno != 0) {
        fprintf(stderr, "errno: %d (%s)\n", errno, strerror(errno));
    }

    void *buffer_bt[32];
    int nptrs = backtrace(buffer_bt, 32);
    char** symbols = backtrace_symbols(buffer_bt, nptrs);

    if (symbols) {
        fprintf(stderr, "Stack trace:\n");
        for (int i = 0; i < nptrs; i++) {
            fprintf(stderr, "  %s\n", symbols[i]);
        }
        free(symbols);
    }
#endif
}

static void log_init(void) {
    memset(&log_queue, 0, sizeof(log_queue));
    pthread_mutex_init(&log_queue.mutex, NULL);
    pthread_cond_init(&log_queue.cond, NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, log_thread_func, NULL);
    pthread_detach(tid);
}

#define LOG_DEBUG(fmt, ...) log_enqueue(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_enqueue(LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_enqueue(LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_fatal_error(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

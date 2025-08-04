#pragma once

#ifdef __unix__
#include <execinfo.h>
#include <errno.h>
#include <string.h>
#endif

#define LOG_INFO(fmt)                                                     \
    log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, "%s", fmt)
#define LOG_WARN(fmt)                                                     \
    log_message(LOG_LEVEL_WARN, __FILE__, __LINE__, "%s", fmt)
#define LOG_ERROR(fmt)                                                    \
    log_fatal_error(__FILE__, __LINE__, fmt)
#define LOG_INFOF(fmt, ...)                                                    \
    log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...)                                                    \
log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERRORF(fmt, ...)                                                   \
log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

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

static void log_message(const LogLevel level, const char* file, const int line, const char* fmt, ...) {
    const time_t t = time(NULL);
    const struct tm* lt = localtime(&t);

    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", lt);

    set_console_color(level);

    fprintf(stderr, "[%s] [%s] %s:%d: ", time_buf, log_level_str[level], file, line);

    va_list args = {};
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    reset_console_color();
}

static void log_fatal_error(const char* file, const int line, const char* msg) {
    log_message(LOG_LEVEL_ERROR, file, line, "%s", msg);

#ifdef __unix__
    // errno
    if (errno != 0) {
        log_message(LOG_LEVEL_ERROR, file, line, "errno: %d (%s)", errno, strerror(errno));
    }

    // backtrace
    void *buffer[32];
    const int nptrs = backtrace(buffer, 32);
    char** symbols = backtrace_symbols(buffer, nptrs);

    if (symbols) {
        fprintf(stderr, "Stack trace:\n");
        for (int i = 0; i < nptrs; i++) {
            fprintf(stderr, "  %s\n", symbols[i]);
        }
        free(symbols);
    }
#endif
}


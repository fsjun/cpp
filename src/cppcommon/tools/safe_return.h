#pragma once

#define SAFE_RETURN(condition, ret, format, ...) \
    if (condition < 0) {                         \
        ERR(format, ##__VA_ARGS__);              \
        return ret;                              \
    }

#define SAFE_RETURN_VOID(condition, format, ...) \
    if (condition) {                             \
        ERR(format, ##__VA_ARGS__);              \
        return;                                  \
    }

#define SAFE_GOTO_END(condition, format, ...) \
    if (condition) {                          \
        ERR(format, ##__VA_ARGS__);           \
        goto end;                             \
    }

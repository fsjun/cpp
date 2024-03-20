#pragma once

#define SAFE_RETURN(condition, ret, format, ...) SAFE_ERROR_CONDITION(condition, return ret, ERR, format, ##__VA_ARGS__)
#define SAFE_RETURN_VOID(condition, format, ...) SAFE_ERROR_CONDITION(condition, return, ERR, format, ##__VA_ARGS__)
#define SAFE_GOTO_END(condition, format, ...) SAFE_ERROR_CONDITION(condition, goto end, ERR, format, ##__VA_ARGS__)
#define SAFE_RETURN_LN(condition, ret, format, ...) SAFE_ERROR_CONDITION(condition, return ret, ERRLN, format, ##__VA_ARGS__)
#define SAFE_RETURN_VOID_LN(condition, format, ...) SAFE_ERROR_CONDITION(condition, return, ERRLN, format, ##__VA_ARGS__)
#define SAFE_GOTO_END_LN(condition, format, ...) SAFE_ERROR_CONDITION(condition, goto end, ERRLN, format, ##__VA_ARGS__)

#define SAFE_ERROR_CONDITION(condition, retexpr, log, format, ...) \
    if (condition) {                                               \
        log(format, ##__VA_ARGS__);                                \
        retexpr;                                                   \
    }

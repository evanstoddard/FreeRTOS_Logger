/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_core.h
 * @author Evan Stoddard
 * @brief Core logging system - main API
 */

#ifndef log_core_h
#define log_core_h

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "log_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#define LOG_LEVEL_EMPTY_STR ""
#define LOG_LEVEL_ERROR_STR "ERR"
#define LOG_LEVEL_WARNING_STR "WRN"
#define LOG_LEVEL_INFO_STR "INF"
#define LOG_LEVEL_DEBUG_STR "DBG"

#define LOG_LEVEL_ERROR_COLOR "\e[31m"
#define LOG_LEVEL_WARNING_COLOR "\e[33m"
#define LOG_LEVEL_INFO_COLOR "\e[37m"
#define LOG_LEVEL_DEBUG_COLOR "\e[34m"

#define LOG_RESET_COLOR "\e[0m"

/*****************************************************************************
 * Log Implementation Macros
 *****************************************************************************/

#define LOG_AUGMENT_FMT_STR(fmt_str)                                           \
  "%s[%u] <%s> %s::%s: " fmt_str LOG_RESET_COLOR "\r\n"

#define LOG_IMPL(level, fmt_str, ...)                                          \
  do {                                                                         \
    char *level_str = LOG_LEVEL_EMPTY_STR;                                     \
    char *color_mod = LOG_LEVEL_EMPTY_STR;                                     \
    TickType_t ticks;                                                          \
    switch (level) {                                                           \
    case LOG_LEVEL_ERROR:                                                      \
      level_str = LOG_LEVEL_ERROR_STR;                                         \
      color_mod = LOG_LEVEL_ERROR_COLOR;                                       \
      break;                                                                   \
    case LOG_LEVEL_WARNING:                                                    \
      level_str = LOG_LEVEL_WARNING_STR;                                       \
      color_mod = LOG_LEVEL_WARNING_COLOR;                                     \
      break;                                                                   \
    case LOG_LEVEL_INFO:                                                       \
      level_str = LOG_LEVEL_INFO_STR;                                          \
      color_mod = LOG_LEVEL_INFO_COLOR;                                        \
      break;                                                                   \
    case LOG_LEVEL_DEBUG:                                                      \
      level_str = LOG_LEVEL_DEBUG_STR;                                         \
      color_mod = LOG_LEVEL_DEBUG_COLOR;                                       \
      break;                                                                   \
    default:                                                                   \
      break;                                                                   \
    }                                                                          \
    if (xPortIsInsideInterrupt()) {                                            \
      ticks = xTaskGetTickCountFromISR();                                      \
    } else {                                                                   \
      ticks = xTaskGetTickCount();                                             \
    }                                                                          \
    log_queue_deferred_message(prv_log_module_name, __FUNCTION__, level,       \
                               LOG_AUGMENT_FMT_STR(fmt_str), color_mod, ticks, \
                               level_str, prv_log_module_name, __FUNCTION__,   \
                               ##__VA_ARGS__);                                 \
  } while (0);

/*****************************************************************************
 * Inline Function
 *****************************************************************************/

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Initialize logging system
 *
 * @return 0 on success, non-zero on error
 */
int log_init(void);

/**
 * @brief Start logging thread
 *
 * @return 0 on success, non-zero on error
 */
int log_start_thread(void);

/**
 * @brief Queue deferred log message (thread-safe)
 *
 * @param module_name Module name
 * @param function_name Function name
 * @param level Log level
 * @param fmt_str Format string
 * @param ... Variable arguments
 * @return 0 on success, non-zero on error
 */
int log_queue_deferred_message(const char *module_name,
                               const char *function_name, uint8_t level,
                               const char *fmt_str, ...);

/**
 * @brief Queue deferred log message from ISR
 *
 * @param module_name Module name
 * @param function_name Function name
 * @param level Log level
 * @param fmt_str Format string
 * @param ... Variable arguments
 * @return 0 on success, non-zero on error
 */
int log_queue_deferred_message_isr(const char *module_name,
                                   const char *function_name, uint8_t level,
                                   const char *fmt_str, ...);

#ifdef __cplusplus
}
#endif
#endif /* log_core_h */

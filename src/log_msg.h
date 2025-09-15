/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_msg.h
 * @author Evan Stoddard
 * @brief Log message structure definition
 */

#ifndef log_msg_h
#define log_msg_h

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Structs, Unions, Enums, & Typedefs
 *****************************************************************************/

/**
 * @brief Log message structure with variable-length args buffer
 */
typedef struct log_msg_t {
  const char *module_name;
  const char *function_name;
  uint8_t log_level;
  const char *fmt_str;
  size_t args_buffer_size;
  uint8_t args_buffer[];  // Variable length array (C99 flexible array member)
} log_msg_t;

/**
 * @brief Calculate the total size needed for a log message
 *
 * @param args_size Size of the arguments buffer
 * @return Total size in bytes for the log message
 */
#define LOG_MSG_SIZE(args_size) (sizeof(log_msg_t) + (args_size))

#ifdef __cplusplus
}
#endif
#endif /* log_msg_h */
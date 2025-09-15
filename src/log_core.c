/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_core.c
 * @author Evan Stoddard
 * @brief Core logging system implementation
 */

#include "log_core.h"

#include <errno.h>
#include <stdarg.h>

#include "log_format.h"
#include "log_pool.h"
#include "log_queue.h"

/*****************************************************************************
 * Functions
 *****************************************************************************/

int log_init(void) {
  // Initialize buffer pool
  if (log_pool_init() != 0) {
    return -1;
  }

  // Initialize queue system
  if (log_queue_init() != 0) {
    return -2;
  }

  log_start_thread();

  return 0;
}

int log_start_thread(void) { return log_queue_start_thread(); }

int log_queue_deferred_message(const char *module_name,
                               const char *function_name, uint8_t level,
                               const char *fmt_str, ...) {
  if (fmt_str == NULL) {
    return -EINVAL;
  }

  va_list args;
  va_start(args, fmt_str);

  // Calculate buffer size needed for arguments
  size_t args_buffer_size = log_format_calculate_buffer_size(fmt_str);

  // Allocate log message from buffer pool
  log_msg_t *msg = log_pool_alloc(args_buffer_size);
  if (msg == NULL) {
    va_end(args);
    return -ENOSPC; // Out of buffer space
  }

  // Populate the log message metadata
  msg->module_name = module_name;
  msg->function_name = function_name;
  msg->log_level = level;
  msg->fmt_str = fmt_str;

  // Copy va_list arguments into the message's args buffer (if any)
  if (args_buffer_size > 0) {
    size_t bytes_written = log_format_copy_args_to_buffer(
        msg->args_buffer, args_buffer_size, fmt_str, args);

    if (bytes_written == 0) {
      log_pool_free(msg);
      va_end(args);
      return -EIO;
    }
  }

  va_end(args);

  return log_queue_send(msg);
}

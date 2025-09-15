/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_queue.h
 * @author Evan Stoddard
 * @brief Thread-safe queue management for deferred logging
 */

#ifndef log_queue_h
#define log_queue_h

#include "log_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Initialize the logging queue system
 *
 * @return 0 on success, non-zero on error
 */
int log_queue_init(void);

/**
 * @brief Start the logging thread
 *
 * @return 0 on success, non-zero on error
 */
int log_queue_start_thread(void);

/**
 * @brief Send a log message to the queue (thread-safe)
 *
 * @param msg Log message to queue
 * @return 0 on success, non-zero on error
 */
int log_queue_send(log_msg_t *msg);

/**
 * @brief Process a log message immediately (fallback when no threading)
 *
 * @param msg Log message to process
 */
void log_queue_process_immediate(log_msg_t *msg);

#ifdef __cplusplus
}
#endif
#endif /* log_queue_h */

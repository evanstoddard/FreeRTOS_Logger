/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_pool.h
 * @author Evan Stoddard
 * @brief Memory pool management for log messages
 */

#ifndef log_pool_h
#define log_pool_h

#include <stddef.h>
#include "log_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Initialize the log message buffer pool
 *
 * @return 0 on success, non-zero on error
 */
int log_pool_init(void);

/**
 * @brief Allocate a log message from the buffer pool
 *
 * @param args_size Size needed for arguments buffer
 * @return Allocated log message, or NULL if insufficient space
 */
log_msg_t *log_pool_alloc(size_t args_size);

/**
 * @brief Free a log message back to the buffer pool
 *
 * @param msg Log message to free
 */
void log_pool_free(log_msg_t *msg);

#ifdef __cplusplus
}
#endif
#endif /* log_pool_h */
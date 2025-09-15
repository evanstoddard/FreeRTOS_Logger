/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_config.h
 * @author Evan Stoddard
 * @brief Logging configuration
 */

#ifndef log_config_h
#define log_config_h

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Configuration
 *****************************************************************************/

/** @brief Total size of logging buffer pool in bytes */
#define LOG_BUFFER_SIZE_BYTES 1024

/** @brief Maximum number of log messages in queue */
#define LOG_QUEUE_SIZE 32

/** @brief Logging thread stack size */
#define LOG_THREAD_STACK_SIZE_BYTES 2048

/** @brief Logging thread priority */
#define LOG_THREAD_PRIORITY 2

#ifdef __cplusplus
}
#endif
#endif /* log_config_h */

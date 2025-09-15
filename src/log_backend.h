/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_backend.h
 * @author Evan Stoddard
 * @brief
 */

#include "log_msg.h"

#ifndef log_backend_h
#define log_backend_h

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Structs, Unions, Enums, & Typedefs
 *****************************************************************************/

/**
 * @class log_backend_t
 * @brief Forward declaration of log backend
 *
 */
struct log_backend_t;

/**
 * @typedef log_backend_api_t
 * @brief API for log backend
 *
 */
typedef struct log_backend_api_t {
  /**
   * @brief Pointer to process message
   * @param backend Pointer to backend instance
   * @param msg Pointer to message
   */
  void (*process_msg)(const struct log_backend_t *backend,
                      const log_msg_t *msg);
} log_backend_api_t;

/**
 * @typedef log_backend_t
 * @brief Log backend definition
 *
 */
typedef struct log_backend_t {
  log_backend_api_t api;
  struct log_backend_t *next;
} log_backend_t;

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Register backend with logging system
 *
 * @param backend Pointer to backend
 * @return Returns 0 on success
 */
int log_backend_register_backend(log_backend_t *backend);

/**
 * @brief Returns head of log backends linked list
 *
 * @return Pointer to first log backend
 */
log_backend_t *log_backend_get_head(void);

#ifdef __cplusplus
}
#endif
#endif /* log_backend_h */

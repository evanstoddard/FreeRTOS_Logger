/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log.h
 * @author Evan Stoddard
 * @brief
 */

#ifndef log_h
#define log_h

#include "log_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Definitions
 *****************************************************************************/

#define LOG_DBG(fmt_str, ...)                                                  \
  LOG_IMPL(LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG_COLOR fmt_str LOG_RESET_COLOR,     \
           ##__VA_ARGS__)

#define LOG_INF(fmt_str, ...)                                                  \
  LOG_IMPL(LOG_LEVEL_INFO, LOG_LEVEL_INFO_COLOR fmt_str LOG_RESET_COLOR,       \
           ##__VA_ARGS__)

#define LOG_WRN(fmt_str, ...)                                                  \
  LOG_IMPL(LOG_LEVEL_WARNING, LOG_LEVEL_WARNING_COLOR fmt_str LOG_RESET_COLOR, \
           ##__VA_ARGS__)

#define LOG_ERR(fmt_str, ...)                                                  \
  LOG_IMPL(LOG_LEVEL_ERROR, LOG_LEVEL_ERROR_COLOR fmt_str LOG_RESET_COLOR,     \
           ##__VA_ARGS__)

#define LOG_REGISTER_MODULE(module_name)                                       \
  static const char *prv_log_module_name = #module_name;

/*****************************************************************************
 * Structs, Unions, Enums, & Typedefs
 *****************************************************************************/

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* log_h */

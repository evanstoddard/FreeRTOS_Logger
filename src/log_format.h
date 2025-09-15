/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_format.h
 * @author Evan Stoddard
 * @brief Format string parsing and analysis
 */

#ifndef log_format_h
#define log_format_h

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Calculate required buffer size by parsing format string
 *
 * @param fmt_str Format string to parse
 * @return Size in bytes needed to buffer the va_list arguments
 */
size_t log_format_calculate_buffer_size(const char *fmt_str);

/**
 * @brief Copy va_list arguments into buffer based on format string
 *
 * @param buffer Destination buffer
 * @param buffer_size Size of destination buffer
 * @param fmt_str Format string
 * @param args Variable argument list
 * @return Number of bytes written to buffer, or 0 on error
 */
size_t log_format_copy_args_to_buffer(void *buffer, size_t buffer_size, const char *fmt_str, va_list args);

#ifdef __cplusplus
}
#endif
#endif /* log_format_h */
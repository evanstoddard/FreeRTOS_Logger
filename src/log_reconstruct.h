/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_reconstruct.h
 * @author Evan Stoddard
 * @brief VA list reconstruction for deferred logging
 */

#ifndef log_reconstruct_h
#define log_reconstruct_h

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

/**
 * @brief Print to output buffer
 *
 * @param fmt_str Pointer to format string
 * @param arg_buf Pointer to argument buffer
 * @param out_buf Pointer to output buffer
 * @param out_buf_size_bytes Size of output buffer
 * @return Returns size written to buffer
 */
size_t log_reconstruct_snprintf(const char *fmt_str, const void *arg_buf,
                                void *out_buf, size_t out_buf_size_bytes);

#ifdef __cplusplus
}
#endif
#endif /* log_reconstruct_h */

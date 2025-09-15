/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_reconstruct.c
 * @author Evan Stoddard
 * @brief VA list reconstruction for deferred logging implementation
 */

#include "log_reconstruct.h"

#include <stdarg.h>
#include <stdio.h>

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * @brief Reconstruct va_list from buffered va_list
 *
 * @param fmt_str Pointer to format string 
 * @param arg_buf Buffer of arg list
 * @param out_buf Pointer to output buffer 
 * @param out_buf_size_bytes Size of output buffer 
 * @return Returns size of bytes written 
 */
static size_t prv_vsnprintf_from_arg_buffer(const char *fmt_str,
                                            const void *arg_buf, void *out_buf,
                                            size_t out_buf_size_bytes) {
  if (fmt_str == NULL || arg_buf == NULL || out_buf == NULL) {
    return 0;
  }

#if defined(__aarch64__)
  // ARM64 - va_list is a complex structure
  struct __va_list {
    void *__stack;
    void *__gr_top;
    void *__vr_top;
    int __gr_offs;
    int __vr_offs;
  };

  union {
    va_list ap;
    struct __va_list __ap;
  } u;

  // Reconstruct va_list pointing to our buffer
  u.__ap.__stack = (void *)buffer;
  u.__ap.__gr_top = NULL;
  u.__ap.__vr_top = NULL;
  u.__ap.__gr_offs = 0;
  u.__ap.__vr_offs = 0;

  return vprintf(fmt_str, u.ap);

#else
  // Default: assume va_list is just a pointer (ARM32, many others)
  // This works for most simple architectures
  union {
    va_list ap;
    void *ptr;
  } u;

  u.ptr = (void *)arg_buf;
  return vsnprintf(out_buf, out_buf_size_bytes, fmt_str, u.ap);

#endif
}

/*****************************************************************************
* Public Functions
*****************************************************************************/

size_t log_reconstruct_snprintf(const char *fmt_str, const void *arg_buf,
                                void *out_buf, size_t out_buf_size_bytes) {

  return prv_vsnprintf_from_arg_buffer(fmt_str, arg_buf, out_buf,
                                       out_buf_size_bytes);
}


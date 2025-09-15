/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_format.c
 * @author Evan Stoddard
 * @brief Format string parsing and analysis implementation
 */

#include "log_format.h"

#include <stdint.h>

#include "log_backend.h"

/*****************************************************************************
 * Functions
 *****************************************************************************/

size_t log_format_calculate_buffer_size(const char *fmt_str) {
  if (!fmt_str)
    return 0;

  size_t buffer_size = 0;
  const char *p = fmt_str;

  while (*p) {
    if (*p == '%' && *(p + 1) != '%') {
      p++; // Skip '%'

      // Skip flags, width, precision
      while (*p &&
             (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0'))
        p++;
      while (*p && (*p >= '0' && *p <= '9'))
        p++;
      if (*p == '.') {
        p++;
        while (*p && (*p >= '0' && *p <= '9'))
          p++;
      }

      // Handle length modifiers and get the conversion specifier
      switch (*p) {
      case 'h':
        if (*(p + 1) == 'h') {
          p++;
          buffer_size += sizeof(int); // char promoted to int
        } else {
          buffer_size += sizeof(int); // short promoted to int
        }
        p++;
        break;
      case 'l':
        if (*(p + 1) == 'l') {
          p++;
          buffer_size += sizeof(long long);
        } else {
          buffer_size += sizeof(long);
        }
        p++;
        break;
      case 'z':
        buffer_size += sizeof(size_t);
        p++;
        break;
      case 't':
        buffer_size += sizeof(ptrdiff_t);
        p++;
        break;
      case 'j':
        buffer_size += sizeof(intmax_t);
        p++;
        break;
      default:
        // Handle conversion specifiers
        switch (*p) {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
        case 'c':
          buffer_size += sizeof(int);
          break;
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
          buffer_size += sizeof(double);
          break;
        case 's':
          buffer_size += sizeof(char *);
          break;
        case 'p':
          buffer_size += sizeof(void *);
          break;
        case 'n':
          buffer_size += sizeof(int *);
          break;
        }
        break;
      }
    } else if (*p == '%' && *(p + 1) == '%') {
      p++; // Skip first '%' of '%%'
    }
    p++;
  }

  return buffer_size;
}

size_t log_format_copy_args_to_buffer(void *buffer, size_t buffer_size,
                                      const char *fmt_str, va_list args) {
  if (!buffer || !fmt_str || buffer_size == 0)
    return 0;

  char *buf_ptr = (char *)buffer;
  size_t bytes_written = 0;
  const char *p = fmt_str;

  while (*p && bytes_written < buffer_size) {
    if (*p == '%' && *(p + 1) != '%') {
      p++; // Skip '%'

      // Skip flags, width, precision (same logic as size calculation)
      while (*p &&
             (*p == '-' || *p == '+' || *p == ' ' || *p == '#' || *p == '0'))
        p++;
      while (*p && (*p >= '0' && *p <= '9'))
        p++;
      if (*p == '.') {
        p++;
        while (*p && (*p >= '0' && *p <= '9'))
          p++;
      }

      // Extract and store arguments based on length modifiers
      switch (*p) {
      case 'h':
        if (*(p + 1) == 'h') {
          p++;
          // char promoted to int
          *(int *)(buf_ptr + bytes_written) = va_arg(args, int);
          bytes_written += sizeof(int);
        } else {
          // short promoted to int
          *(int *)(buf_ptr + bytes_written) = va_arg(args, int);
          bytes_written += sizeof(int);
        }
        p++;
        break;
      case 'l':
        if (*(p + 1) == 'l') {
          p++;
          *(long long *)(buf_ptr + bytes_written) = va_arg(args, long long);
          bytes_written += sizeof(long long);
        } else {
          *(long *)(buf_ptr + bytes_written) = va_arg(args, long);
          bytes_written += sizeof(long);
        }
        p++;
        break;
      case 'z':
        *(size_t *)(buf_ptr + bytes_written) = va_arg(args, size_t);
        bytes_written += sizeof(size_t);
        p++;
        break;
      case 't':
        *(ptrdiff_t *)(buf_ptr + bytes_written) = va_arg(args, ptrdiff_t);
        bytes_written += sizeof(ptrdiff_t);
        p++;
        break;
      case 'j':
        *(intmax_t *)(buf_ptr + bytes_written) = va_arg(args, intmax_t);
        bytes_written += sizeof(intmax_t);
        p++;
        break;
      default:
        // Handle conversion specifiers without length modifiers
        switch (*p) {
        case 'd':
        case 'i':
        case 'o':
        case 'u':
        case 'x':
        case 'X':
        case 'c':
          *(int *)(buf_ptr + bytes_written) = va_arg(args, int);
          bytes_written += sizeof(int);
          break;
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
          *(double *)(buf_ptr + bytes_written) = va_arg(args, double);
          bytes_written += sizeof(double);
          break;
        case 's':
          *(char **)(buf_ptr + bytes_written) = va_arg(args, char *);
          bytes_written += sizeof(char *);
          break;
        case 'p':
          *(void **)(buf_ptr + bytes_written) = va_arg(args, void *);
          bytes_written += sizeof(void *);
          break;
        case 'n':
          *(int **)(buf_ptr + bytes_written) = va_arg(args, int *);
          bytes_written += sizeof(int *);
          break;
        }
        break;
      }
    } else if (*p == '%' && *(p + 1) == '%') {
      p++; // Skip first '%' of '%%'
    }
    p++;
  }

  return bytes_written;
}

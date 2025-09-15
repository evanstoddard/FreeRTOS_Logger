/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_backend.c
 * @author Evan Stoddard
 * @brief
 */

#include "log_backend.h"

#include <errno.h>
#include <stddef.h>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Variables
 *****************************************************************************/

/**
 * @brief Private instance
 */
static struct {
  log_backend_t *head;
} prv_inst;

/*****************************************************************************
 * Functions
 *****************************************************************************/

int log_backend_register_backend(log_backend_t *backend) {
  if (backend == NULL) {
    return -EINVAL;
  }

  if (prv_inst.head == NULL) {
    prv_inst.head = backend;
    prv_inst.head->next = NULL;
  }

  log_backend_t *node = prv_inst.head;

  while (node) {
    if (node->next == NULL) {
      node->next = backend;
      backend->next = NULL;

      break;
    }

    node = node->next;
  }

  return 0;
}

log_backend_t *log_backend_get_head(void) { return prv_inst.head; }

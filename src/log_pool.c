/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_pool.c
 * @author Evan Stoddard
 * @brief Memory pool management for log messages implementation
 */

#include "log_pool.h"
#include "log_config.h"

#include <errno.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"

/*****************************************************************************
 * Variables
 *****************************************************************************/

// Buffer pool management
static uint8_t prv_log_buffer_pool[LOG_BUFFER_SIZE_BYTES];
static size_t prv_log_buffer_used = 0;

static SemaphoreHandle_t prv_log_pool_mutex = NULL;
static StaticSemaphore_t prv_log_mutex_storage;

/*****************************************************************************
 * Functions
 *****************************************************************************/

int log_pool_init(void) {
  // Create mutex for buffer pool protection using static allocation
  prv_log_pool_mutex = xSemaphoreCreateMutexStatic(&prv_log_mutex_storage);
  if (!prv_log_pool_mutex) {
    return -EIO;
  }

  // Initialize buffer pool
  prv_log_buffer_used = 0;

  return 0;
}

log_msg_t *log_pool_alloc(size_t args_size) {
  size_t total_size = LOG_MSG_SIZE(args_size);

  if (prv_log_pool_mutex == NULL) {
    return NULL;
  }

  BaseType_t ret = pdFALSE;
  BaseType_t higher_prio = pdFALSE;

  // Thread-safe allocation using FreeRTOS mutex
  if (xPortIsInsideInterrupt()) {
    ret = xSemaphoreTakeFromISR(prv_log_pool_mutex, &higher_prio);
  } else {
    ret = xSemaphoreTake(prv_log_pool_mutex, portMAX_DELAY);
  }

  if (ret == pdFALSE) {
    if (xPortIsInsideInterrupt()) {
      portYIELD_FROM_ISR(higher_prio);
    }

    return NULL;
  }

  // Simple linear allocator - in production, use proper memory management
  if (prv_log_buffer_used + total_size > LOG_BUFFER_SIZE_BYTES) {
    if (xPortIsInsideInterrupt()) {
      xSemaphoreGiveFromISR(prv_log_pool_mutex, &higher_prio);
      portYIELD_FROM_ISR(higher_prio);
    } else {
      xSemaphoreGive(prv_log_pool_mutex);
    }

    return NULL; // Out of space
  }

  log_msg_t *msg = (log_msg_t *)(prv_log_buffer_pool + prv_log_buffer_used);
  prv_log_buffer_used += total_size;

  // Initialize the message
  msg->args_buffer_size = args_size;

  if (xPortIsInsideInterrupt()) {
    xSemaphoreGiveFromISR(prv_log_pool_mutex, &higher_prio);
    portYIELD_FROM_ISR(higher_prio);
  } else {
    xSemaphoreGive(prv_log_pool_mutex);
  }

  return msg;
}

void log_pool_free(log_msg_t *msg) {
  if (msg == NULL) {
    return;
  }

  if (prv_log_pool_mutex == NULL) {
    return;
  }

  BaseType_t ret = pdFALSE;
  BaseType_t higher_prio = pdFALSE;

  if (xPortIsInsideInterrupt()) {
    ret = xSemaphoreTakeFromISR(prv_log_pool_mutex, &higher_prio);
  } else {
    ret = xSemaphoreTake(prv_log_pool_mutex, portMAX_DELAY);
  }

  if (ret == pdFALSE) {
    if (xPortIsInsideInterrupt()) {
      portYIELD_FROM_ISR(higher_prio);
    }

    return;
  }

  if ((uint8_t *)msg + LOG_MSG_SIZE(msg->args_buffer_size) ==
      prv_log_buffer_pool + prv_log_buffer_used) {
    prv_log_buffer_used -= LOG_MSG_SIZE(msg->args_buffer_size);
  }

  if (xPortIsInsideInterrupt()) {
    xSemaphoreGiveFromISR(prv_log_pool_mutex, &higher_prio);
    portYIELD_FROM_ISR(higher_prio);
  } else {
    xSemaphoreGive(prv_log_pool_mutex);
  }
}

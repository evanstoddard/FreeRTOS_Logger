/*
 * Copyright (C) Evan Stoddard
 */

/**
 * @file log_queue.c
 * @author Evan Stoddard
 * @brief Thread-safe queue management for deferred logging implementation
 */

#include "log_queue.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "log_backend.h"
#include "log_config.h"
#include "log_pool.h"

/*****************************************************************************
 * Variables
 *****************************************************************************/

// FreeRTOS handles
static QueueHandle_t prv_log_queue = NULL;
static TaskHandle_t prv_log_task_handle = NULL;

// Static storage for FreeRTOS objects
static StaticQueue_t prv_log_queue_storage;
static log_msg_t *prv_log_queue_buffer[LOG_QUEUE_SIZE];
static StaticTask_t prv_log_task_storage;
static StackType_t
    prv_log_task_stack[LOG_THREAD_STACK_SIZE_BYTES / sizeof(StackType_t)];

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

/**
 * @brief Logging thread
 *
 * @param args Unused params
 */
static void prv_log_thread_task(void *args) {
  log_msg_t *msg;

  while (true) {
    if (xQueueReceive(prv_log_queue, &msg, portMAX_DELAY) == pdTRUE) {
      log_queue_process_immediate(msg);
    }
  }
}

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

int log_queue_init(void) {
  // Create queue for log message pointers using static allocation
  prv_log_queue = xQueueCreateStatic(LOG_QUEUE_SIZE, sizeof(log_msg_t *),
                                     (uint8_t *)prv_log_queue_buffer,
                                     &prv_log_queue_storage);
  if (!prv_log_queue) {
    return -EIO;
  }

  return 0;
}

int log_queue_start_thread(void) {
  if (!prv_log_queue) {
    return -EIO; // Not initialized
  }

  prv_log_task_handle = xTaskCreateStatic(
      prv_log_thread_task, "LogThread",
      LOG_THREAD_STACK_SIZE_BYTES / sizeof(StackType_t), NULL,
      LOG_THREAD_PRIORITY, prv_log_task_stack, &prv_log_task_storage);

  return (prv_log_task_handle != NULL) ? 0 : -EIO;
}

int log_queue_send(log_msg_t *msg) {
  if (msg == NULL) {
    return -EINVAL;
  }

  if (prv_log_queue == NULL) {
    return -EIO;
  }

  BaseType_t ret = pdFALSE;
  BaseType_t higher_prio = pdFALSE;

  if (xPortIsInsideInterrupt()) {
    ret = xQueueSendFromISR(prv_log_queue, &msg, &higher_prio);
  } else {
    ret = xQueueSend(prv_log_queue, &msg, 0);
  }

  if (xPortIsInsideInterrupt()) {
    portYIELD_FROM_ISR(higher_prio);
  }

  return (ret == pdTRUE ? 0 : -ENOSPC);
}

void log_queue_process_immediate(log_msg_t *msg) {
  if (!msg)
    return;

  log_backend_t *backend = log_backend_get_head();

  while (backend) {
    if (backend->api.process_msg == NULL) {
      backend = backend->next;
      continue;
    }

    backend->api.process_msg(backend, msg);
    backend = backend->next;
  }

  // Free the message back to pool
  log_pool_free(msg);
}

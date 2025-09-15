# Creating Custom Log Backends

This guide explains how to create custom backends for the FreeRTOS logging system. Backends are responsible for outputting log messages to various destinations such as UART, files, network, or custom displays.

## Table of Contents
- [Backend Architecture](#backend-architecture)
- [Basic Backend Implementation](#basic-backend-implementation)
- [Example Backends](#example-backends)
  - [UART Backend](#uart-backend)
  - [File Backend](#file-backend)
  - [Network Backend](#network-backend)
  - [Ring Buffer Backend](#ring-buffer-backend)
- [Message Processing](#message-processing)
- [Backend Registration](#backend-registration)
- [Advanced Features](#advanced-features)
- [Best Practices](#best-practices)

## Backend Architecture

A log backend consists of two main components:

1. **Backend Structure** (`log_backend_t`): Contains the API functions and linked list pointer
2. **API Functions** (`log_backend_api_t`): Function pointers for message processing

```c
#include "log_backend.h"

// Backend API structure
typedef struct log_backend_api_t {
    void (*process_msg)(const struct log_backend_t *backend,
                       const log_msg_t *msg);
} log_backend_api_t;

// Backend structure
typedef struct log_backend_t {
    log_backend_api_t api;
    struct log_backend_t *next;
} log_backend_t;
```

## Basic Backend Implementation

Here's the minimal structure for a custom backend:

```c
#include "log_backend.h"
#include "log_format.h"
#include <stdio.h>

// Backend-specific data (optional)
typedef struct {
    log_backend_t backend;  // Must be first member
    // Add your backend-specific fields here
    int custom_field;
    void *custom_buffer;
} my_custom_backend_t;

// Message processing function
static void my_backend_process_msg(const log_backend_t *backend,
                                   const log_msg_t *msg) {
    // Format the message
    static char buffer[256];
    log_format_message(msg, buffer, sizeof(buffer));

    // Output the formatted message (customize this part)
    printf("%s", buffer);
}

// Backend instance
static my_custom_backend_t my_backend_instance = {
    .backend = {
        .api = {
            .process_msg = my_backend_process_msg,
        },
    },
    .custom_field = 0,
};

// Initialization function
void my_backend_init(void) {
    // Perform any backend-specific initialization

    // Register the backend
    log_backend_register_backend(&my_backend_instance.backend);
}
```

## Example Backends

### UART Backend

A backend that outputs logs via UART:

```c
#include "log_backend.h"
#include "log_format.h"
#include "uart_driver.h"  // Your UART driver

typedef struct {
    log_backend_t backend;
    UART_HandleTypeDef *uart_handle;
    uint32_t baud_rate;
    SemaphoreHandle_t tx_mutex;
} uart_backend_t;

static void uart_backend_process_msg(const log_backend_t *backend,
                                     const log_msg_t *msg) {
    uart_backend_t *uart_backend = (uart_backend_t *)backend;
    static char buffer[512];

    // Format the message
    size_t len = log_format_message(msg, buffer, sizeof(buffer));

    // Thread-safe UART transmission
    if (xSemaphoreTake(uart_backend->tx_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uart_transmit(uart_backend->uart_handle, (uint8_t *)buffer, len);
        xSemaphoreGive(uart_backend->tx_mutex);
    }
}

static uart_backend_t uart_backend_instance = {
    .backend = {
        .api = {
            .process_msg = uart_backend_process_msg,
        },
    },
};

int uart_backend_init(UART_HandleTypeDef *uart, uint32_t baud_rate) {
    uart_backend_instance.uart_handle = uart;
    uart_backend_instance.baud_rate = baud_rate;

    // Create mutex for thread-safe access
    uart_backend_instance.tx_mutex = xSemaphoreCreateMutex();
    if (!uart_backend_instance.tx_mutex) {
        return -1;
    }

    // Initialize UART hardware
    uart_init(uart, baud_rate);

    // Register backend
    return log_backend_register_backend(&uart_backend_instance.backend);
}
```

### File Backend

A backend that writes logs to a file (SD card or flash):

```c
#include "log_backend.h"
#include "log_format.h"
#include "ff.h"  // FatFS or your filesystem

typedef struct {
    log_backend_t backend;
    FIL log_file;
    char filename[32];
    SemaphoreHandle_t file_mutex;
    uint32_t max_file_size;
    uint32_t current_size;
} file_backend_t;

static void file_backend_process_msg(const log_backend_t *backend,
                                     const log_msg_t *msg) {
    file_backend_t *file_backend = (file_backend_t *)backend;
    static char buffer[512];

    // Format the message
    size_t len = log_format_message(msg, buffer, sizeof(buffer));

    // Thread-safe file write
    if (xSemaphoreTake(file_backend->file_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        UINT bytes_written;

        // Check for file rotation
        if (file_backend->current_size + len > file_backend->max_file_size) {
            file_backend_rotate(file_backend);
        }

        // Write to file
        f_write(&file_backend->log_file, buffer, len, &bytes_written);
        f_sync(&file_backend->log_file);

        file_backend->current_size += bytes_written;
        xSemaphoreGive(file_backend->file_mutex);
    }
}

static void file_backend_rotate(file_backend_t *backend) {
    // Close current file
    f_close(&backend->log_file);

    // Generate new filename with timestamp
    char new_filename[32];
    generate_timestamped_filename(new_filename, sizeof(new_filename));

    // Open new file
    f_open(&backend->log_file, new_filename, FA_WRITE | FA_CREATE_ALWAYS);
    backend->current_size = 0;
}

static file_backend_t file_backend_instance = {
    .backend = {
        .api = {
            .process_msg = file_backend_process_msg,
        },
    },
    .max_file_size = 1024 * 1024,  // 1MB per file
};

int file_backend_init(const char *log_dir) {
    // Create mutex
    file_backend_instance.file_mutex = xSemaphoreCreateMutex();
    if (!file_backend_instance.file_mutex) {
        return -1;
    }

    // Create initial log file
    snprintf(file_backend_instance.filename,
             sizeof(file_backend_instance.filename),
             "%s/log.txt", log_dir);

    FRESULT res = f_open(&file_backend_instance.log_file,
                         file_backend_instance.filename,
                         FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        return -1;
    }

    // Register backend
    return log_backend_register_backend(&file_backend_instance.backend);
}
```

### Network Backend

A backend that sends logs over network (TCP/UDP):

```c
#include "log_backend.h"
#include "log_format.h"
#include "lwip/sockets.h"  // LwIP stack

typedef struct {
    log_backend_t backend;
    int socket_fd;
    struct sockaddr_in server_addr;
    QueueHandle_t tx_queue;
    TaskHandle_t tx_task;
    bool connected;
} network_backend_t;

typedef struct {
    char message[512];
    size_t length;
} network_log_msg_t;

static void network_backend_process_msg(const log_backend_t *backend,
                                        const log_msg_t *msg) {
    network_backend_t *net_backend = (network_backend_t *)backend;

    if (!net_backend->connected) {
        return;  // Skip if not connected
    }

    // Format message
    network_log_msg_t net_msg;
    net_msg.length = log_format_message(msg, net_msg.message,
                                        sizeof(net_msg.message));

    // Queue for transmission (non-blocking)
    xQueueSend(net_backend->tx_queue, &net_msg, 0);
}

static void network_tx_task(void *pvParameters) {
    network_backend_t *backend = (network_backend_t *)pvParameters;
    network_log_msg_t msg;

    while (1) {
        // Wait for messages
        if (xQueueReceive(backend->tx_queue, &msg, portMAX_DELAY) == pdTRUE) {
            // Attempt to send
            if (backend->connected) {
                int sent = send(backend->socket_fd, msg.message,
                               msg.length, MSG_DONTWAIT);
                if (sent < 0) {
                    // Handle connection loss
                    backend->connected = false;
                    network_reconnect(backend);
                }
            }
        }
    }
}

static void network_reconnect(network_backend_t *backend) {
    // Close old socket
    if (backend->socket_fd >= 0) {
        close(backend->socket_fd);
    }

    // Create new socket
    backend->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (backend->socket_fd < 0) {
        return;
    }

    // Attempt to connect
    if (connect(backend->socket_fd, (struct sockaddr *)&backend->server_addr,
                sizeof(backend->server_addr)) == 0) {
        backend->connected = true;
    }
}

static network_backend_t network_backend_instance = {
    .backend = {
        .api = {
            .process_msg = network_backend_process_msg,
        },
    },
    .socket_fd = -1,
    .connected = false,
};

int network_backend_init(const char *server_ip, uint16_t port) {
    // Setup server address
    network_backend_instance.server_addr.sin_family = AF_INET;
    network_backend_instance.server_addr.sin_port = htons(port);
    inet_aton(server_ip, &network_backend_instance.server_addr.sin_addr);

    // Create transmission queue
    network_backend_instance.tx_queue = xQueueCreate(100,
                                                     sizeof(network_log_msg_t));
    if (!network_backend_instance.tx_queue) {
        return -1;
    }

    // Create transmission task
    xTaskCreate(network_tx_task, "LogNetTx", 2048,
                &network_backend_instance, 2,
                &network_backend_instance.tx_task);

    // Initial connection attempt
    network_reconnect(&network_backend_instance);

    // Register backend
    return log_backend_register_backend(&network_backend_instance.backend);
}
```

### Ring Buffer Backend

A backend that stores logs in a circular buffer for post-mortem analysis:

```c
#include "log_backend.h"
#include "log_format.h"
#include <string.h>

#define RING_BUFFER_SIZE (64 * 1024)  // 64KB

typedef struct {
    log_backend_t backend;
    uint8_t buffer[RING_BUFFER_SIZE];
    uint32_t write_pos;
    uint32_t read_pos;
    SemaphoreHandle_t mutex;
    bool overflow;
} ringbuf_backend_t;

static void ringbuf_backend_process_msg(const log_backend_t *backend,
                                        const log_msg_t *msg) {
    ringbuf_backend_t *rb_backend = (ringbuf_backend_t *)backend;
    static char temp_buffer[512];

    // Format message
    size_t msg_len = log_format_message(msg, temp_buffer, sizeof(temp_buffer));

    // Write to ring buffer with mutex protection
    if (xSemaphoreTake(rb_backend->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Calculate available space
        uint32_t available = RING_BUFFER_SIZE - rb_backend->write_pos;

        if (msg_len <= available) {
            // Message fits without wrapping
            memcpy(&rb_backend->buffer[rb_backend->write_pos],
                   temp_buffer, msg_len);
            rb_backend->write_pos += msg_len;
        } else {
            // Message wraps around
            memcpy(&rb_backend->buffer[rb_backend->write_pos],
                   temp_buffer, available);
            memcpy(&rb_backend->buffer[0],
                   temp_buffer + available, msg_len - available);
            rb_backend->write_pos = msg_len - available;
            rb_backend->overflow = true;
        }

        // Handle wrap-around
        if (rb_backend->write_pos >= RING_BUFFER_SIZE) {
            rb_backend->write_pos = 0;
            rb_backend->overflow = true;
        }

        xSemaphoreGive(rb_backend->mutex);
    }
}

// Function to dump ring buffer contents (for debugging)
int ringbuf_backend_dump(uint8_t *output, size_t output_size) {
    ringbuf_backend_t *backend = &ringbuf_backend_instance;

    if (xSemaphoreTake(backend->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return -1;
    }

    size_t bytes_to_copy;
    if (backend->overflow) {
        // Buffer has wrapped, start from write_pos (oldest data)
        bytes_to_copy = RING_BUFFER_SIZE;
    } else {
        // Buffer hasn't wrapped, copy from start to write_pos
        bytes_to_copy = backend->write_pos;
    }

    if (bytes_to_copy > output_size) {
        bytes_to_copy = output_size;
    }

    if (backend->overflow) {
        // Copy from write_pos to end, then from start to write_pos
        size_t first_part = RING_BUFFER_SIZE - backend->write_pos;
        memcpy(output, &backend->buffer[backend->write_pos], first_part);
        memcpy(output + first_part, &backend->buffer[0],
               bytes_to_copy - first_part);
    } else {
        memcpy(output, backend->buffer, bytes_to_copy);
    }

    xSemaphoreGive(backend->mutex);
    return bytes_to_copy;
}

static ringbuf_backend_t ringbuf_backend_instance = {
    .backend = {
        .api = {
            .process_msg = ringbuf_backend_process_msg,
        },
    },
    .write_pos = 0,
    .read_pos = 0,
    .overflow = false,
};

int ringbuf_backend_init(void) {
    // Create mutex
    ringbuf_backend_instance.mutex = xSemaphoreCreateMutex();
    if (!ringbuf_backend_instance.mutex) {
        return -1;
    }

    // Clear buffer
    memset(ringbuf_backend_instance.buffer, 0, RING_BUFFER_SIZE);

    // Register backend
    return log_backend_register_backend(&ringbuf_backend_instance.backend);
}
```

## Message Processing

The `log_msg_t` structure contains all the information about a log message:

```c
#include "log_msg.h"
#include "log_format.h"
#include "log_reconstruct.h"

static void advanced_process_msg(const log_backend_t *backend,
                                 const log_msg_t *msg) {
    // Access message fields directly
    uint8_t level = msg->level;
    uint32_t timestamp = msg->timestamp;
    const char *module = msg->module_name;
    const char *function = msg->function_name;

    // Option 1: Use the formatting helper
    char formatted[512];
    log_format_message(msg, formatted, sizeof(formatted));

    // Option 2: Reconstruct the original message
    char reconstructed[512];
    log_reconstruct_message(msg, reconstructed, sizeof(reconstructed));

    // Option 3: Custom formatting
    char custom[512];
    snprintf(custom, sizeof(custom), "[%u][%s] %s::%s - ",
             timestamp, get_level_string(level), module, function);

    // Append the actual message content
    strncat(custom, reconstructed, sizeof(custom) - strlen(custom) - 1);

    // Output using your backend's method
    output_message(custom);
}

static const char* get_level_string(uint8_t level) {
    switch(level) {
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARNING: return "WARN";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        default:                return "UNKNOWN";
    }
}
```

## Backend Registration

Backends must be registered with the logging system to receive messages:

```c
// Single backend registration
void init_logging_system(void) {
    // Initialize logging core
    log_init();

    // Initialize and register backends
    uart_backend_init(UART1, 115200);
    file_backend_init("/logs");
    network_backend_init("192.168.1.100", 5000);
    ringbuf_backend_init();

    // Start the logging thread
    log_start_thread();
}

// Multiple backends can be registered
void setup_debug_logging(void) {
    // Console output for development
    console_backend_init();

    // File logging for persistent storage
    file_backend_init("/debug");

    // Network logging for remote monitoring
    network_backend_init("logger.example.com", 514);
}
```

## Advanced Features

### Filtered Backend

A backend that only processes certain log levels:

```c
typedef struct {
    log_backend_t backend;
    uint8_t min_level;
    uint8_t max_level;
    log_backend_t *target_backend;  // Delegate to another backend
} filter_backend_t;

static void filter_backend_process_msg(const log_backend_t *backend,
                                       const log_msg_t *msg) {
    filter_backend_t *filter = (filter_backend_t *)backend;

    // Check if message passes filter
    if (msg->level >= filter->min_level &&
        msg->level <= filter->max_level) {
        // Forward to target backend
        filter->target_backend->api.process_msg(filter->target_backend, msg);
    }
}

// Example: Only errors go to network
filter_backend_t error_only_network = {
    .backend = {
        .api = {
            .process_msg = filter_backend_process_msg,
        },
    },
    .min_level = LOG_LEVEL_ERROR,
    .max_level = LOG_LEVEL_ERROR,
    .target_backend = &network_backend_instance.backend,
};
```

### Async Backend with Buffering

A backend that buffers messages for batch processing:

```c
typedef struct {
    log_backend_t backend;
    QueueHandle_t msg_queue;
    TaskHandle_t processor_task;
    uint32_t batch_size;
    uint32_t batch_timeout_ms;
} async_backend_t;

static void async_backend_process_msg(const log_backend_t *backend,
                                      const log_msg_t *msg) {
    async_backend_t *async = (async_backend_t *)backend;

    // Copy message to queue (non-blocking)
    log_msg_t *msg_copy = pvPortMalloc(sizeof(log_msg_t));
    if (msg_copy) {
        memcpy(msg_copy, msg, sizeof(log_msg_t));
        xQueueSend(async->msg_queue, &msg_copy, 0);
    }
}

static void async_processor_task(void *pvParameters) {
    async_backend_t *backend = (async_backend_t *)pvParameters;
    log_msg_t *messages[32];
    uint32_t msg_count = 0;
    TickType_t last_flush = xTaskGetTickCount();

    while (1) {
        log_msg_t *msg;
        TickType_t timeout = pdMS_TO_TICKS(backend->batch_timeout_ms);

        if (xQueueReceive(backend->msg_queue, &msg, timeout) == pdTRUE) {
            messages[msg_count++] = msg;

            // Flush if batch is full
            if (msg_count >= backend->batch_size) {
                flush_batch(messages, msg_count);
                msg_count = 0;
                last_flush = xTaskGetTickCount();
            }
        } else {
            // Timeout - flush any pending messages
            if (msg_count > 0) {
                flush_batch(messages, msg_count);
                msg_count = 0;
                last_flush = xTaskGetTickCount();
            }
        }
    }
}
```

### Colored Console Backend

A backend with ANSI color support:

```c
typedef struct {
    log_backend_t backend;
    bool colors_enabled;
    FILE *output_stream;
} console_backend_t;

static void console_backend_process_msg(const log_backend_t *backend,
                                        const log_msg_t *msg) {
    console_backend_t *console = (console_backend_t *)backend;

    // Get color code based on level
    const char *color = "";
    const char *reset = "";

    if (console->colors_enabled) {
        switch (msg->level) {
            case LOG_LEVEL_ERROR:
                color = "\033[31m";  // Red
                break;
            case LOG_LEVEL_WARNING:
                color = "\033[33m";  // Yellow
                break;
            case LOG_LEVEL_INFO:
                color = "\033[37m";  // White
                break;
            case LOG_LEVEL_DEBUG:
                color = "\033[34m";  // Blue
                break;
        }
        reset = "\033[0m";
    }

    // Format with colors
    char buffer[512];
    log_format_message(msg, buffer, sizeof(buffer));

    fprintf(console->output_stream, "%s%s%s", color, buffer, reset);
    fflush(console->output_stream);
}
```

## Best Practices

### 1. Thread Safety
Always protect shared resources with mutexes or semaphores:

```c
static void thread_safe_backend_process(const log_backend_t *backend,
                                        const log_msg_t *msg) {
    my_backend_t *my_backend = (my_backend_t *)backend;

    if (xSemaphoreTake(my_backend->mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Critical section
        perform_output(msg);
        xSemaphoreGive(my_backend->mutex);
    } else {
        // Handle timeout - maybe increment dropped message counter
        my_backend->dropped_messages++;
    }
}
```

### 2. Non-Blocking Operations
Avoid blocking the logging thread:

```c
static void non_blocking_backend_process(const log_backend_t *backend,
                                         const log_msg_t *msg) {
    // BAD: Blocking operation
    // vTaskDelay(pdMS_TO_TICKS(100));  // Don't do this!

    // GOOD: Queue for later processing
    xQueueSend(processing_queue, msg, 0);  // Non-blocking send
}
```

### 3. Error Handling
Gracefully handle backend failures:

```c
static void robust_backend_process(const log_backend_t *backend,
                                   const log_msg_t *msg) {
    static uint32_t consecutive_errors = 0;

    if (output_message(msg) != 0) {
        consecutive_errors++;

        if (consecutive_errors > 10) {
            // Disable backend or attempt recovery
            backend_recovery_procedure();
            consecutive_errors = 0;
        }
    } else {
        consecutive_errors = 0;
    }
}
```

### 4. Resource Management
Properly manage memory and resources:

```c
typedef struct {
    log_backend_t backend;
    uint8_t *buffer;
    size_t buffer_size;
    bool initialized;
} managed_backend_t;

int managed_backend_init(size_t buffer_size) {
    managed_backend_t *backend = &managed_backend_instance;

    // Allocate resources
    backend->buffer = pvPortMalloc(buffer_size);
    if (!backend->buffer) {
        return -1;
    }

    backend->buffer_size = buffer_size;
    backend->initialized = true;

    // Register backend
    return log_backend_register_backend(&backend->backend);
}

void managed_backend_deinit(void) {
    managed_backend_t *backend = &managed_backend_instance;

    if (backend->initialized) {
        // Free resources
        vPortFree(backend->buffer);
        backend->buffer = NULL;
        backend->initialized = false;
    }
}
```

### 5. Performance Optimization
Minimize processing time in the backend:

```c
static void optimized_backend_process(const log_backend_t *backend,
                                      const log_msg_t *msg) {
    // Pre-allocate buffers
    static char format_buffer[512];

    // Use stack allocation when possible
    char temp[64];

    // Avoid expensive operations
    if (msg->level < LOG_LEVEL_WARNING) {
        return;  // Early exit for low-priority messages
    }

    // Batch operations when possible
    add_to_batch(msg);
    if (batch_full()) {
        flush_batch();
    }
}
```

## Testing Your Backend

Example test code for your custom backend:

```c
void test_custom_backend(void) {
    // Initialize the backend
    my_backend_init();

    // Register a test module
    LOG_REGISTER_MODULE(backend_test)

    // Test different log levels
    LOG_DBG("Debug test: %d", 1);
    LOG_INF("Info test: %s", "hello");
    LOG_WRN("Warning test: %.2f", 3.14);
    LOG_ERR("Error test: 0x%08X", 0xDEADBEEF);

    // Test high-frequency logging
    for (int i = 0; i < 100; i++) {
        LOG_DBG("Stress test message %d", i);
    }

    // Allow time for processing
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Verify backend received all messages
    verify_backend_output();
}
```

## Conclusion

Creating custom backends allows you to extend the logging system to meet your specific needs. Whether you need to send logs over a network, store them persistently, or display them on a custom interface, the backend API provides the flexibility to implement any output mechanism while maintaining the benefits of the centralized logging system.
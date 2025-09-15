# FreeRTOS Logger Usage Guide

This guide covers how to use the FreeRTOS logging system in your application.

## Table of Contents
- [Quick Start](#quick-start)
- [Initialization](#initialization)
- [Basic Logging](#basic-logging)
- [Module Registration](#module-registration)
- [Log Levels](#log-levels)
- [Thread Safety](#thread-safety)
- [ISR Logging](#isr-logging)
- [Performance Considerations](#performance-considerations)

## Quick Start

```c
#include "log.h"

// Register your module name (typically at the top of your source file)
LOG_REGISTER_MODULE(my_module)

void example_function() {
    // Log at different levels
    LOG_DBG("Debug message: value = %d", 42);
    LOG_INF("Info message: system initialized");
    LOG_WRN("Warning: buffer usage at %d%%", 85);
    LOG_ERR("Error: failed to open file %s", filename);
}
```

## Initialization

Before using the logging system, you must initialize it and start the logging thread:

```c
#include "log_core.h"

int main() {
    // Initialize FreeRTOS (your normal setup)

    // Initialize the logging system
    if (log_init() != 0) {
        // Handle initialization error
        return -1;
    }

    // Start the logging thread
    if (log_start_thread() != 0) {
        // Handle thread creation error
        return -1;
    }

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}
```

## Basic Logging

The logging system provides four macros for different severity levels:

### Debug Messages
```c
LOG_DBG("Entering function with params: x=%d, y=%d", x, y);
LOG_DBG("Memory allocated: %p, size: %zu bytes", ptr, size);
```

### Info Messages
```c
LOG_INF("System startup complete");
LOG_INF("Connected to server at %s:%d", ip_addr, port);
LOG_INF("Configuration loaded from %s", config_file);
```

### Warning Messages
```c
LOG_WRN("Queue nearly full: %d/%d items", current, max);
LOG_WRN("Retry attempt %d of %d", attempt, max_retries);
LOG_WRN("Temperature reading high: %.1f°C", temp);
```

### Error Messages
```c
LOG_ERR("Failed to allocate %zu bytes", size);
LOG_ERR("Timeout waiting for response from device 0x%02X", device_id);
LOG_ERR("Invalid parameter: expected range [%d, %d], got %d", min, max, val);
```

## Module Registration

Each source file should register a module name to help identify the source of log messages:

```c
// At the top of your source file, after includes
LOG_REGISTER_MODULE(uart_driver)

void uart_init() {
    LOG_INF("Initializing UART");
    // Your initialization code
}

void uart_send(const uint8_t *data, size_t len) {
    LOG_DBG("Sending %zu bytes", len);
    // Your send implementation
}
```

Module names appear in log output to help with debugging:
```
[1234] <INF> uart_driver::uart_init: Initializing UART
[1250] <DBG> uart_driver::uart_send: Sending 64 bytes
```

## Log Levels

The system defines five log levels:

| Level | Value | Macro | Description |
|-------|-------|-------|-------------|
| NONE | 0 | - | No logging |
| ERROR | 1 | LOG_ERR | Critical errors that require attention |
| WARNING | 2 | LOG_WRN | Important warnings, potential issues |
| INFO | 3 | LOG_INF | General informational messages |
| DEBUG | 4 | LOG_DBG | Detailed debug information |

Log levels can be configured at compile time or runtime (depending on your configuration).

## Thread Safety

The logging system is thread-safe and can be called from any FreeRTOS task:

```c
void task1(void *pvParameters) {
    LOG_REGISTER_MODULE(task1)

    while (1) {
        LOG_INF("Task 1 executing");
        // Thread-safe - no mutex needed
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task2(void *pvParameters) {
    LOG_REGISTER_MODULE(task2)

    while (1) {
        LOG_INF("Task 2 executing");
        // Can log simultaneously with task1
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

## ISR Logging

The logging system automatically detects when it's called from an ISR context and handles it appropriately:

```c
void UART_IRQHandler(void) {
    LOG_REGISTER_MODULE(uart_isr)

    uint8_t data = UART_READ_DATA();
    LOG_DBG("Received byte: 0x%02X", data);

    // The logger automatically uses ISR-safe functions
    // when called from interrupt context
}
```

**Note**: Logging from ISRs should be minimized as it can affect interrupt latency.

## Performance Considerations

### Deferred Processing
The logging system uses deferred processing to minimize impact on your application:

```c
void time_critical_function() {
    // Log message is queued, not processed immediately
    LOG_DBG("Starting critical section");

    // Your time-critical code runs without waiting for log output
    perform_critical_operation();

    LOG_DBG("Critical section complete");
}
```

### Memory Pool
Messages are allocated from a pre-allocated pool to avoid dynamic allocation:

```c
void high_frequency_function() {
    // Messages use pre-allocated buffers
    LOG_DBG("Processing item %d", item_id);

    // No malloc/free overhead
    process_item();
}
```

### Best Practices for Performance

1. **Avoid excessive debug logging in production**
   ```c
   #ifdef DEBUG_BUILD
       LOG_DBG("Detailed state: %s", get_detailed_state());
   #endif
   ```

2. **Keep format strings simple**
   ```c
   // Good - simple and fast
   LOG_INF("Temperature: %d", temp);

   // Avoid - complex formatting
   LOG_INF("Complex: %.*s %#x %+.2f", len, str, hex, float_val);
   ```

3. **Batch related information**
   ```c
   // Instead of multiple calls
   LOG_DBG("X: %d", x);
   LOG_DBG("Y: %d", y);
   LOG_DBG("Z: %d", z);

   // Use a single call
   LOG_DBG("Position: X=%d, Y=%d, Z=%d", x, y, z);
   ```

4. **Use appropriate log levels**
   ```c
   // Use DEBUG for frequent messages
   LOG_DBG("Packet received");  // Called frequently

   // Use INFO for important state changes
   LOG_INF("Connection established");  // Called occasionally

   // Reserve ERROR for actual errors
   LOG_ERR("Hardware fault detected");  // Rare, critical events
   ```

## Advanced Usage

### Conditional Logging
```c
void process_data(uint8_t *data, size_t len) {
    LOG_REGISTER_MODULE(data_processor)

    if (len > 1024) {
        LOG_WRN("Large data block: %zu bytes", len);
    }

    #ifdef VERBOSE_LOGGING
        LOG_DBG("Processing data block:");
        for (size_t i = 0; i < MIN(len, 16); i++) {
            LOG_DBG("  [%02zu]: 0x%02X", i, data[i]);
        }
    #endif

    // Process the data
}
```

### Structured Logging
```c
typedef struct {
    uint32_t id;
    uint32_t timestamp;
    float value;
} sensor_reading_t;

void log_sensor_reading(sensor_reading_t *reading) {
    LOG_REGISTER_MODULE(sensor)

    LOG_INF("Sensor[%u] @ %u: %.2f",
            reading->id,
            reading->timestamp,
            reading->value);
}
```

### Error Reporting Pattern
```c
int perform_operation() {
    LOG_REGISTER_MODULE(operations)

    LOG_DBG("Starting operation");

    int result = do_something();
    if (result != 0) {
        LOG_ERR("Operation failed with code: %d", result);
        return result;
    }

    LOG_INF("Operation completed successfully");
    return 0;
}
```

## Troubleshooting

### Common Issues

1. **No log output**
   - Ensure `log_init()` and `log_start_thread()` are called
   - Check that a backend is registered
   - Verify log level configuration

2. **Missing module name in output**
   - Add `LOG_REGISTER_MODULE()` at the top of your source file

3. **Logs appear delayed**
   - This is normal due to deferred processing
   - Critical logs may need immediate processing (custom backend)

4. **Running out of message buffers**
   - Increase pool size in configuration
   - Reduce logging frequency
   - Check for log flooding in loops
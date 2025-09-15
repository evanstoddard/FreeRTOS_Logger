# FreeRTOS Logging System

A lightweight, thread-safe, and extensible logging system designed specifically for FreeRTOS-based embedded systems. This library provides efficient deferred logging with minimal impact on real-time performance.

## Features

- **Thread-Safe**: Safe to call from multiple FreeRTOS tasks and ISRs
- **Deferred Processing**: Minimal impact on calling thread performance
- **Multiple Log Levels**: DEBUG, INFO, WARNING, ERROR
- **Extensible Backend System**: Output to UART, files, network, or custom destinations
- **Memory Pool Based**: No dynamic allocation during runtime
- **Module Support**: Track log sources by module name
- **ISR Safe**: Automatically detects and handles ISR context
- **Formatted Output**: Printf-style formatting with color support
- **Low Overhead**: Optimized for embedded systems

## Quick Start

### Basic Usage

```c
#include "log.h"

// Register your module (once per source file)
LOG_REGISTER_MODULE(my_module)

void example_function() {
    LOG_INF("System initialized");
    LOG_DBG("Debug value: %d", sensor_reading);
    LOG_WRN("Buffer usage: %d%%", usage);
    LOG_ERR("Failed to open file: %s", filename);
}
```

### Initialization

```c
#include "log_core.h"

int main() {
    // Initialize the logging system
    log_init();

    // Register your backend (e.g., UART)
    uart_backend_init();

    // Start FreeRTOS scheduler
    vTaskStartScheduler();
}
```

## Architecture

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Application   │────▶│   Log Macros    │────▶│   Message Queue │
│     Tasks       │     │  (LOG_INF, etc) │     │   (Thread-safe) │
└─────────────────┘     └─────────────────┘     └────────┬────────┘
                                                          │
                        ┌─────────────────┐              ▼
                        │  Log Backends   │◀────┌─────────────────┐
                        │  - UART         │     │   Log Thread    │
                        │  - File         │     │  (Deferred      │
                        │  - Network      │     │   Processing)   │
                        │  - Custom       │     └─────────────────┘
                        └─────────────────┘
```

## Components

### Core Components

- **log.h**: Main API with logging macros (LOG_DBG, LOG_INF, LOG_WRN, LOG_ERR)
- **log_core.h/c**: Core logging system implementation
- **log_backend.h/c**: Backend registration and management
- **log_msg.h**: Message structure definitions
- **log_queue.h/c**: Thread-safe message queue
- **log_pool.h/c**: Memory pool for message allocation
- **log_format.h/c**: Message formatting utilities
- **log_reconstruct.h/c**: Message reconstruction from binary format


## Documentation

- [Usage Guide](docs/usage.md) - Detailed usage instructions and examples
- [Backend Creation Guide](docs/backend-creation.md) - How to create custom log backends

## Disclaimer

This code is very much a work in progress.  There are several currently known issues:

- `float`/`double` corruption
- Strings build on stack are broken and will most likely result in a fault
- Thread safety and allocation pool is largely untested
- Format strings must be static or literals
- Documentation is currently AI generated an may be incorrect
- Currently no test cases
- `va_list` reconstruction not fully tested/validated

Some future features:

- Hexdump logs
- Compile time and runtime filtering
- More compile time/runtime options such as selectable timestamp, level string length, module/function name selectors, color support
- Panic mode
- Ratelimiting
- Backend templates
- Compiler support (currently only targetting gcc, clang untested)
- Immediate logging mode
- Dropped message indicators

---
title: "WAMR in-depth tutorial (wasmruntime.com)"
doc_type: reference
ticket: ESP32-P4-QUICKJS-WASM
topics:
  - wasm
  - quickjs
  - esp32p4
  - esp-idf
status: active
source_type: harvested
---

## Core Positioning

**WAMR** (WebAssembly Micro Runtime) is a lightweight WebAssembly runtime maintained by the 
[Bytecode Alliance](https://bytecodealliance.org/), **designed specifically for 
resource-constrained environments**. It is one of the smallest production-grade Wasm runtimes 
available.

| Target Scenario | Fit | Technical Basis |
| --- | --- | --- |
| Industrial IoT | Excellent | Supports Zephyr, NuttX, RT-Thread, and other RTOS |
| Edge Gateways | Excellent | AOT compilation provides near-native performance |
| Automotive Electronics | Excellent | VxWorks support, deterministic execution |
| Smart Home | Good | Minimum configuration ~50KB Flash |
| General Servers | Limited | Less feature-rich than Wasmtime/WasmEdge |

> \[!IMPORTANT\] **WAMR vs Other Runtimes**: The key difference between WAMR and Wasmtime/WasmEdge 
is the **design goal** - WAMR prioritizes size and resource consumption over feature completeness. 
If your target platform has RAM < 512KB or requires RTOS support, WAMR is the best choice.

### Prerequisites

| Requirement | Version | Notes |
| --- | --- | --- |
| WAMR | v2.2.0+ | Build from source or use Docker |
| WASI SDK | 24.0 | For compiling C/C++ to Wasm |
| Rust | 1.78+ | For wasm32-wasip1 target (optional) |

---

## Chapter 1: Mental Model

Before writing any code, understand WebAssembly runtime core concepts:

### 1.1 Concept Analogy

| Concept | Analogy | Implementation in WAMR |
| --- | --- | --- |
| **Runtime** | Operating System Kernel | Global runtime environment created by 
wasm\_runtime\_init() |
| **Module** | Executable Program (.exe) | In-memory representation after loading.wasm binary |
| **Instance** | Running Process | Instantiated module with independent memory and state |

### 1.2 Lifecycle Diagram

![WAMR Lifecycle: Shows the complete lifecycle from Wasm Binary through Loader to Module, then 
Instantiate to Instance, finally Execute to get 
results](https://wasmruntime.com/tutorials/wamr/diagram-0.png)

*Figure 1: WAMR Lifecycle - Complete flow from Wasm Binary loading to execution results*

### 1.3 Development Workflow

A complete WAMR development cycle follows this flow:

```
1. Write -> 2. Compile -> 3. [Optional AOT] -> 4. Load/Instantiate -> 5. Call Exported Functions
```

![WAMR Development Workflow: Shows source code (C/Rust/TinyGo) compiled to Wasm Binary, optionally 
AOT compiled via wamrc to .aot file, then loaded by iwasm to call exported 
functions](https://wasmruntime.com/tutorials/wamr/diagram-1.png)

*Figure 2: WAMR Development Workflow - From source compilation through optional AOT to execution*

---

## Chapter 2: Installation

### 2.1 Version Notes

This tutorial is based on **WAMR v2.2.0** (released December 2024). Choose installation method 
based on your use case.

### 2.2 Method 1: Build from Source (Recommended)

```bash
# Clone specific version
git clone --branch WAMR-2.2.0 --depth 1 \
    https://github.com/bytecodealliance/wasm-micro-runtime.git
cd wasm-micro-runtime

# Build iwasm (Linux x64)
cd product-mini/platforms/linux
mkdir build && cd build
cmake .. \
    -DWAMR_BUILD_INTERP=1 \
    -DWAMR_BUILD_FAST_INTERP=1 \
    -DWAMR_BUILD_AOT=1 \
    -DWAMR_BUILD_LIBC_WASI=1 \
    -DWAMR_BUILD_LIBC_BUILTIN=1
make -j$(nproc)

# Verify installation
./iwasm --version
# Expected output: iwasm 2.2.0
```

### 2.3 Method 2: Docker (Quick Testing)

```bash
# Use pre-built Docker image
docker run --rm -it bytecodealliance/wasm-micro-runtime:v2.2.0 iwasm --version
```

### 2.4 Tool Overview

WAMR provides two core tools:

| Tool | Responsibility | Use Case |
| --- | --- | --- |
| **iwasm** | Runtime interpreter/loader | Load and execute.wasm or.aot files |
| **wamrc** | AOT compiler | Pre-compile.wasm to.aot for better performance |

![WAMR Execution Modes: Shows .wasm file can choose Interpreter mode directly via iwasm, or AOT 
mode via wamrc to .aot file then executed by 
iwasm](https://wasmruntime.com/tutorials/wamr/diagram-2.png)

*Figure 3: WAMR Execution Modes - Interpreter vs AOT execution paths*

### 2.5 Build Options Reference

Common CMake build options:

```bash
# Execution mode selection
-DWAMR_BUILD_INTERP=1          # Enable classic interpreter
-DWAMR_BUILD_FAST_INTERP=1     # Enable fast interpreter (recommended)
-DWAMR_BUILD_AOT=1             # Enable AOT loading support
-DWAMR_BUILD_JIT=1             # Enable LLVM JIT (requires LLVM)

# Feature modules
-DWAMR_BUILD_LIBC_WASI=1       # Enable WASI (file/network/env)
-DWAMR_BUILD_LIBC_BUILTIN=1    # Enable built-in libc (printf/malloc)
-DWAMR_BUILD_MULTI_MODULE=1    # Enable multi-module linking

# Debug and profiling
-DWAMR_BUILD_DEBUG_INTERP=1    # Enable GDB debug support
-DWAMR_BUILD_DUMP_CALL_STACK=1 # Enable call stack dumping
-DWAMR_BUILD_MEMORY_PROFILING=1 # Enable memory profiling

# Size optimization (embedded scenarios)
-DWAMR_BUILD_MINI_LOADER=1     # Minimal loader
-DWAMR_DISABLE_HW_BOUND_CHECK=1 # Disable hardware bounds checking
```

> \[!WARNING\] **Security Warning**: Disabling hardware bounds checking removes Wasm sandbox memory 
isolation. Only use when trusting all Wasm code.

---

## Chapter 3: Quick Start

### 3.1 Step 1: Write Guest Code

Create a simple addition function (using C):

```c
// add.c
__attribute__((export_name("add")))
int add(int a, int b) {
    return a + b;
}
```

### 3.2 Step 2: Compile to Wasm

```bash
# Set WASI SDK path
export WASI_SDK_PATH=/opt/wasi-sdk-24.0

# Compile to wasm
$WASI_SDK_PATH/bin/clang \
    --target=wasm32-wasi \
    -O2 \
    -Wl,--no-entry \
    -Wl,--export=add \
    -o add.wasm \
    add.c

# Verify exports
wasm-objdump -x add.wasm | grep "Export"
```

### 3.3 Step 3: Run with iwasm

```bash
# Run directly with interpreter
iwasm --function add add.wasm 3 5
# Expected output: 8

# Or use AOT mode for better performance
wamrc -o add.aot add.wasm
iwasm --function add add.aot 3 5
# Expected output: 8
```

### 3.4 Step 4: C Embedding Integration

Production-grade C embedding example with error handling:

```c
// main.c - WAMR embedding example
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wasm_export.h"

static uint8_t *load_file(const char *filename, uint32_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[ERROR] Failed to open file: %s\n", filename);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *buf = malloc(*size);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    if (fread(buf, 1, *size, f) != *size) {
        free(buf);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    char error_buf[128];
    uint32_t wasm_size;
    uint8_t *wasm_buf = NULL;
    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    int ret = 1;

    // Step 1: Initialize WAMR Runtime
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    
    if (!wasm_runtime_full_init(&init_args)) {
        fprintf(stderr, "[ERROR] wasm_runtime_full_init failed\n");
        goto cleanup;
    }

    // Step 2: Load Wasm Binary
    wasm_buf = load_file("add.wasm", &wasm_size);
    if (!wasm_buf) goto cleanup;

    // Step 3: Parse and Validate Module
    module = wasm_runtime_load(wasm_buf, wasm_size, error_buf, sizeof(error_buf));
    if (!module) {
        fprintf(stderr, "[ERROR] wasm_runtime_load failed: %s\n", error_buf);
        goto cleanup;
    }

    // Step 4: Instantiate Module
    module_inst = wasm_runtime_instantiate(module, 8192, 8192, error_buf, sizeof(error_buf));
    if (!module_inst) {
        fprintf(stderr, "[ERROR] wasm_runtime_instantiate failed: %s\n", error_buf);
        goto cleanup;
    }

    // Step 5: Create Execution Environment
    exec_env = wasm_runtime_create_exec_env(module_inst, 4096);
    if (!exec_env) {
        fprintf(stderr, "[ERROR] wasm_runtime_create_exec_env failed\n");
        goto cleanup;
    }

    // Step 6: Find and Call Exported Function
    wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "add");
    if (!func) {
        fprintf(stderr, "[ERROR] Function 'add' not found\n");
        goto cleanup;
    }

    uint32_t argv[2] = {3, 5};
    
    if (!wasm_runtime_call_wasm(exec_env, func, 2, argv)) {
        const char *exception = wasm_runtime_get_exception(module_inst);
        fprintf(stderr, "[ERROR] Call failed: %s\n", exception ? exception : "unknown");
        goto cleanup;
    }

    printf("[SUCCESS] add(3, 5) = %d\n", argv[0]);
    ret = 0;

cleanup:
    if (exec_env) wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst) wasm_runtime_deinstantiate(module_inst);
    if (module) wasm_runtime_unload(module);
    if (wasm_buf) free(wasm_buf);
    wasm_runtime_destroy();
    
    return ret;
}
```

Build and run:

```bash
gcc -O2 main.c \
    -I/path/to/wasm-micro-runtime/core/iwasm/include \
    -L/path/to/wasm-micro-runtime/product-mini/platforms/linux/build \
    -lwamr -lpthread -lm \
    -o wamr_host

./wamr_host
# Expected: [SUCCESS] add(3, 5) = 8
```

---

## Chapter 4: Host Functions

Host Functions are the core mechanism for Wasm-to-host interaction.

### 4.1 Complete Example: Logging Host Function

```c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "wasm_export.h"

// Host function: Print string
static void host_log(wasm_exec_env_t exec_env, const char *message, uint32_t message_len) {
    printf("[WASM LOG] %.*s\n", message_len, message);
}

// Host function: Get current timestamp
static int64_t host_get_timestamp(wasm_exec_env_t exec_env) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Host function: Math operation
static double host_pow(wasm_exec_env_t exec_env, double base, double exp) {
    return pow(base, exp);
}

// Signature syntax: i=i32, I=i64, f=f32, F=f64, *=pointer, ~=length
static NativeSymbol native_symbols[] = {
    { "log",           (void*)host_log,           "(*~)",   NULL },
    { "get_timestamp", (void*)host_get_timestamp, "()I",    NULL },
    { "pow",           (void*)host_pow,           "(FF)F",  NULL },
};

int register_host_functions() {
    int count = sizeof(native_symbols) / sizeof(NativeSymbol);
    if (!wasm_runtime_register_natives("env", native_symbols, count)) {
        fprintf(stderr, "[ERROR] Failed to register native functions\n");
        return -1;
    }
    printf("[INFO] Registered %d host functions\n", count);
    return 0;
}
```

### 4.2 Corresponding Wasm Code (C Source)

```c
// guest.c - Wasm code calling host functions
#include <stdint.h>

__attribute__((import_module("env"), import_name("log")))
extern void host_log(const char *msg, uint32_t len);

__attribute__((import_module("env"), import_name("get_timestamp")))
extern int64_t host_get_timestamp(void);

__attribute__((export_name("run")))
void run() {
    const char *msg = "Hello from Wasm!";
    host_log(msg, 16);
    int64_t ts = host_get_timestamp();
}
```

---

## Chapter 5: Execution Modes

WAMR provides four execution modes:

| Mode | Performance | Startup Time | Size Impact | Use Case |
| --- | --- | --- | --- | --- |
| Classic Interpreter | 1x | Fastest | Smallest | Debugging, extremely constrained |
| Fast Interpreter | 2-3x | Fast | +10KB | Development, general embedded |
| LLVM JIT | 6-10x | Slow | +MB | Dynamic loading, long-running |
| AOT (wamrc) | 8-12x | Fast | Larger | Production, max performance |

![WAMR Execution Mode Architecture: Shows .wasm module can choose Classic Interpreter, Fast 
Interpreter, LLVM JIT Compiler, or wamrc AOT 
Compiler](https://wasmruntime.com/tutorials/wamr/diagram-3.png)

*Figure 4: WAMR Execution Mode Architecture*

### Decision Guide

- **Flash < 100KB**: Use Classic Interpreter (smallest size)
- **Need dynamic loading**: Use Fast Interpreter or JIT
- **Need max performance**: Use AOT (wamrc pre-compilation)
- **Development/debugging**: Use Fast Interpreter

---

## Chapter 6: AOT Compilation

### 6.1 Using wamrc Compiler

```bash
# Basic compilation
wamrc -o app.aot app.wasm

# Target architecture (cross-compilation)
wamrc --target=aarch64 -o app.aot app.wasm     # ARM64
wamrc --target=thumbv7 -o app.aot app.wasm     # ARM Cortex-M
wamrc --target=riscv64 -o app.aot app.wasm     # RISC-V 64
wamrc --target=x86_64 -o app.aot app.wasm      # x86-64

# Optimization levels
wamrc -O0 -o app.aot app.wasm  # No optimization (debugging)
wamrc -O2 -o app.aot app.wasm  # Balanced (recommended)
wamrc -O3 -o app.aot app.wasm  # Aggressive (may increase size)

# Enable features
wamrc --enable-simd -o app.aot app.wasm
wamrc --enable-multi-thread -o app.aot app.wasm
```

### 6.2 Cross-Compilation Examples

```bash
# ARM Cortex-M4 (Thumb instruction set)
wamrc --target=thumbv7em --cpu=cortex-m4 --cpu-features=+fp-armv8 -o app.aot app.wasm

# RISC-V 32-bit (no FPU)
wamrc --target=riscv32 --cpu-features=-d,-f -o app.aot app.wasm
```

### Common Issues

| Error | Cause | Solution |
| --- | --- | --- |
| Illegal Instruction | CPU features mismatch | Check --cpu-features matches target |
| Unreachable | Wrong Thumb/ARM selection | Use thumbv7 for Cortex-M, armv7 for Cortex-A |
| Link failure | Sysroot/Libc mismatch | Ensure AOT Libc ABI matches runtime |

![WAMR AOT Workflow](https://wasmruntime.com/tutorials/wamr/diagram-4.png)

*Figure 5: WAMR AOT Workflow*

---

## Chapter 7: Production Deployment

### 7.1 Resource Planning

```c
// Production resource configuration
RuntimeInitArgs init_args = {0};

// Method 1: Static memory pool (recommended for RTOS)
static uint8_t global_heap[64 * 1024];
init_args.mem_alloc_type = Alloc_With_Pool;
init_args.mem_alloc_option.pool.heap_buf = global_heap;
init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap);

wasm_runtime_full_init(&init_args);

// Sizing: Stack = call depth * ~256 bytes, Heap = allocation needs * 1.5
wasm_module_inst_t inst = wasm_runtime_instantiate(
    module, 8 * 1024, 16 * 1024, error_buf, sizeof(error_buf));
```

### 7.2 XIP (Execute In Place) Mode

For Flash supporting XIP (like SPI NOR Flash), WAMR AOT can execute directly from Flash:

```bash
wamrc --enable-indirect-mode --disable-llvm-intrinsics -o app.aot app.wasm
```

### 7.3 Minimal Size Configuration

```bash
cmake .. \
    -DWAMR_BUILD_INTERP=1 \
    -DWAMR_BUILD_FAST_INTERP=0 \
    -DWAMR_BUILD_AOT=0 \
    -DWAMR_BUILD_LIBC_WASI=0 \
    -DWAMR_BUILD_LIBC_BUILTIN=1 \
    -DWAMR_BUILD_MINI_LOADER=1 \
    -DWAMR_DISABLE_HW_BOUND_CHECK=1 \
    -DCMAKE_C_FLAGS="-Os"
```

![WAMR Multi-Instance Management](https://wasmruntime.com/tutorials/wamr/diagram-5.png)

*Figure 6: WAMR Multi-Instance Management*

---

## Chapter 8: Debugging and Observability

### 8.1 Enable Logging

```bash
cmake .. -DWAMR_BUILD_DUMP_CALL_STACK=1 -DCMAKE_BUILD_TYPE=Debug

export WAMR_LOG_LEVEL=5  # 0=off, 5=verbose
iwasm --log-level=5 app.wasm
```

### 8.2 GDB Remote Debugging

```bash
cmake .. -DWAMR_BUILD_DEBUG_INTERP=1

iwasm -g=127.0.0.1:1234 app.wasm

# In another terminal
gdb
(gdb) target remote 127.0.0.1:1234
(gdb) b main
(gdb) c
(gdb) bt
```

### 8.3 Exception Handling

```c
if (!wasm_runtime_call_wasm(exec_env, func, argc, argv)) {
    const char *exception = wasm_runtime_get_exception(module_inst);
    fprintf(stderr, "[WASM EXCEPTION] %s\n", exception ? exception : "unknown");
    wasm_runtime_clear_exception(module_inst);
}
```

### 8.4 Memory Profiling

```bash
cmake .. -DWAMR_BUILD_MEMORY_PROFILING=1
```

```c
uint32_t heap_size = wasm_runtime_get_app_heap_size(module_inst);
uint32_t mem_size = wasm_runtime_get_memory_size(module_inst);
printf("App Heap: %u bytes, Linear Memory: %u bytes\n", heap_size, mem_size);
```

---

## Chapter 9: RTOS Integration

### Zephyr RTOS Example

```c
#include <zephyr/kernel.h>
#include "wasm_export.h"

static uint8_t global_heap_buf[32 * 1024];

void main(void) {
    char error_buf[64];
    
    RuntimeInitArgs init_args = {0};
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
    
    if (!wasm_runtime_full_init(&init_args)) {
        printk("WAMR init failed\n");
        return;
    }
    
    extern const uint8_t wasm_bytes[];
    extern const uint32_t wasm_size;
    
    wasm_module_t module = wasm_runtime_load(
        wasm_bytes, wasm_size, error_buf, sizeof(error_buf));
    if (!module) {
        printk("Load failed: %s\n", error_buf);
        return;
    }
    
    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, 4096, 4096, error_buf, sizeof(error_buf));
    if (!inst) {
        printk("Instantiate failed: %s\n", error_buf);
        return;
    }
    
    wasm_application_execute_main(inst, 0, NULL);
    
    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
    wasm_runtime_destroy();
    
    printk("WASM execution completed\n");
}
```

---

## Troubleshooting

| Error | Cause | Solution |
| --- | --- | --- |
| wasm\_runtime\_load failed | Invalid Wasm binary | Validate with wasm-validate tool |
| Function not found | Function not exported | Check exports with wasm-objdump -x |
| Memory out of bounds | Invalid pointer/offset | Validate bounds in host functions |
| Stack overflow | Insufficient stack size | Increase stack\_size in instantiate |
| Illegal instruction (AOT) | Architecture mismatch | Verify --target matches runtime CPU |

---

## FAQ

### WAMR vs Wasmtime/WasmEdge

| Consideration | Choose WAMR | Choose Wasmtime/WasmEdge |
| --- | --- | --- |
| Target Platform | RTOS, MCU, embedded Linux | General servers, desktop |
| RAM Limit | < 512KB | No strict limit |
| Feature Needs | Basic WASI, core Wasm | Full WASI, Component Model |
| Size Requirement | Strict (< 500KB) | Flexible |

### Which Execution Mode?

- **Classic Interpreter**: Flash < 100KB, extremely constrained
- **Fast Interpreter**: Development/debugging, general embedded
- **AOT**: Production, maximum performance
- **JIT**: Dynamic loading with sufficient resources

### Heap/Stack Sizing

```
Stack estimate: expected call depth * 256 bytes
Heap estimate: Wasm malloc total needs * 1.5 (safety margin)
```

### Does WAMR Support Component Model?

Currently (v2.2.0), WAMR focuses on core WebAssembly spec and WASI Preview 1. Component Model 
support is being evaluated but is not a priority.

### Minimum Hardware Requirements

| Resource | Minimum | Recommended |
| --- | --- | --- |
| Flash | 50KB (minimal config) | 256KB |
| RAM | 64KB | 256KB |
| CPU | Any 32-bit | ARM Cortex-M4+ |

---

## Further Reading

- [WAMR GitHub](https://github.com/bytecodealliance/wasm-micro-runtime)
- [Architecture 
Overview](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/architecture.md)
- [Build 
Instructions](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md)
- [RTOS Integration 
Guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/port_wamr.md)

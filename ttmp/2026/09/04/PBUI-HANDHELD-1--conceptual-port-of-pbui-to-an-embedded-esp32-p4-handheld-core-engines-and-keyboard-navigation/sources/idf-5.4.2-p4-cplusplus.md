## C++ Support

[\[中文\]](../../../../zh_CN/v5.4.2/esp32p4/api-guides/cplusplus.html)

ESP-IDF is primarily written in C and provides C APIs. However, ESP-IDF supports development of applications in C++. This document covers various topics relevant to C++ development.

The following C++ features are supported:

- [Thread Local Storage](thread-local-storage.html) (`thread_local` keyword)
- All C++ features implemented by GCC, except for some . See [GCC documentation](https://gcc.gnu.org/projects/cxx-status.html) for details on features implemented by GCC.

## esp-idf-cxx Component

[esp-idf-cxx](https://github.com/espressif/esp-idf-cxx) component provides higher-level C++ APIs for some of the ESP-IDF features. This component is available from the [ESP Component Registry](https://components.espressif.com/components/espressif/esp-idf-cxx).

## C++ Language Standard

By default, ESP-IDF compiles C++ code with C++23 language standard with GNU extensions (`-std=gnu++23`).

To compile the source code of a certain component using a different language standard, set the desired compiler flag in the component's `CMakeLists.txt` file:

Use `PUBLIC` instead of `PRIVATE` if the public header files of the component also need to be compiled with the same language standard.

## Multithreading

C++ threads, mutexes, and condition variables are supported. C++ threads are built on top of pthreads, which in turn wrap FreeRTOS tasks.

See [cxx/pthread](https://github.com/espressif/esp-idf/tree/v5.4.2/examples/cxx/pthread) for an example of creating threads in C++. Specifically, this example demonstrates how to use the ESP-pthread component to modify the stack sizes, priorities, names, and core affinities of C++ threads.

Note

The destructor of [std::jthread](https://en.cppreference.com/w/cpp/thread/jthread) can only safely be called from a task that has been created by [Thread APIs](../api-reference/system/pthread.html#posix-thread-api) or by the [C++ threading library API](https://en.cppreference.com/w/cpp/thread).

## Exception Handling

Support for C++ Exceptions in ESP-IDF is disabled by default, but can be enabled using the [CONFIG\_COMPILER\_CXX\_EXCEPTIONS](../api-reference/kconfig.html#config-compiler-cxx-exceptions) option.

If an exception is thrown, but there is no `catch` block, the program is terminated by the `abort` function, and the backtrace is printed. See [Fatal Errors](fatal-errors.html) for more information about backtraces.

C++ Exceptions should **only** be used for exceptional cases, i.e., something happening unexpectedly and occurs rarely, such as events that happen less frequently than 1/100 times. **Do not** use them for control flow (see also the section about resource usage below). For more information on how to use C++ Exceptions, see the [ISO C++ FAQ](https://isocpp.org/wiki/faq/exceptions) and [CPP Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-errors).

See [cxx/exceptions](https://github.com/espressif/esp-idf/tree/v5.4.2/examples/cxx/exceptions) for an example of C++ exception handling. Specifically, this example demonstrates how to enable and use C++ exceptions in ESP32-P4, with a class that throws an exception from the constructor if the provided argument is equal to 0.

### C++ Exception Handling and Resource Usage

Enabling exception handling normally increases application binary size by a few KB.

Additionally, it may be necessary to reserve some amount of RAM for the exception emergency memory pool. Memory from this pool is used if it is not possible to allocate an exception object from the heap.

The amount of memory in the emergency pool can be set using the [CONFIG\_COMPILER\_CXX\_EXCEPTIONS\_EMG\_POOL\_SIZE](../api-reference/kconfig.html#config-compiler-cxx-exceptions-emg-pool-size) variable.

Some additional stack memory (around 200 bytes) is also used if and only if a C++ Exception is actually thrown, because it requires calling some functions from the top of the stack to initiate exception handling.

The run time of code using C++ exceptions depends on what actually happens at run time.

- If no exception is thrown, the code tends to be somewhat faster since there is no need to check error codes.
- If an exception is thrown, the run time of the code that handles exceptions is orders of magnitude slower than code returning an error code.

If an exception is thrown, the run time of the code that unwinds the stack is orders of magnitude slower than code returning an error code. The significance of the increased run time will depend on the application's requirements and implementation of error handling (e.g., requiring user input or messaging to a cloud). As a result, exception-throwing code should never be used in real-time critical code paths.

## Runtime Type Information (RTTI)

Support for RTTI in ESP-IDF is disabled by default, but can be enabled using [CONFIG\_COMPILER\_CXX\_RTTI](../api-reference/kconfig.html#config-compiler-cxx-rtti) option.

Enabling this option compiles all C++ files with RTTI support enabled, which allows using `dynamic_cast` conversion and `typeid` operator. Enabling this option typically increases the binary size by tens of kB.

See [cxx/rtti](https://github.com/espressif/esp-idf/tree/v5.4.2/examples/cxx/rtti) for an example of using RTTI in ESP-IDF. Specifically, this example demonstrates how to use the RTTI feature in ESP-IDF, enabling compile time support for RTTI, and showing how to print demangled type names of objects and functions, and how dynamic\_cast behaves with objects of two classes derived from a common base class.

## Filesystem Library

C++ Filesystem library (`#include <filesystem>`) features are supported in ESP-IDF, with the following exceptions:

- Since symbolic and hard links are not supported in ESP-IDF, related functions are not implemented.
- `std::filesystem::space` is not implemented.
- `std::filesystem::resize_file` is not implemented.
- `std::filesystem::current_path` always returns `/`. Setting the current path is not supported.
- `std::filesystem::permissions` doesn't support setting file permissions.

Note that the choice of the filesystem also affects the behavior of the filesystem library. For example, SPIFFS filesystem has limited support for directories, therefore the related std::filesystem functions may not work as they do on a filesystem which does support directories.

## Developing in C++

The following sections provide tips on developing ESP-IDF applications in C++.

### Combining C and C++ Code

When an application is developed using both C and C++, it is important to understand the concept of [language linkage](https://en.cppreference.com/w/cpp/language/language_linkage).

In order for a C++ function to be callable from C code, it has to be both **declared** and **defined** with C linkage (`extern "C"`):

```cpp
// declaration in the .h file:
#ifdef __cplusplus
extern "C" {
#endif

void my_cpp_func(void);

#ifdef __cplusplus
}
#endif

// definition in a .cpp file:
extern "C" void my_cpp_func(void) {
    // ...
}
```

In order for a C function to be callable from C++, it has to be **declared** with C linkage:

```c
// declaration in .h file:
#ifdef __cplusplus
extern "C" {
#endif

void my_c_func(void);

#ifdef __cplusplus
}
#endif

// definition in a .c file:
void my_c_func(void) {
    // ...
}
```

### Defining app\_main in C++

ESP-IDF expects the application entry point, `app_main`, to be defined with C linkage. When `app_main` is defined in a.cpp source file, it has to be designated as `extern "C"`:

```cpp
extern "C" void app_main()
{
}
```

### Designated Initializers

Many of the ESP-IDF components use [Configuration Structures](../api-reference/api-conventions.html#api-reference-config-structures) as arguments to the initialization functions. ESP-IDF examples written in C routinely use [designated initializers](https://en.cppreference.com/w/c/language/struct_initialization) to fill these structures in a readable and a maintainable way.

C and C++ languages have different rules with regards to the designated initializers. For example, C++23 (currently the default in ESP-IDF) does not support out-of-order designated initialization, nested designated initialization, mixing of designated initializers and regular initializers, and designated initialization of arrays. Therefore, when porting ESP-IDF C examples to C++, some changes to the structure initializers may be necessary. See the [C++ aggregate initialization reference](https://en.cppreference.com/w/cpp/language/aggregate_initialization) for more details.

### iostream

`iostream` functionality is supported in ESP-IDF, with a couple of caveats:

1. Normally, ESP-IDF build process eliminates the unused code. However, in the case of iostreams, simply including `<iostream>` header in one of the source files significantly increases the binary size by about 200 kB.
2. By default, ESP-IDF uses a simple non-blocking implementation of the standard input stream (`stdin`). To get the usual behavior of `std::cin`, the application has to initialize the UART driver and enable the blocking mode as shown in [common\_components/protocol\_examples\_common/stdin\_out.c](https://github.com/espressif/esp-idf/blob/v5.4.2/examples/common_components/protocol_examples_common/stdin_out.c).

## Limitations

- Linker script generator does not support function level placements for functions with C++ linkage.
- Various section attributes (such as `IRAM_ATTR`) are ignored when used with template functions.
- Vtables are placed into Flash and are not accessible when the flash cache is disabled. Therefore, virtual function calls should be avoided in [IRAM-Safe Interrupt Handlers](../api-reference/peripherals/spi_flash/spi_flash_concurrency.html#iram-safe-interrupt-handlers). Placement of Vtables cannot be adjusted using the linker script generator, yet.

## What to Avoid

Do not use `setjmp` / `longjmp` in C++. `longjmp` blindly jumps up the stack without calling any destructors, easily introducing undefined behavior and memory leaks. Use C++ exceptions instead, they guarantee correctly calling destructors. If you cannot use C++ exceptions, use alternatives (except `setjmp` / `longjmp` themselves) such as simple return codes.

---

**Was this page helpful?**

- Thank you! We received your feedback.
- If you have any comments, fill in [Espressif Documentation Feedback Form](<https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287&sections=C++ Support \(api-guides/cplusplus\)&version=esp32p4 v5.4.2 \(v5.4.2\)>).

- We value your feedback.
- Let us know how we can improve this page by filling in [Espressif Documentation Feedback Form](<https://www.espressif.com/en/company/documents/documentation_feedback?docId=4287&sections=C++ Support \(api-guides/cplusplus\)&version=esp32p4 v5.4.2 \(v5.4.2\)>).

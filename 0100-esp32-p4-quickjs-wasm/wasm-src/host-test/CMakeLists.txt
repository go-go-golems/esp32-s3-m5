# host-test CMakeLists — builds WAMR libvmlib (interp + WASI) and the host_test
# smoke program that loads quickjs.wasm and calls qjs_init/qjs_eval.
#
# Usage:
#   cmake -DWAMR_ROOT_DIR=<path-to-wasm-micro-runtime> -B build
#   cmake --build build -j
#   ./build/host_test <quickjs.wasm> "<js source>"
#
# WAMR_ROOT_DIR defaults to the copy vendored by project 0079 (managed_components).
cmake_minimum_required(VERSION 3.14)
project(host_test C)
set(CMAKE_C_STANDARD 11)

if(NOT DEFINED WAMR_ROOT_DIR)
  set(WAMR_ROOT_DIR "$ENV{HOME}/workspaces/2025-12-21/echo-base-documentation/esp32-s3-m5/0079-papers3-wamr-assemblyscript-console/managed_components/bytecodealliance__wasm-micro-runtime")
endif()
if(NOT EXISTS "${WAMR_ROOT_DIR}/core/iwasm/include/wasm_export.h")
  message(FATAL_ERROR "WAMR source not found at WAMR_ROOT_DIR=${WAMR_ROOT_DIR}\n"
                       "Pass -DWAMR_ROOT_DIR=<wasm-micro-runtime source>.")
endif()
message(STATUS "WAMR_ROOT_DIR = ${WAMR_ROOT_DIR}")

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
set(WAMR_BUILD_TARGET    "X86_64")
set(WAMR_BUILD_INTERP    1)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_AOT       0)   # interp only — matches the device config (CONFIG_WAMR_ENABLE_AOT=n)
set(WAMR_BUILD_JIT       0)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI  1)
set(WAMR_BUILD_REF_TYPES 1)   # quickjs.wasm (clang 22) emits reference-types table ops
set(WAMR_BUILD_PLATFORM  ${WAMR_BUILD_PLATFORM})
set(WAMR_ROOT_DIR ${WAMR_ROOT_DIR})

include("${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake")
add_library(vmlib STATIC ${WAMR_RUNTIME_LIB_SOURCE})
target_include_directories(vmlib PUBLIC
  ${WAMR_ROOT_DIR}/core/iwasm/include
  ${WAMR_ROOT_DIR}/core/shared/platform)

add_executable(host_test host_test.c)
target_link_libraries(host_test vmlib m pthread)

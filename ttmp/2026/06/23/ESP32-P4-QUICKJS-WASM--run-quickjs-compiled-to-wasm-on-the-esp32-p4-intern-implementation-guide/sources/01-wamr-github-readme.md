---
title: "WAMR GitHub README (bytecodealliance/wasm-micro-runtime)"
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

## WebAssembly Micro Runtime

**A [Bytecode Alliance](https://bytecodealliance.org/) project**

**[Guide](https://wamr.gitbook.io/)**    
**[Website](https://bytecodealliance.github.io/wamr.dev)**    
**[Chat](https://bytecodealliance.zulipchat.com/#narrow/stream/290350-wamr)**

[Build WAMR](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md) | 
[Build AOT 
Compiler](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/wamr-compiler/README.md) 
| [Embed WAMR](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md) 
| [Export Native 
API](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md) | 
[Build Wasm 
Apps](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md) | 
[Samples](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/README.md)

WebAssembly Micro Runtime (WAMR) is a lightweight standalone WebAssembly (Wasm) runtime with small 
footprint, high performance and highly configurable features for applications cross from embedded, 
IoT, edge to Trusted Execution Environment (TEE), smart contract, cloud native and so on. It 
includes a few parts as below:

- [**VMcore**](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/core/iwasm): A set 
of runtime libraries for loading and running Wasm modules. It supports rich running modes including 
interpreter, Ahead-of-Time compilation(AoT) and Just-in-Time compilation (JIT). WAMR supports two 
JIT tiers - Fast JIT, LLVM JIT, and dynamic tier-up from Fast JIT to LLVM JIT.
- [**iwasm**](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini): The 
executable binary built with WAMR VMcore which supports WASI and command line interface.
- [**wamrc**](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/wamr-compiler): The 
AOT compiler to compile Wasm file into AOT file
- Useful components and tools for building real solutions with WAMR vmcore:
	- 
[App-framework](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-framework/READM
E.md): A framework for supporting APIs for the Wasm applications
		- 
[App-manager](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-mgr/README.md): 
A framework for dynamical loading the Wasm module remotely
		- 
[WAMR-IDE](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/test-tools/wamr-ide): 
An experimental VSCode extension for developping WebAssembly applications with C/C++

### Key features

- Full compliant to the W3C Wasm MVP
- Small runtime binary size (core vmlib on cortex-m4f with tail-call/bulk memory/shared memory 
support, text size from bloaty)
	- ~58.9K for fast interpreter
		- ~56.3K for classic interpreter
		- ~29.4K for aot runtime
		- ~21.4K for libc-wasi library
		- ~3.7K for libc-builtin library
- Near to native speed by AOT and JIT
- Self-implemented AOT module loader to enable AOT working on Linux, Windows, MacOS, Android, SGX 
and MCU systems
- Choices of Wasm application libc support: the built-in libc subset for the embedded environment 
or [WASI](https://github.com/WebAssembly/WASI) for the standard libc
- [The simple C APIs to embed WAMR into host 
environment](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md), 
see [how to integrate 
WAMR](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md) and the 
[API 
list](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/core/iwasm/include/wasm_expor
t.h)
- [The mechanism to export native APIs to Wasm 
applications](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api
.md), see [how to register native 
APIs](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md)
- [Multiple modules as 
dependencies](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/multi_module.md),
 ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/multi_module.md) 
and [sample](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/multi-module)
- [Multi-thread, pthread APIs and thread 
management](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/pthread_library.md)
, ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/pthread_library.md) 
and [sample](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/multi-thread)
- 
[wasi-threads](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/pthread_impls.md
#wasi-threads-new), ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/pthread_impls.md#was
i-threads-new) and 
[sample](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/wasi-threads)
- [Linux SGX (Intel Software Guard Extension) 
support](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md), ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md)
- [Source debugging 
support](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/source_debugging.md), 
ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/source_debugging.md)
- [XIP (Execution In Place) 
support](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/xip.md), ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/xip.md)
- [Berkeley/Posix Socket 
support](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/socket_api.md), ref 
to [document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/socket_api.md) 
and [sample](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/socket-api)
- [Multi-tier 
JIT](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini#linux) and 
[Running mode 
control](https://bytecodealliance.github.io/wamr.dev/blog/introduction-to-wamr-running-modes/)
- Language bindings: 
[Go](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/language-bindings/go/README.md
), 
[Python](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/language-bindings/python/R
EADME.md), 
[Rust](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/language-bindings/rust/READM
E.md)

### Wasm post-MVP features

- [wasm-c-api](https://github.com/WebAssembly/wasm-c-api), ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/wasm_c_api.md) and 
[sample](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/wasm-c-api)
- [128-bit SIMD](https://github.com/WebAssembly/simd), ref to 
[samples/workload](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/workload
)
- [Reference Types](https://github.com/WebAssembly/reference-types), ref to 
[document](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/ref_types.md) and 
[sample](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples/ref-types)
- [Bulk memory operations](https://github.com/WebAssembly/bulk-memory-operations), [Shared 
memory](https://github.com/WebAssembly/threads/blob/main/proposals/threads/Overview.md#shared-linear
-memory), [Memory64](https://github.com/WebAssembly/memory64)
- [Tail-call](https://github.com/WebAssembly/tail-call), [Garbage 
Collection](https://github.com/WebAssembly/gc), [Exception 
Handling](https://github.com/WebAssembly/exception-handling), [Branch 
Hinting](https://github.com/WebAssembly/branch-hinting)
- [Extended Constant Expressions](https://github.com/WebAssembly/extended-const)

### Supported architectures and platforms

The WAMR VMcore supports the following architectures:

- X86-64, X86-32
- ARM, THUMB (ARMV7 Cortex-M7 and Cortex-A15 are tested)
- AArch64 (Cortex-A57 and Cortex-A53 are tested)
- RISCV64, RISCV32 (RISC-V LP64 and RISC-V LP64D are tested)
- XTENSA, MIPS, ARC

The following platforms are supported, click each link below for how to build iwasm on that 
platform. Refer to [WAMR porting 
guide](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/port_wamr.md) for how 
to port WAMR to a new platform.

- 
[Linux](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#linu
x), [Linux SGX (Intel Software Guard 
Extension)](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md), 
[MacOS](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#maco
s), 
[Android](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#an
droid), 
[Windows](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#wi
ndows), [Windows (MinGW, 
MSVC)](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#mingw
)
- 
[Zephyr](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#zep
hyr), 
[AliOS-Things](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.
md#alios-things), 
[VxWorks](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#vx
works), 
[NuttX](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#nutt
x), 
[RT-Thread](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#
RT-Thread), 
[ESP-IDF(FreeRTOS)](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/RE
ADME.md#esp-idf)

## Getting started

- [Build VM 
core](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md) and 
[Build wamrc AOT 
compiler](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/wamr-compiler/README.md)
- [Build iwasm (mini 
product)](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md): 
[Linux](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#linu
x), [SGX](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/linux_sgx.md), 
[MacOS](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#maco
s) and 
[Windows](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/product-mini/README.md#wi
ndows)
- [Embed into 
C/C++](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/embed_wamr.md), [Embed 
into 
Python](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/language-bindings/python), 
[Embed into 
Go](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/language-bindings/go), [Embed 
in Rust](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/language-bindings/rust)
- [Register native APIs for Wasm 
applications](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api
.md)
- [Build wamrc AOT 
compiler](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/wamr-compiler/README.md)
- [Build Wasm 
applications](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wasm_app.md
)
- [Port WAMR to a new 
platform](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/port_wamr.md)
- [VS Code development 
container](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/devcontainer.md)
- [Samples](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/samples) and 
[Benchmarks](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/tests/benchmarks)
- [End-user APIs documentation](https://bytecodealliance.github.io/wamr.dev/apis/)

### Performance and memory

- [Blog: The WAMR memory 
model](https://bytecodealliance.github.io/wamr.dev/blog/the-wamr-memory-model/)
- [Blog: Understand WAMR 
heaps](https://bytecodealliance.github.io/wamr.dev/blog/understand-the-wamr-heaps/) and 
[stacks](https://bytecodealliance.github.io/wamr.dev/blog/understand-the-wamr-stacks/)
- [Blog: Introduction to WAMR running 
modes](https://bytecodealliance.github.io/wamr.dev/blog/introduction-to-wamr-running-modes/)
- [Memory usage 
tuning](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/memory_tune.md): the 
memory model and how to tune the memory usage
- [Memory usage 
profiling](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/build_wamr.md#enable
-memory-profiling-experiment): how to profile the memory usage
- [Performance 
tuning](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/perf_tune.md): how to 
tune the performance
- [Benchmarks](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/tests/benchmarks): 
checkout these links for how to run the benchmarks: 
[PolyBench](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/tests/benchmarks/polybe
nch), 
[CoreMark](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/tests/benchmarks/coremar
k), 
[Sightglass](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/tests/benchmarks/sight
glass), 
[JetStream2](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/tests/benchmarks/jetst
ream)
- [Performance and footprint 
data](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/Performance): the performance and 
footprint data

## Project Technical Steering Committee

The [WAMR PTSC 
Charter](https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/TSC_Charter.md) governs 
the operations of the project TSC. The current TSC members:

- [dongsheng28849455](https://github.com/dongsheng28849455) - **Dongsheng Yan**, 
[dongsheng.yan@sony.com](mailto:dongsheng.yan@sony.com)
- [loganek](https://github.com/loganek) - **Marcin Kolny**, 
[mkolny@amazon.co.uk](mailto:mkolny@amazon.co.uk)
- [lum1n0us](https://github.com/lum1n0us) - **Liang He** ， 
[liang.he@intel.com](mailto:liang.he@intel.com)
- [no1wudi](https://github.com/no1wudi) **Qi Huang**, 
[huangqi3@xiaomi.com](mailto:huangqi3@xiaomi.com)
- [qinxk-inter](https://github.com/qinxk-inter) - **Xiaokang Qin** ， 
[xiaokang.qxk@antgroup.com](mailto:xiaokang.qxk@antgroup.com)
- [ttrenner](https://github.com/ttrenner) - **Trenner, Thomas** ， 
[trenner.thomas@siemens.com](mailto:trenner.thomas@siemens.com)
- [wei-tang](https://github.com/wei-tang) - **Wei Tang** ， 
[tangwei.tang@antgroup.com](mailto:tangwei.tang@antgroup.com)
- [wenyongh](https://github.com/wenyongh) - **Wenyong Huang** ， 
[wenyong.huang@intel.com](mailto:wenyong.huang@intel.com)
- [woodsmc](https://github.com/woodsmc) - **Woods, Chris** ， 
[chris.woods@siemens.com](mailto:chris.woods@siemens.com)
- [xujuntwt95329](https://github.com/xujuntwt95329) - **Jun Xu** ， 
[Jun1.Xu@intel.com](mailto:Jun1.Xu@intel.com)
- [xwang98](https://github.com/xwang98) - **Xin Wang** ， 
[xin.wang@intel.com](mailto:xin.wang@intel.com) (chair)
- [yamt](https://github.com/yamt) - **Takashi Yamamoto**, 
[yamamoto@midokura.com](mailto:yamamoto@midokura.com)

## License

WAMR uses the same license as LLVM: the `Apache 2.0 license` with the LLVM exception. See the 
LICENSE file for details. This license allows you to freely use, modify, distribute and sell your 
own products based on WAMR. Any contributions you make will be under the same license.

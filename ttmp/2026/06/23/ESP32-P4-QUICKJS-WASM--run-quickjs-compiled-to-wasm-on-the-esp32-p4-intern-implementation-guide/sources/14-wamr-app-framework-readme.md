---
title: "WAMR app-framework README (bytecodealliance/wamr-app-framework)"
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

## WebAssembly Micro Runtime Application Framework

**[Guide](https://wamr.gitbook.io/)**    
**[Website](https://bytecodealliance.github.io/wamr.dev)**    
**[Chat](https://bytecodealliance.zulipchat.com/#narrow/stream/290350-wamr)**

[Use 
WAMR-App-framework](https://github.com/bytecodealliance/wamr-app-framework/blob/main/doc/wamr_api.md
) | [Samples](https://github.com/bytecodealliance/wamr-app-framework/blob/main/samples/README.md)

WebAssembly Micro Runtime Application Framework (WAMR-App-framework) is a comprehensive framework 
for programming WebAssembly (Wasm) applications for device and IoT usages. The framework supports 
running multiple applications, that are based on the event driven programming model.

It includes a few parts as below:

- 
[App-framework](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-framework/READM
E.md): A framework for supporting APIs for the Wasm applications, and it include same app library:
	- 
[Timer](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-framework/base)
		- 
[Connection](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-framework/connecti
on)
		- 
[Sensor](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-framework/sensor)
		- 
[WGL](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-framework/wgl)
- 
[App-manager](https://github.com/bytecodealliance/wamr-app-framework/blob/main/app-mgr/README.md): 
a framework for dynamical loading the Wasm module remotely
- Useful components and tools for building real solutions with WAMR-App-framework
	- 
[Component-test](https://github.com/bytecodealliance/wamr-app-framework/blob/main/test-tools/compone
nt-test/README.md): A test suite to verify the basic components of WAMR work well in combination.
		- 
[IoT-APP-Store-Demo](https://github.com/bytecodealliance/wamr-app-framework/blob/main/test-tools/IoT
-APP-Store-Demo/README.md): A demo of Wasm application management portal for WAMR
		- [Assembly-script on 
WAMR](https://github.com/bytecodealliance/wamr-app-framework/blob/main/assembly-script/README.md): 
A project based on [Wasm Micro Runtime](https://github.com/bytecodealliance/wasm-micro-runtime) 
(WAMR) and [AssemblyScript](https://github.com/AssemblyScript/assemblyscript). It implements some 
of the `wamr app framework` in *assemblyscript*, which allows you to write some applications in 
*assemblyscript* and dynamically installed on *WAMR Runtime*
		- 
[WAMR-SDK](https://github.com/bytecodealliance/wamr-app-framework/blob/main/wamr-sdk/README.md): 
The **[WAMR SDK](https://github.com/bytecodealliance/wamr-app-framework/blob/main/wamr-sdk)** tools 
is helpful to integrate **WAMR** with your project and generate APP SDK for developing WASM apps. 
It supports menu configuration for selecting WAMR components and builds the WAMR to a SDK package 
that includes **runtime SDK** and **APP SDK**.

## Getting started

- Just try this sample: 
[simple](https://github.com/bytecodealliance/wamr-app-framework/blob/main/samples/simple/README.md)

## License

WAMR-App-framework uses the same license as LLVM: the `Apache 2.0 license` with the LLVM exception. 
See the LICENSE file for details. This license allows you to freely use, modify, distribute and 
sell your own products based on WAMR. Any contributions you make will be under the same license.

## More resources

- [Who use WAMR?](https://github.com/bytecodealliance/wasm-micro-runtime/wiki)
- [WAMR Blogs](https://bytecodealliance.github.io/wamr.dev/blog/)
- [Community news and events](https://bytecodealliance.github.io/wamr.dev/events/)
- [WAMR TSC meetings](https://github.com/bytecodealliance/wasm-micro-runtime/wiki/TSC-meeting-notes)

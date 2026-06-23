---
title: "QuickJS home (bellard.org)"
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

## News

- 2026-06-04:
	- New release ([Changelog](https://bellard.org/quickjs/Changelog)). It is 42% faster than 
the previous release based on the bench-v8 (aka v8-v7) score.
- 2025-09-13:
	- New release ([Changelog](https://bellard.org/quickjs/Changelog))
- 2025-04-26:
	- New release ([Changelog](https://bellard.org/quickjs/Changelog)). The bignum extensions 
and the qjscalc application were removed to simplify the code. The [BFCalc 
calculator](https://bellard.org/libbf) ([web version](http://numcalc.com/)) can be used as a 
replacement for qjscalc.

## Introduction

QuickJS is a small and embeddable Javascript engine. It supports the 
[ES2025](https://tc39.github.io/ecma262/2025) specification.

Main Features:

- Small and easily embeddable: just a few C files, no external dependency, 367 KiB of x86 code for 
a simple hello world program.
- Fast interpreter with very low startup time: runs the [ECMAScript Test 
Suite](https://github.com/tc39/test262) in about 2 minutes on a single core of a desktop PC. The 
complete life cycle of a runtime instance completes in less than 300 microseconds.
- Almost complete [ES2025](https://tc39.github.io/ecma262/2023) support including modules, 
asynchronous generators and full Annex B support (legacy web compatibility).
- Passes nearly 100% of the ECMAScript Test Suite tests when selecting the ES2025 features (see 
[test262.fyi](https://test262.fyi/)).
- Can compile Javascript sources to executables with no external dependency.
- Garbage collection using reference counting (to reduce memory usage and have deterministic 
behavior) with cycle removal.
- Command line interpreter with contextual colorization implemented in Javascript.
- Small built-in standard library with C library wrappers.

## Online Demo

qjs can be run in [JSLinux](https://bellard.org/jslinux/vm.html?url=alpine-x86.cfg).

## Benchmarks

- [JavaScript engines zoo](https://zoo.js.org/?arch=amd64&v8=true)
- [Boa benchmarks](https://boajs.dev/benchmarks)
- [ahaoboy/js-engine-benchmark](https://github.com/ahaoboy/js-engine-benchmark)

## Documentation

QuickJS documentation: [HTML version](https://bellard.org/quickjs/quickjs.html), [PDF 
version](https://bellard.org/quickjs/quickjs.pdf).

## Download

- QuickJS source code: 
[quickjs-2026-06-04.tar.xz](https://bellard.org/quickjs/quickjs-2026-06-04.tar.xz)
- QuickJS extras (contain the unicode files needed to rebuild the unicode tables and the bench-v8 
benchmark): 
[quickjs-extras-2026-06-04.tar.xz](https://bellard.org/quickjs/quickjs-extras-2026-06-04.tar.xz)
- Official [GitHub repository](https://github.com/bellard/quickjs).
- Binary releases are available [here](https://bellard.org/quickjs/binary_releases).
- [Cosmopolitan](https://github.com/jart/cosmopolitan) binaries running on Linux, Mac, Windows, 
FreeBSD, OpenBSD, NetBSD for both the ARM64 and x86\_64 architectures: 
[quickjs-cosmo-2026-06-04.zip](https://bellard.org/quickjs/binary_releases/quickjs-cosmo-2026-06-04.
zip).
- Typescript compiler compiled with QuickJS: 
[quickjs-typescript-5.9.3-linux-x86.tar.xz](https://bellard.org/quickjs/quickjs-typescript-5.9.3-lin
ux-x86.tar.xz)
- Babel compiler compiled with QuickJS: 
[quickjs-babel-linux-x86.tar.xz](https://bellard.org/quickjs/quickjs-babel-linux-x86.tar.xz)

## Sub-projects

QuickJS embeds the following C libraries which can be used in other projects:
- **libregexp**: small and fast regexp library fully compliant with the Javascript ES2023 
specification.
- **libunicode**: small unicode library supporting case conversion, unicode normalization, unicode 
script queries, unicode general category queries and all unicode binary properties.
- **dtoa**: small library implementing float64 printing and parsing.

## Links

- [Micro QuickJS](https://github.com/bellard/mquickjs): a Javascript engine for microcontrollers.

## Licensing

QuickJS is released under the [MIT license](https://opensource.org/licenses/MIT).

Unless otherwise specified, the QuickJS sources are copyright Fabrice Bellard and Charlie Gordon.

---

Fabrice Bellard - [https://bellard.org/](https://bellard.org/)

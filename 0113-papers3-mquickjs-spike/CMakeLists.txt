# Phase 11 MicroQuickJS feasibility spike (ESP-50-PAPERS3-EREADER-PRIMITIVES).
# Deliberately separate from 0112-papers3-reader-primitives: the engine is
# NOT linked into the production reader path (task atze).
cmake_minimum_required(VERSION 3.16)

# mqjs_service comes from the repo-shared components; the mquickjs engine is
# the project-local copy (its mquickjs_atom.h is generated from THIS spike's
# stdlib and must not be shared).
set(EXTRA_COMPONENT_DIRS "../components")
# Only build main's dependency closure: the shared components dir contains
# unrelated components (some with unmet requirements like M5GFX).
# esp_psram must be named explicitly or its Kconfig symbols (CONFIG_SPIRAM)
# don't exist and the defaults are silently dropped.
set(COMPONENTS main esp_psram)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(papers3_mquickjs_spike)

ROOT := ../../..
QJS_DIR := $(ROOT)/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
MAIN_DIR := $(ROOT)/0103-atoms3r-m12-native-quickjs/main
BUILD := build
TARGET := $(BUILD)/qjs-http-host
CXX ?= g++
CC ?= gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -I$(QJS_DIR) -I$(MAIN_DIR)
CFLAGS := -O2 -w -D_GNU_SOURCE -DCONFIG_VERSION=\"2026-06-25\" -DHAVE_CLOSEFROM -I$(QJS_DIR)
LDFLAGS := -lm -ldl -lpthread
HOST_CPP_SRCS := src/main.cpp src/host_http_ops.cpp
CORE_CPP_SRCS := $(MAIN_DIR)/http_namespace_core.cpp
C_SRCS := $(QJS_DIR)/quickjs.c $(QJS_DIR)/cutils.c $(QJS_DIR)/dtoa.c $(QJS_DIR)/libregexp.c $(QJS_DIR)/libunicode.c
HOST_CPP_OBJS := $(HOST_CPP_SRCS:%.cpp=$(BUILD)/%.o)
CORE_CPP_OBJS := $(BUILD)/core/http_namespace_core.o
C_OBJS := $(patsubst $(QJS_DIR)/%.c,$(BUILD)/quickjs/%.o,$(C_SRCS))

.PHONY: all clean smoke
all: $(TARGET)

$(TARGET): $(HOST_CPP_OBJS) $(CORE_CPP_OBJS) $(C_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BUILD)/src/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/core/http_namespace_core.o: $(MAIN_DIR)/http_namespace_core.cpp $(MAIN_DIR)/http_namespace_core.h
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/quickjs/%.o: $(QJS_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

smoke: $(TARGET)
	./tests/run-smoke.sh

clean:
	rm -rf $(BUILD)

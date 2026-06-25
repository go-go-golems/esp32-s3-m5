ROOT := ../../../..
QJS_DIR := $(ROOT)/0100-esp32-p4-quickjs-wasm/wasm-src/quickjs
BUILD := build
TARGET := $(BUILD)/picojs-host
CXX ?= g++
CC ?= gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -I$(QJS_DIR)
CFLAGS := -O2 -w -D_GNU_SOURCE -DCONFIG_VERSION=\"2026-06-04\" -DHAVE_CLOSEFROM -I$(QJS_DIR)
LDFLAGS := -lm -ldl -lpthread
CPP_SRCS := src/main.cpp src/pico_native_api.cpp
C_SRCS := $(QJS_DIR)/quickjs.c $(QJS_DIR)/cutils.c $(QJS_DIR)/dtoa.c $(QJS_DIR)/libregexp.c $(QJS_DIR)/libunicode.c
CPP_OBJS := $(CPP_SRCS:%.cpp=$(BUILD)/%.o)
C_OBJS := $(patsubst $(QJS_DIR)/%.c,$(BUILD)/quickjs/%.o,$(C_SRCS))

.PHONY: all clean run
all: $(TARGET)

$(TARGET): $(CPP_OBJS) $(C_OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(BUILD)/src/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/quickjs/%.o: $(QJS_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	$(TARGET) hello-native

clean:
	rm -rf $(BUILD)

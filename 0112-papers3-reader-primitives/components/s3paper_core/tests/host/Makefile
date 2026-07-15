# Host tests for s3paper_core (no ESP-IDF required).
# Usage: make && ./s3paper_core_tests
CXX ?= g++
CXXFLAGS ?= -std=c++17 -g -O1 -Wall -Wextra -Werror \
            -fsanitize=address,undefined -fno-omit-frame-pointer
INCLUDES = -I../../include
SRCS = ../../src/status.cpp ../../src/geometry.cpp ../../src/frame_arena.cpp \
       ../../src/frame_builder.cpp ../../src/fake_backend.cpp \
       ../../src/refresh_planner.cpp ../../src/input.cpp ../../src/text.cpp ../../src/content.cpp ../../src/paginator.cpp \
       test_main.cpp

s3paper_core_tests: $(SRCS) $(wildcard ../../include/s3paper/*.h)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $@

.PHONY: run clean
run: s3paper_core_tests
	./s3paper_core_tests

clean:
	rm -f s3paper_core_tests

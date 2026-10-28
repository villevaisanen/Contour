# Local SDK default; override with make RACK_DIR=/path/to/Rack-SDK.
RACK_DIR ?= $(HOME)/Rack-SDK

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res README.md tests/Scope.vcv
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

.PHONY: test
TEST_CXXFLAGS = -std=c++11 -O2 -Wall -Wextra -Wno-unused-parameter -pthread
build/tests/core_test: tests/core_test.cpp src/core.hpp
	mkdir -p build/tests
	$(CXX) $(TEST_CXXFLAGS) $< -o $@
build/tests/rack_test: tests/rack_test.cpp src/Contour.cpp src/core.hpp src/plugin.hpp
	mkdir -p build/tests
	$(CXX) $(TEST_CXXFLAGS) -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include $< -L$(RACK_DIR) -Wl,-rpath,$(abspath $(RACK_DIR)) -lRack -o $@
test: build/tests/core_test build/tests/rack_test
	./build/tests/core_test
	./build/tests/rack_test

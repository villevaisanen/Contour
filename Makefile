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
DISTRIBUTABLES += res README.md docs/contour_screenshot.png
DISTRIBUTABLES += licence.txt
DISTRIBUTABLES += $(wildcard presets)

# SDK-free goals must not parse plugin.mk (or require an installed SDK).
.DEFAULT_GOAL := all
CORE_GOALS := test-core test-stack test-equivalence build/tests/core_test build/tests/stack_test
ifneq ($(strip $(MAKECMDGOALS)),)
ifeq ($(strip $(filter-out $(CORE_GOALS),$(MAKECMDGOALS))),)
SKIP_RACK_SDK := 1
endif
endif
ifeq ($(SKIP_RACK_SDK),)
include $(RACK_DIR)/plugin.mk
endif

# -UNDEBUG is last so assertions remain active even with user release flags.
TEST_CXXFLAGS := -std=c++11 -O2 -Wall -Wextra -Wno-unused-parameter -pthread
TEST_FLAGS = $(TEST_CXXFLAGS) -UNDEBUG
.PHONY: test test-core test-stack test-rack test-equivalence
build/tests/core_test: tests/core_test.cpp src/core.hpp
	mkdir -p build/tests
	$(CXX) $(TEST_FLAGS) $< -o $@
build/tests/stack_test: tests/stack_test.cpp src/core.hpp tests/terrain_checks.hpp
	mkdir -p build/tests
	$(CXX) $(TEST_FLAGS) $< -o $@
build/tests/rack_test: tests/rack_test.cpp src/Contour.cpp src/core.hpp src/plugin.hpp
	mkdir -p build/tests
	$(CXX) $(TEST_FLAGS) -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include $< -L$(RACK_DIR) -Wl,-rpath,$(abspath $(RACK_DIR)) -lRack -o $@
test-stack: build/tests/stack_test
	./build/tests/stack_test
test-core: build/tests/core_test test-stack
	./build/tests/core_test
test-rack: build/tests/rack_test
	./build/tests/rack_test
test-equivalence:
	CXX="$(CXX)" python3 tests/compare_terrain.py
test: test-core test-rack

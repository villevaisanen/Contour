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
SOURCES += $(wildcard src/*.cpp) $(wildcard src/orbitals/*.cpp) $(wildcard src/orbitals/ui/*.cpp)

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res README.md CHANGELOG.md docs
DISTRIBUTABLES += licence.txt LICENSES.md
DISTRIBUTABLES += $(wildcard presets)

# SDK-free goals must not parse plugin.mk (or require an installed SDK).
.DEFAULT_GOAL := all
CORE_GOALS := test-orbitals test-orbitals-visualization test-audio-regression spectrum test-migration  test-core test-stack test-equivalence build/tests/core_test build/tests/stack_test
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
test-equivalence:
	CXX="$(CXX)" python3 tests/compare_terrain.py
test: test-core test-rack test-orbitals test-orbitals-visualization test-audio-regression test-migration

ORBITALS_HEADERS := $(wildcard src/orbitals/dsp/*.hpp) $(wildcard src/orbitals/ui/*.hpp)
.PHONY: test-orbitals test-orbitals-visualization test-audio-regression test-migration spectrum
build/tests/orbitals/math: tests/orbitals/test.cpp $(ORBITALS_HEADERS)
	mkdir -p build/tests/orbitals
	$(CXX) $(TEST_FLAGS) $< -o $@
build/tests/orbitals/visualization: tests/orbitals/visualization.cpp $(ORBITALS_HEADERS)
	mkdir -p build/tests/orbitals
	$(CXX) $(TEST_FLAGS) $< -o $@
build/tests/orbitals/rack: tests/orbitals/rack.cpp src/orbitals/OrbitalModule.hpp $(ORBITALS_HEADERS) plugin.so
	mkdir -p build/tests/orbitals
	$(CXX) $(TEST_FLAGS) -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include $< -L$(RACK_DIR) -Wl,-rpath,$(abspath $(RACK_DIR)) -lRack -ldl -o $@
build/tests/plugin_test: tests/plugin_test.cpp plugin.so
	mkdir -p build/tests
	$(CXX) $(TEST_FLAGS) -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include $< -L$(RACK_DIR) -Wl,-rpath,$(abspath $(RACK_DIR)) -lRack -ldl -o $@
test-orbitals: build/tests/orbitals/math
	./build/tests/orbitals/math
test-orbitals-visualization: build/tests/orbitals/visualization
	./build/tests/orbitals/visualization
test-rack: build/tests/rack_test build/tests/orbitals/rack build/tests/plugin_test
	./build/tests/rack_test
	./build/tests/orbitals/rack
	./build/tests/plugin_test
test-audio-regression:
	CXX="$(CXX)" python3 tests/orbitals/audio-regression.py
test-migration:
	python3 tests/check_migration.py
spectrum:
	mkdir -p build/tests/orbitals
	$(CXX) $(TEST_FLAGS) tests/orbitals/spectrum.cpp -o build/tests/orbitals/spectrum
	./build/tests/orbitals/spectrum
	python3 tests/orbitals/spectrum.py

# Optional GUI validation. Never uses the user's Rack profile.
RACK_SYSTEM_DIR ?= $(HOME)/Rack2Free
.PHONY: test-gui
build/tests/theme_test: tests/theme_test.cpp plugin.so
	mkdir -p build/tests
	$(CXX) $(TEST_FLAGS) -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include $< -L$(RACK_DIR) -Wl,-rpath,$(abspath $(RACK_DIR)) -lRack -lGL -ldl -o $@
test-gui: build/tests/theme_test
	mkdir -p build/tests/theme-profile
	./build/tests/theme_test "$(RACK_SYSTEM_DIR)" "$(abspath build/tests/theme-profile)"

.PHONY: test-package
test-package: dist
	python3 tests/check_migration.py --package

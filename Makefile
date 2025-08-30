CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g
INCLUDES = -Ilib -Ivendor/swaptube/src -I/opt/homebrew/include $(shell pkg-config --cflags sfml-all libcjson zlib)
LDFLAGS = $(shell pkg-config --libs sfml-all libcjson zlib)

SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build
BIN_DIR = bin

LIB_SOURCES = $(wildcard $(LIB_DIR)/*.cpp)
LIB_OBJECTS = $(LIB_SOURCES:$(LIB_DIR)/%.cpp=$(BUILD_DIR)/%.o)

MAIN_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
TARGETS = $(MAIN_SOURCES:$(SRC_DIR)/%.cpp=$(BIN_DIR)/%)

HEADERS = $(wildcard $(LIB_DIR)/*.hpp)

SOURCES = $(LIB_SOURCES) $(MAIN_SOURCES)

.PHONY: all clean install-deps

all: $(TARGETS)

# Compile library sources (only rebuild if source or its dependencies changed)
$(BUILD_DIR)/%.o: $(LIB_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Link final executables (only rebuild if objects or main source changed)
$(BIN_DIR)/%: $(SRC_DIR)/%.cpp $(LIB_OBJECTS)
	@echo "Linking $@..."
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIB_OBJECTS) $(LDFLAGS) -o $@

# Include dependency files for accurate rebuilding
-include $(LIB_OBJECTS:.o=.d)

install-deps:
	@echo "Installing dependencies..."
	@if ! command -v brew >/dev/null 2>&1; then \
		echo "Homebrew not found. Please install Homebrew first."; \
		exit 1; \
	fi
	brew install sfml cjson glm

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

test: all
	@echo "Running graphew..."
	./$(BIN_DIR)/graphew
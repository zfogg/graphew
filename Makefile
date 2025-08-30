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

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/%: $(SRC_DIR)/%.cpp $(HEADERS) $(LIB_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LIB_OBJECTS) $(LDFLAGS) -o $@

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
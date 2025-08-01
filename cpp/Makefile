# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows系统
    TARGET = back.exe
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
    PATHSEP = \\
    EXECUTABLE_PREFIX = 
else
    # Linux/Unix系统
    TARGET = back
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
    PATHSEP = /
    EXECUTABLE_PREFIX = ./
endif

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -I$(SRC_DIR)

# Directories
SRC_DIR = src
BUILD_DIR = .build
OUTPUT_DIR = output
TEST_DIR = test

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Test files
TEST_ASTS = $(wildcard $(TEST_DIR)/*.ast)
TEST_ASMS = $(TEST_ASTS:$(TEST_DIR)/%.ast=$(OUTPUT_DIR)/%.asm)

# Default target
build: $(TARGET)

# Create directories
$(BUILD_DIR):
ifeq ($(OS),Windows_NT)
	if not exist $(BUILD_DIR) $(MKDIR) $(BUILD_DIR)
else
	$(MKDIR) $(BUILD_DIR)
endif

$(OUTPUT_DIR):
ifeq ($(OS),Windows_NT)
	if not exist $(OUTPUT_DIR) $(MKDIR) $(OUTPUT_DIR)
else
	$(MKDIR) $(OUTPUT_DIR)
endif


# Build the compiler
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)
	@echo "build successfully: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the compiler (build and run with specified file)
run: $(TARGET) | $(OUTPUT_DIR)
	@echo "Note: Use 'make test' to run all tests, or run manually:"
	@echo "  Linux: cat test/filename.ast | ./$(TARGET) > output/filename.asm"
	@echo "  Windows: type test\\filename.ast | $(TARGET) > output\\filename.asm"

# Catch-all rule to prevent make from trying to build the filename as a target
%:
	@:

# Test the compiler (build and test all AST files)
test: $(TEST_ASMS)
	@echo "Test finished. Generated $(words $(TEST_ASMS)) ASM files in $(OUTPUT_DIR)/"

# Pattern rule to compile any .ast file to .asm (now using stdin/stdout)
$(OUTPUT_DIR)/%.asm: $(TEST_DIR)/%.ast $(TARGET) | $(OUTPUT_DIR)
	@echo "Compile $< -> $@"
ifeq ($(OS),Windows_NT)
	type $< | $(TARGET) > $@
else
	cat $< | $(EXECUTABLE_PREFIX)$(TARGET) > $@
endif


# Clean all generated files
clean:
ifeq ($(OS),Windows_NT)
	if exist $(BUILD_DIR) $(RMDIR) $(BUILD_DIR)
	if exist $(TARGET) $(RM) $(TARGET)
	if exist $(OUTPUT_DIR) $(RMDIR) $(OUTPUT_DIR)
else
	$(RMDIR) $(BUILD_DIR)
	$(RM) $(TARGET)
	$(RMDIR) $(OUTPUT_DIR)

endif
	@echo "All cleaned up."

# Help
help:
	@echo "Available command:"
	@echo "  build  - build compiler"
	@echo "  test   - build and test with all test files (cross-platform)"
	@echo "  clean  - clean all build and output files"
	@echo "  help   - show this help information"
	@echo ""
	@echo "Manual usage:"
ifeq ($(OS),Windows_NT)
	@echo "  type test\\filename.ast | $(TARGET) > output\\filename.asm"
else
	@echo "  cat test/filename.ast | ./$(TARGET) > output/filename.asm"
endif

# Phony targets
.PHONY: build run test clean help
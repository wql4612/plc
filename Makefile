ifeq ($(OS),Windows_NT)
	FRONT_NAME = front.exe
	BACK_NAME = back.exe
	COMPILER_NAME = compiler.exe
	RM = del /Q
	CP = copy
	CXX = g++
	MKDIR = if not exist
	MKDIR_CMD = mkdir
else
	FRONT_NAME = front
	BACK_NAME = back
	COMPILER_NAME = compiler
	RM = rm -f
	CP = cp
	CXX = g++
	MKDIR = mkdir -p
	MKDIR_CMD = $(MKDIR)
endif

# 定义测试相关变量
TESTS_DIR = tests
OUTPUT_DIR = output
TEST_FILES = $(sort $(wildcard $(TESTS_DIR)/*.tc))
OUTPUT_FILES = $(patsubst $(TESTS_DIR)/%.tc,$(OUTPUT_DIR)/%.s,$(TEST_FILES))


check-tools:
	@command -v gcc >/dev/null 2>&1 || { echo "gcc not found! Please install gcc."; exit 1; }
	@command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 || { echo "riscv64-unknown-elf-gcc not found! Please install riscv toolchain."; exit 1; }
	@command -v qemu-riscv64 >/dev/null 2>&1 || { echo "qemu-riscv64 not found! Please install qemu."; exit 1; }

build: build-frontend build-backend build-center
	@echo "Over!"

build-frontend:
	@echo "Build frontend interpreter..."
	cd toyc-interpreter && dune build
	$(CP) toyc-interpreter/_build/default/bin/main.exe ./$(FRONT_NAME)
	@echo "Copy frontend executable to root directory as $(FRONT_NAME)"

build-backend:
	@echo "Build backend compiler..."
	cd cpp && make build
ifeq ($(OS),Windows_NT)
	$(CP) cpp\\back.exe .\\$(BACK_NAME)
else
	$(CP) cpp/back ./$(BACK_NAME)
endif
	@echo "Copy backend executable to root directory as $(BACK_NAME)"

build-center:
	@echo "Build center program..."
	$(CXX) -o $(COMPILER_NAME) compiler.cpp
	@echo "Build center program as $(COMPILER_NAME)"

# 创建输出目录
$(OUTPUT_DIR):
ifeq ($(OS),Windows_NT)
	@if not exist $(OUTPUT_DIR) mkdir $(OUTPUT_DIR)
else
	@mkdir -p $(OUTPUT_DIR)
endif

# 测试功能

# 只生成汇编，不依赖 riscv/qemu
test: build $(OUTPUT_DIR)
	@echo "Running tests..."
	@echo "Found $(words $(TEST_FILES)) test files."
ifeq ($(OS),Windows_NT)
	@for %%f in ($(subst /,\\,$(TEST_FILES))) do ( \
		echo "Testing: %%f" && \
		( \
			type "%%f" | $(COMPILER_NAME) > "$(OUTPUT_DIR)\%%~nf.s" 2> temp_error.txt || \
			( \
				echo "Error when testing %%f:" && \
				type temp_error.txt && \
				echo "" \
			) \
		) \
	)
	@if exist temp_error.txt $(RM) temp_error.txt
else
	@for test_file in $(TEST_FILES); do \
		echo "Testing: $$test_file"; \
		base_name=$$(basename $$test_file .tc); \
		s_file=$(OUTPUT_DIR)/$$base_name.s; \
		cat $$test_file | ./$(COMPILER_NAME) > $$s_file 2> /tmp/compiler_error.txt || { \
			echo "Error when compiling $$test_file to asm:"; \
			cat /tmp/compiler_error.txt; \
			echo ""; \
			continue; \
		}; \
		echo "Generated: $$s_file"; \
	done
endif
	@echo "compile finish.For check please install riscv64-unknown-elf-gcc & qemu-riscv64, then execute 'make test-full' "


# 全流程测试，依赖 riscv32/qemu
test-full: check-tools build $(OUTPUT_DIR)
	@echo "Running tests..."
	@echo "Found $(words $(TEST_FILES)) test files."
ifeq ($(OS),Windows_NT)
	@for %%f in ($(subst /,\\,$(TEST_FILES))) do ( \
		echo "Testing: %%f" && \
		( \
			type "%%f" | $(COMPILER_NAME) > "$(OUTPUT_DIR)\%%~nf.s" 2> temp_error.txt || \
			( \
				echo "Error when testing %%f:" && \
				type temp_error.txt && \
				echo "" \
			) \
		) \
	)
	@if exist temp_error.txt $(RM) temp_error.txt
else
	@for test_file in $(TEST_FILES); do \
		echo "Testing: $$test_file"; \
		base_name=$$(basename $$test_file .tc); \
		s_file=$(OUTPUT_DIR)/$$base_name.s; \
		elf_file=$(OUTPUT_DIR)/$$base_name.elf; \
		native_file=$(OUTPUT_DIR)/$$base_name.native; \
		cat $$test_file | ./$(COMPILER_NAME) > $$s_file 2> /tmp/compiler_error.txt || { \
			echo "Error when compiling $$test_file to asm:"; \
			cat /tmp/compiler_error.txt; \
			echo ""; \
			continue; \
		}; \
		riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -nostdlib -static -o $$elf_file $$s_file 2> /tmp/gcc_error.txt || { \
			echo "Error assembling $$s_file:"; \
			cat /tmp/gcc_error.txt; \
			echo ""; \
			continue; \
		}; \
		gcc -std=c++20 -x c -o $$native_file $$test_file 2> /tmp/native_gcc_error.txt || { \
			echo "Error compiling native $$test_file:"; \
			cat /tmp/native_gcc_error.txt; \
			echo ""; \
			continue; \
		}; \
		qemu-riscv32 $$elf_file; riscv_ret=$$?; \
		$$native_file; native_ret=$$?; \
		if [ "$$riscv_ret" -eq "$$native_ret" ]; then \
			echo "PASS: $$test_file"; \
		else \
			echo "FAIL: $$test_file (native=$$native_ret, riscv=$$riscv_ret)"; \
		fi; \
	done
endif
	@echo "Testing completed. Results are in $(OUTPUT_DIR)/"

clean:
	@echo "Clean begin"
	cd toyc-interpreter && dune clean
	cd cpp && make clean
	$(RM) $(FRONT_NAME) $(BACK_NAME) $(COMPILER_NAME)
	$(RM) $(wildcard *.ast) $(wildcard *.asm) ./code.tc
ifeq ($(OS),Windows_NT)
	@if exist $(OUTPUT_DIR) rmdir /S /Q $(OUTPUT_DIR)
else
	rm -rf $(OUTPUT_DIR)
endif
	@echo "clean completed"

.PHONY: build build-frontend build-backend build-center clean test
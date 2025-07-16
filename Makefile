# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows系统
    FRONT_NAME = front.exe
    BACK_NAME = back.exe
    COMPILER_NAME = compiler.exe
    RM = del /Q
    CP = copy
    CXX = g++
else
    # Linux/Unix系统
    FRONT_NAME = front
    BACK_NAME = back
    COMPILER_NAME = compiler
    RM = rm -f
    CP = cp
    CXX = g++
endif

# 构建整个翻译器
build: build-frontend build-backend build-center
	@echo "Over!"

# 构建前端OCaml项目
build-frontend:
	@echo "Build frontend interpreter..."
	cd toyc-interpreter && dune build
	$(CP) toyc-interpreter/_build/default/bin/main.exe ./$(FRONT_NAME)
	@echo "Copy frontend executable to root directory as $(FRONT_NAME)"

# 构建后端C++编译器
build-backend:
	@echo "Build backend compiler..."
	cd cpp && make build
ifeq ($(OS),Windows_NT)
	$(CP) cpp\\back.exe .\\$(BACK_NAME)
else
	$(CP) cpp/back ./$(BACK_NAME)
endif
	@echo "Copy backend executable to root directory as $(BACK_NAME)"

# 构建center程序
build-center:
	@echo "Build center program..."
	$(CXX) -o $(COMPILER_NAME) compiler.cpp
	@echo "Build center program as $(COMPILER_NAME)"
# 清理所有生成的文件
clean:
	@echo "Clean begin"
	cd toyc-interpreter && dune clean
	cd cpp && make clean
	$(RM) $(FRONT_NAME) $(BACK_NAME) $(COMPILER_NAME)
	$(RM) $(wildcard *.ast) $(wildcard *.asm) ./code.tc
	@echo "clean completed"

# 伪目标
.PHONY: build build-frontend build-backend clean
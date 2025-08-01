cmake_minimum_required(VERSION 3.10)
project(toyc_compiler CXX)

# 构建 C++ 后端（直接调用 make）
add_custom_command(
    OUTPUT cpp/back
    COMMAND make build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cpp
    COMMENT "Building C++ backend with make"
)


# 构建 C++ 后端（直接调用 make）
add_custom_command(
    OUTPUT cpp/back
    COMMAND make build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/cpp
    COMMENT "Building C++ backend with make"
)
add_custom_target(backend ALL DEPENDS cpp/back)



# 拷贝 C++ 后端到源码根目录
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/back
    COMMAND cp ${CMAKE_SOURCE_DIR}/cpp/back ${CMAKE_SOURCE_DIR}/back
    DEPENDS cpp/back
    COMMENT "Copying backend executable to source directory"
)
add_custom_target(copy_backend ALL DEPENDS ${CMAKE_SOURCE_DIR}/back)


# 构建 OCaml 前端（通过 dune）并拷贝到源码根目录
add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/front
    COMMAND dune build
    COMMAND cp ${CMAKE_SOURCE_DIR}/toyc-interpreter/_build/default/bin/main.exe ${CMAKE_SOURCE_DIR}/front
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/toyc-interpreter
    COMMENT "Copying frontend executable to source directory"
)
add_custom_target(frontend ALL DEPENDS ${CMAKE_SOURCE_DIR}/front)


# 构建中心 compiler.cpp 到 build 目录
add_executable(compiler compiler.cpp)
set_target_properties(compiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 依赖关系
add_dependencies(compiler frontend copy_backend)
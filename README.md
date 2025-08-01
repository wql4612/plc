# Compiler
## 小组成员
- 武秋霖：信息待补充
- 程英浩：信息待补充
- 徐松林：信息待补充
- 黄健：信息待补充

## 实验方法
为实现ToyC语言的编译器和性能优化，小组内初步讨论，决定综合Ocaml的模式匹配能力和C++的底层优化能力，构建前端-后端分离的编译器，实现正确快速高效的编译器。

## 参考思路及实现过程

## 文件结构
- cpp:包含用c++20实现的AST到RISCV汇编指令的项目代码。
- toyc-interpreter:包含用ocaml实现的源代码到AST的项目代码。
- center.cpp:项目的链接程序，可以将输入转为tc文件，并调用相应的处理程序进行处理。
- README.md:包含对项目的介绍。
- Makefile:项目构建脚本。

## 使用方法

本项目的 Makefile 支持 Windows（cmd/MSYS2）和 Linux/WSL 环境，会自动识别操作系统类型并执行对应脚本。
若需要使用test-full功能进行自动化检查，请安装 riscv64-unknown-elf-gcc 和 qemu-riscv64 的环境：
```
// 对于ubuntu20.04以上版本
sudo apt update
sudo apt install build-essential gcc make perl dkms git gcc-riscv64-unknown-elf gdb-multiarch qemu-system-misc
sudo apt install gcc-riscv64-linux-gnu
sudo apt install qemu-user qemu-user-static
```

### 常用命令
- `make build`：自动构建前端、后端和链接程序，生成 `compiler`、`front`、`back` 可执行文件。
- `make test`：对 `tests` 目录下所有测试用例（.tc 文件）进行编译，生成对应的 RISC-V 汇编文件（.s）到 `output` 目录。此命令**不依赖 riscv 工具链和 qemu**，适用于所有环境。
- `make test-full`：在已安装 riscv64-unknown-elf-gcc 和 qemu-riscv64 的环境下，自动对每个测试用例进行 RISC-V 汇编编译、模拟运行，并与本地 gcc 编译结果进行返回值比对，输出 PASS/FAIL。
- `make clean`：清理所有生成的可执行文件和 output 目录。

### 可执行文件说明
- `compiler`：主编译器链接模块，从标准输入读取，输出到标准输出。
- `front`：前端模块，输入文件名，输出到标准输出。
- `back`：后端模块，从标准输入读取，输出到标准输出。

### 注意事项
- `make test` 适用于所有环境，仅生成汇编，不依赖 riscv 工具链。
- `make test-full` 需提前安装 riscv64-unknown-elf-gcc 和 qemu-riscv64，仅在 Linux/WSL/MSYS2 下支持自动比对。
- Windows 用户推荐使用 MSYS2/MinGW 或 WSL 环境运行 make。

## 实验进度记录
### 第一周（6/23-6/29）
目标为实现一个基本的编译框架，能够从ToyC语言编译出正确的RISCV汇编语言。组内分工如下：
- 程英浩、徐松林：使用Ocaml语言实现ToyC语言到中间语言的转换，并进行正确性验证。编写实验报告。
- 武秋霖、黄健：使用C++语言实现中间语言到RISCV汇编语言的转换，并进行正确性验证。编写实验报告。
### 第二周（6/30-7/6）
目标为修正上周两部分搭建时各自的解析问题，以及链接项目时出现的不匹配问题，建立起前端扫描器与后端翻译器的链接，并初步实现编译初期优化。组内分工如下：
- 程英浩、徐松林：解决前端扫描时作用域的问题，实现AST的文件输出，并进行正确性验证。编写实验报告。
- 黄健：解决后端翻译时无法正确识别AST中特殊符号和关键字位置错误的问题，对前端和后端的链接编写操作脚本，收集代码优化相关资料。
- 武秋霖：初步实现汇编器的简单优化，并尝试排查汇编器中出现的正确性问题。编写实验报告。
### 第三周（7/7-7/13）
目标为根据老师发布的测试用例，排查相关问题，正确翻译所有测试用例。
本周主要为排查错误，综合并为下周的代码修正做准备。
### 第四周（7/14-7/20）
目标为根据上周的分析总结，得出我们当前编译器的问题所在，并进行代码修正。
- 程英浩、徐松林：重点修复在源码到AST过程中出现的注释处理错误问题。
- 黄健、武秋霖：解决测试点中出现的多参数传递导致的寄存器溢出错误和栈管理错误。
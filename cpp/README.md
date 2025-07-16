# BACK模块
## 作用
根据前端给予的抽象语法树文件（.ast），翻译得到正确且高效的RISCV汇编代码，并输出到标准输出流。
## 文件结构
- src：源文件目录。
- test：测试文件目录。
- Makefile：生成脚本文件。
- README.md：本说明文件。
## 使用方法
**注意：该makefile文件在Linux环境下编写，未测试是否能够在Windows下正常执行test，如果出现test错误，请执行`make help`查看Windows环境下的手动执行方法。**
本模块已经在顶层模块中实现直接与前端对接，因此可以直接在上一层级执行`make build`生成文件。同时本模块也提供了自己的生成脚本，可以执行`make help`查看可以使用的指令。
## 源文件简介
- main.cpp：该模块的集成器，串联汇编器的其他模块，形成一个整体。
- ASTNode.h：根据题目要求构建的AST结点头文件。
- ASTParser：将输入流转化为结构化AST流，以进行后续的分析。
- RegManager：寄存器分配模块，提供临时寄存器的分配和回收操作。
- Generator：汇编生成器，将结构化AST流解析为RISCV汇编语言。
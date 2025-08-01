// main module to connect front-end and back-end of the compiler
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>


int main() {
    try {
        // 直接将标准输入内容通过管道传递给 front，再传递给 back
        std::string command;
#ifdef _WIN32
        command = "type nul | front.exe | back.exe"; // Windows下可用方式（需调整）
#else
        command = "cat - | ./front | ./back";
#endif
        
        // 使用 popen 读取 back 的标准输出并由 compiler 输出
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to run pipeline command");
        }
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::cout << buffer;
        }
        int result = pclose(pipe);
        if (result != 0) {
            throw std::runtime_error("Compilation pipeline failed with exit code " + std::to_string(result));
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Compilation Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown compilation error occurred" << std::endl;
        return 1;
    }
}
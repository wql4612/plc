// main module to connect front-end and back-end of the compiler
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

int main(){
    try {
        std::string tcfilename = "code.tc";
        std::ofstream outputFile(tcfilename);
        
        if (!outputFile.is_open()) {
            throw std::runtime_error("Failed to open output file: " + tcfilename);
        }
        
        std::string line;
        while (std::getline(std::cin, line)) {
            outputFile << line << std::endl;
        }
        outputFile.close();

        std::string command;
#ifdef _WIN32
        // Windows环境 - 使用管道连接front和back
        command = "front.exe " + tcfilename + " | back.exe";
#else
        // Linux/Unix环境 - 使用管道连接front和back
        command = "./front " + tcfilename + " | ./back";
#endif
        
        int result = std::system(command.c_str());
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
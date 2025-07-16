#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "Generator.h"
#include "ASTNode.h"
#include "ASTParser.h"

int main() {
    try {
        // Parse AST from stdin
        ASTParser parser(std::cin);
        
        auto program = parser.parse();
        if (!program) {
            std::cerr << "Failed to parse AST from stdin" << std::endl;
            return 1;
        }
        // Generate assembly to stdout
        Generator generator(std::cout);
        generator.generateProg(*program);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
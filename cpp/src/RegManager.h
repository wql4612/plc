#pragma once
#include <vector>
#include <stack>
#include <string>
#include <stdexcept>

#define MAX_REG_COUNT 7 // t0 to t6

class RegManager {
private:
    int regCount = MAX_REG_COUNT; // total 7 registers: t0, t1, t2, t3, t4, t5, t6
    std::stack<int> freeRegs;
    std::stack<int> usedRegs;
    
public:
    RegManager();
    // allocate a first register
    std::string alloc();
    // allocate a specific register
    std::string alloc(const std::string& name);
    // release the last allocated register
    void release();
    // release a specific register
    void release(const std::string& reg);
    bool hasAvailable() const;
    void reset();
    // get list of currently used registers
    std::vector<std::string> getUsedRegisters() const;
};

#pragma once
#include <vector>
#include <stack>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <set>

// 扩展寄存器类型
enum class RegType
{
    TEMP, // 临时寄存器
    SAVE, // 被调用者保存寄存器
    ARG   // 参数寄存器
};

class RegManager
{ 
private:
    struct Register
    {
        int index;
        RegType type;
        bool used;
    };
    std::vector<Register> registers;
    std::unordered_map<RegType, std::set<int>> freeRegs;
    std::unordered_map<std::string, int> nameToIndex;
    std::unordered_map<std::string, int> spillTable;
    void initializeRegisters();

public:
    RegManager();
    void reset();

    std::string alloc(RegType type = RegType::TEMP);
    std::string alloc(const std::string &name);
    void release(const std::string &reg);
    void release(const std::vector<std::string> &regs);
    bool hasAvailable(RegType type = RegType::TEMP) const;
    std::vector<std::string> getUsedRegisters() const;
    RegType getRegType(const std::string &name) const;
    bool isRegInUse(const std::string &reg) const;

    // 溢出相关接口
    void spill(const std::string &reg, int offset); // 标记reg溢出到offset
    void restore(const std::string &reg);           // 恢复reg内容
    bool isSpilled(const std::string &reg) const;   // 查询是否溢出
    int getSpillOffset(const std::string &reg) const; // 获取溢出栈偏移
    void removeSpill(const std::string &reg);       // 清除溢出标记
};

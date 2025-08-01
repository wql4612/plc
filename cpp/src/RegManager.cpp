
#include "RegManager.h"
#include <unordered_map>
#include <iostream>
#include <string>

// 寄存器映射配置
const std::vector<std::pair<RegType, int>> REG_CONFIG = {
    {RegType::TEMP, 7},  // t0-t6
    {RegType::SAVE, 12}, // s0-s11
    {RegType::ARG, 8}    // a0-a7
};

void RegManager::initializeRegisters()
{
    int globalIndex = 0;
    for (const auto &[type, count] : REG_CONFIG)
    {
        for (int i = 0; i < count; i++)
        {
            std::string prefix;
            switch (type)
            {
            case RegType::TEMP:
                prefix = "t";
                break;
            case RegType::SAVE:
                prefix = "s";
                break;
            case RegType::ARG:
                prefix = "a";
                break;
            }

            std::string name = prefix + std::to_string(i);
            registers.push_back({globalIndex, type, false});
            nameToIndex[name] = globalIndex;
            freeRegs[type].insert(globalIndex);
            globalIndex++;
        }
    }
}

RegManager::RegManager()
{
    initializeRegisters();
}

bool RegManager::hasAvailable(RegType type) const
{
    return !freeRegs.at(type).empty();
}

std::string RegManager::alloc(RegType type)
{
    if (!hasAvailable(type))
    {
        throw std::runtime_error("No registers available for allocation");
    }
    int regIndex = *freeRegs[type].begin();
    freeRegs[type].erase(freeRegs[type].begin());
    registers[regIndex].used = true;
    for (const auto &[name, idx] : nameToIndex)
    {
        if (idx == regIndex) {
            return name;
        }
    }
    throw std::runtime_error("Register index not found");
}

std::string RegManager::alloc(const std::string &name)
{
    if (nameToIndex.find(name) == nameToIndex.end())
    {
        throw std::runtime_error("Invalid register name: " + name);
    }

    int index = nameToIndex[name];
    if (registers[index].used)
    {
        throw std::runtime_error("Register " + name + " is already in use");
    }

    RegType type = registers[index].type;
    freeRegs[type].erase(index);
    registers[index].used = true;
    return name;
}


void RegManager::release(const std::string &reg)
{
    if (nameToIndex.find(reg) == nameToIndex.end())
    {
        throw std::runtime_error("Invalid register name: " + reg);
    }

    int index = nameToIndex[reg];
    RegType type = registers[index].type;
    // if (!registers[index].used)
    // {
    //     throw std::runtime_error("Register " + reg + " is not in use");
    // }
    registers[index].used = false;
    freeRegs[type].insert(index);
}

void RegManager::release(const std::vector<std::string> &regs)
{
    for (const auto &reg : regs)
    {
        release(reg);
    }
}

void RegManager::reset()
{
    freeRegs.clear();
    for (auto &reg : registers)
    {
        reg.used = false;
        freeRegs[reg.type].insert(reg.index);
    }
}
std::vector<std::string> RegManager::getUsedRegisters() const
{
    std::vector<std::string> used;
    for (const auto &[name, index] : nameToIndex)
    {
        if (registers[index].used)
        {
            used.push_back(name);
        }
    }
    return used;
}

RegType RegManager::getRegType(const std::string &name) const
{
    if (nameToIndex.find(name) == nameToIndex.end())
    {
        throw std::runtime_error("Invalid register name: " + name);
    }
    return registers[nameToIndex.at(name)].type;
}

bool RegManager::isRegInUse(const std::string &reg) const
{
    if (nameToIndex.find(reg) == nameToIndex.end())
    {
        return false;
    }
    int index = nameToIndex.at(reg);
    return registers[index].used;
}


// 溢出相关接口实现
void RegManager::spill(const std::string &reg, int offset)
{
    spillTable[reg] = offset;
}

void RegManager::restore(const std::string &reg)
{
    spillTable.erase(reg);
}

bool RegManager::isSpilled(const std::string &reg) const
{
    return spillTable.find(reg) != spillTable.end();
}

int RegManager::getSpillOffset(const std::string &reg) const
{
    auto it = spillTable.find(reg);
    if (it == spillTable.end())
        throw std::runtime_error("Register " + reg + " is not spilled");
    return it->second;
}

void RegManager::removeSpill(const std::string &reg)
{
    spillTable.erase(reg);
}
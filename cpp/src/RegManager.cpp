#include "RegManager.h"

RegManager::RegManager() {
    for(int i=regCount-1;i>=0;i--){ //reverse order
        freeRegs.push(i);
    }
}
bool RegManager::hasAvailable() const {
    return !freeRegs.empty();
}
std::string RegManager::alloc(){
    if(!hasAvailable()) {
        throw std::runtime_error("No registers available for allocation");
    }
    int index = freeRegs.top();
    freeRegs.pop();
    usedRegs.push(index);
    return "t" + std::to_string(index);
}
std::string RegManager::alloc(const std::string& name) {
    // Validate register name format
    if (name.length() < 2 || name[0] != 't') {
        throw std::runtime_error("Invalid register name format: " + name);
    }
    
    int index;
    try {
        index = std::stoi(name.substr(1));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid register name: " + name);
    }
    
    if(index < 0 || index >= regCount) {
        throw std::runtime_error("Invalid register name: " + name);
    }
    
    bool allocated = false;
    std::stack<int> tempStack;
    while(!freeRegs.empty()){
        if(freeRegs.top() != index) {
            tempStack.push(freeRegs.top());
            freeRegs.pop();
        }else{
            allocated = true;
            usedRegs.push(index);
            freeRegs.pop();
            break;
        }
    }
    while (!tempStack.empty()) {
        freeRegs.push(tempStack.top());
        tempStack.pop();
    }
    if(!allocated) {
        throw std::runtime_error("Register " + name + " is not available");
    }
    return name;
}
void RegManager::release() {
    if(usedRegs.empty()) {
        throw std::runtime_error("No registers to release");
    }
    int index = usedRegs.top();
    usedRegs.pop();
    freeRegs.push(index);
}
void RegManager::release(const std::string& reg){
    // Validate register name format
    if (reg.length() < 2 || reg[0] != 't') {
        throw std::runtime_error("Invalid register name format: " + reg);
    }
    
    int index;
    try {
        index = std::stoi(reg.substr(1));
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid register name: " + reg);
    }
    
    if(index < 0 || index >= regCount) {
        throw std::runtime_error("Invalid register name: " + reg);
    }
    bool released = false;
    std::stack<int> tempStack;
    while(!usedRegs.empty()){
        if(usedRegs.top() != index) {
            tempStack.push(usedRegs.top());
            usedRegs.pop();
        }else{
            released = true;
            usedRegs.pop();
            freeRegs.push(index);
            break;
        }
    }
    while (!tempStack.empty()) {
        usedRegs.push(tempStack.top());
        tempStack.pop();
    }
    if(!released) {
        throw std::runtime_error("Register " + reg + " is not in use");
    }
}
void RegManager::reset(){
    while(!freeRegs.empty()) freeRegs.pop();
    while(!usedRegs.empty()) usedRegs.pop();
    for(int i=regCount-1;i>=0;i--) freeRegs.push(i);
}
std::vector<std::string> RegManager::getUsedRegisters() const {
    std::vector<std::string> result;
    std::stack<int> temp = usedRegs;
    
    while (!temp.empty()) {
        result.push_back("t" + std::to_string(temp.top()));
        temp.pop();
    }
    
    return result;
}
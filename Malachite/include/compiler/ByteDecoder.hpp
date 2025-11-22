#pragma once
#include "..\core\vm.h"
#include "CompilationState.hpp"
#include <bitset>
#include <stack>
#include "Logger.hpp"

namespace Malachite 
{
	constexpr size_t InvalidRegister = SIZE_MAX;
    constexpr int64_t StartDepth = -1;
	struct RegistersTable 
	{
    private:
		std::bitset<MalachiteCore::REGISTER_COUNT> registers{};	//0 is free, 1 is busy

    public:
        RegistersTable() = default;

        size_t Allocate() {
            for (size_t i = 0; i < MalachiteCore::REGISTER_COUNT; i++) {
                if (!registers.test(i)) {
                    registers.set(i);
                    return i;
                }
            }
            return InvalidRegister;
        }

        bool Acquire(size_t reg) {
            if (reg >= MalachiteCore::REGISTER_COUNT || registers.test(reg))
                return false;
            registers.set(reg);
            return true;
        }

        bool Release(size_t reg) {
            if (reg >= MalachiteCore::REGISTER_COUNT || !registers.test(reg))
                return false;
            registers.reset(reg);
            return true;
        }

        size_t CountFree() const {
            return MalachiteCore::REGISTER_COUNT - registers.count();
        }

        bool IsFree(size_t reg) const {
            return reg < MalachiteCore::REGISTER_COUNT && !registers.test(reg);
        }

        void Clear() {
            registers.reset();
        }

        void DebugPrint() const {
            std::cout << "Registers: ";
            for (size_t i = 0; i < MalachiteCore::REGISTER_COUNT; i++) {
                std::cout << (registers.test(i) ? 'B' : 'F');   //B - busy, F - free
            }
            std::cout << "\n";
        }
	};

    struct ValueFrame 
    {
        enum ValueType {IMMEDIATE,VARIABLE,OPERATION_RESULT};

        uint64_t used_register = InvalidRegister;
        TokenValue immediate_value = TokenValue((uint64_t)0);
        variableID variable_id = InvalidRegister;
        ValueType value_type = ValueType::IMMEDIATE;

        ValueFrame(variableID var, uint64_t ur) 
        {
            variable_id = var;
            value_type = ValueType::VARIABLE;
            used_register = ur;
        }
        ValueFrame(TokenValue vaL, uint64_t ur)
        {
            immediate_value = vaL;
            value_type = ValueType::IMMEDIATE;
            used_register = ur;
        }
        ValueFrame(uint64_t ur)
        {
            value_type = ValueType::OPERATION_RESULT;
            used_register = ur;
        }
    };

    struct VariableInfo 
    {
        int depth = StartDepth;
        uint64_t stack_offset = 0;  
    }; 

	class ByteDecoder		//Pseudo->Byte code
	{
    private:
        //global_state
        RegistersTable regsTable;
        std::shared_ptr<CompilationState> current_state;
        int64_t current_depth = StartDepth; //Program starts by ScopeStart and ends by ScopeEnd, but we need start depth = 0
        uint64_t ip = 0;
        std::stack<ValueFrame> value_stack;         //Stack for operations
        std::stack<uint64_t> frame_size_stack;    //When we create variable add it size to frame_size stack;

        std::unordered_map<variableID, VariableInfo> variable_depth; //variableID and info about variable. If we meet DECLARE_VARIABLE -> add writting <ID, Info>


        //Methods---------------------
        std::vector<MalachiteCore::VMCommand> HandleMemoryCommand(const std::vector<PseudoCommand>& cmds, size_t ip);
        std::vector<MalachiteCore::VMCommand> HandleDeclaringCommand(const std::vector<PseudoCommand>& cmds, size_t ip);

        std::vector<MalachiteCore::VMCommand> HandleCommand(const std::vector<PseudoCommand>& cmds, size_t ip);

        void ClearState();

    public:
        std::vector<MalachiteCore::VMCommand> PseudoToByte(std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> state);
	};
}
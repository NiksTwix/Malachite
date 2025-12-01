#pragma once
#include "..\core\vm.h"
#include "CompilationState.hpp"
#include <bitset>
#include <stack>
#include "Logger.hpp"

namespace Malachite 
{
	constexpr size_t InvalidRegister = SIZE_MAX;
    constexpr int64_t StartDepth = 0;

    struct ValueFrame
    {
        enum ValueSourceType { IMMEDIATE, VARIABLE, OPERATION_RESULT };
        enum ValueType { UINT, INT, DOUBLE };    //FLOAT(malachite syntax) = DOUBLE(c++)
        uint64_t used_register = InvalidRegister;
        TokenValue immediate_value = TokenValue((uint64_t)0);
        variableID variable_id = InvalidRegister;
        ValueSourceType value_source_type = ValueSourceType::IMMEDIATE;
        Type::VMAnalog value_type = Type::VMAnalog::UINT;

        ValueFrame()
        {
        }

        ValueFrame(variableID var, uint64_t ur, Type::VMAnalog type = Type::VMAnalog::UINT)
        {
            variable_id = var;
            value_source_type = ValueSourceType::VARIABLE;
            used_register = ur;
            value_type = type;
        }
        ValueFrame(TokenValue vaL, uint64_t ur, Type::VMAnalog type = Type::VMAnalog::UINT)
        {
            immediate_value = vaL;
            value_source_type = ValueSourceType::IMMEDIATE;
            used_register = ur;
            value_type = type;
        }
        ValueFrame(uint64_t ur, Type::VMAnalog type = Type::VMAnalog::UINT)
        {
            value_source_type = ValueSourceType::OPERATION_RESULT;
            used_register = ur;
            value_type = type;
        }
    };
	struct RegistersTable 
	{
    private:
		std::bitset<MalachiteCore::REGISTER_COUNT> registers{};	//0 is free, 1 is busy

    public:

        std::unordered_map<std::string, std::pair<uint64_t, ValueFrame>> pseudonymized_registers;

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
            std::unordered_set<uint64_t> allocated_handle = {};
            for (auto& p : pseudonymized_registers) 
            {
                allocated_handle.insert(p.second.first);
            }
            for (size_t i = 0; i < MalachiteCore::REGISTER_COUNT; i++) {
                if (registers.test(i) && !allocated_handle.count(i)) {
                    registers.reset(i);
                }
            }
        }
        void FullClear() {
            registers.reset();
            pseudonymized_registers.clear();
        }
        void DebugPrint() const {
            std::cout << "Registers: ";
            for (size_t i = 0; i < MalachiteCore::REGISTER_COUNT; i++) {
                std::cout << (registers.test(i) ? 'B' : 'F');   //B - busy, F - free
            }
            std::cout << "\n";
        }
	};

    

    struct VariableInfo 
    {
        int depth = StartDepth;
        uint64_t stack_offset = 0;  
    }; 

    struct ByteDecodingState 
    {
        uint64_t ip = 0;
        RegistersTable registers_table{};
        std::shared_ptr<CompilationState> current_state{};
        int64_t current_depth = StartDepth; //Program starts by OpenVisibleScope and ends by CloseVisibleScope, but we need start depth = 0
        std::stack<ValueFrame> value_stack{};         //Stack for operations
        std::stack<uint64_t> frame_size_stack{};    //When we create variable add it size to frame_size stack;

        std::unordered_map<variableID, VariableInfo> variable_depth{}; //variableID and info about variable. If we meet DECLARE_VARIABLE -> add writting <ID, Info>

        std::unordered_map<uint64_t, uint64_t>  labels{}; //ID, IP
        std::unordered_map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> waiting_jumps{};  //Label ID, IP (Pseudo,Byte) of jmp commands

        std::vector<MalachiteCore::VMCommand>* current_commands = nullptr;

    };


	class ByteDecoder		//Pseudo->Byte code
	{
    private:

        ByteDecodingState current_BDS;
        //Methods---------------------
        std::vector<MalachiteCore::VMCommand> HandleMemoryCommand(const std::vector<PseudoCommand>& cmds, size_t& ip);
        std::vector<MalachiteCore::VMCommand> HandleDeclaringCommand(const std::vector<PseudoCommand>& cmds, size_t& ip);
        std::vector<MalachiteCore::VMCommand> HandleArithmeticCommand(const std::vector<PseudoCommand>& cmds, size_t& ip);
        std::vector<MalachiteCore::VMCommand> HandleLogicCommand(const std::vector<PseudoCommand>& cmds, size_t& ip);

        std::vector<MalachiteCore::VMCommand> HandleOpCodeSectionCommands(const std::vector<PseudoCommand>& cmds, size_t& ip);

        std::vector<MalachiteCore::VMCommand> HandleSpecialCommands(const std::vector<PseudoCommand>& cmds, size_t& ip);

        std::vector<MalachiteCore::VMCommand> HandleControlFlowCommand(const std::vector<PseudoCommand>& cmds, size_t& ip);

        std::vector<MalachiteCore::VMCommand> HandleCommand(const std::vector<PseudoCommand>& cmds, size_t& ip);




        MalachiteCore::VMCommand ByteDecoder::GetVMTypeConvertionCommand(Type::VMAnalog first, uint64_t first_register,Type::VMAnalog second, uint64_t second_register,uint64_t& converted_register, Type::VMAnalog& result_type);
        MalachiteCore::VMCommand GetConversionCommand(Type::VMAnalog from, Type::VMAnalog to, uint64_t reg);

        MalachiteCore::OpCode GetVMTypeConvertionCommand(Type::VMAnalog first, Type::VMAnalog second);  //First to second
        MalachiteCore::OpCode GetVMTypedArithmeticCommand(PseudoOpCode code, Type::VMAnalog type); 

        MalachiteCore::OpCode GetVMLogicCommand(PseudoOpCode code, Type::VMAnalog type);

        void ClearState();

    public:
        std::vector<MalachiteCore::VMCommand> PseudoToByte(std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> state);
	};
}
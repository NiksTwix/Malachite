#include "../../include/core/functions.h"

namespace MalachiteCore 
{
	VMError execute(VMState* state, VMCommand* commands, size_t commands_size)
	{
		if (state == nullptr)return VMError::VMS_PTR_INVALID;
		if (commands == nullptr) return VMError::VMCS_INVALID;
		if (commands_size == 0) return VMError::VMCS_INVALID;

		if (!(state->flags & STOPPED_FLAG))state->ip = 0;
		else state->flags &= ~STOPPED_FLAG;
		size_t command_size = sizeof(VMCommand);

		VMError result = VMError::NO_ERROR;

		for (; state->ip < commands_size;) 
		{
			state->flags &= ~JUMPED_FLAG;
			VMCommand* command = commands + state->ip;

			if (command->operation == OpCode::OP_NOP)
			{
				state->ip++;
				continue;
			}

			if (OperationListBlock::IsOperationInInterval(command->operation, OperationListBlock::ARITHMETIC_START, OperationListBlock::ARITHMETIC_END))
			{
				result = arithmetic_handler(state, command);
			}
			else if (OperationListBlock::IsOperationInInterval(command->operation, OperationListBlock::LOGIC_START, OperationListBlock::LOGIC_END))
			{
				result = logic_handler(state, command);
			}
			else if (OperationListBlock::IsOperationInInterval(command->operation, OperationListBlock::MEMORY_START, OperationListBlock::MEMORY_END))
			{
				result = memory_handler(state, command);
			}
			else if (OperationListBlock::IsOperationInInterval(command->operation, OperationListBlock::CONTROL_FLOW_START, OperationListBlock::CONTROL_FLOW_END))
			{
				result = control_flow_handler(state, command);
			}
			else if (OperationListBlock::IsOperationInInterval(command->operation, OperationListBlock::SYSTEM_CALLS_START, OperationListBlock::SYSTEM_CALLS_END))
			{
				result = syscalls_handler(state, command);
			}
			else 
			{
				result = special_handler(state, command);
			}
			if (result == VMError::EXIT)
			{
				state->flags |= STOPPED_FLAG;
				return result;
			}
			if (result != VMError::NO_ERROR) 
			{
				state->error_stack.push(ErrorFrame(result, state->ip));
				return result;
			}
			if (state->flags & JUMPED_FLAG) continue;
			state->ip++;
		}

		return VMError::NO_ERROR;
	}
	VMError syscalls_handler(VMState* state, VMCommand* command)
	{
		SysCall call = static_cast<SysCall>(command->destination);

		switch (call)
		{
		case MalachiteCore::PRINT_INT:
		{
			uint64_t value = state->registers[command->source0].i;	//Take value from register
			std::cout << value;
			break;
		}
		case MalachiteCore::PRINT_DOUBLE:
		{
			double value = state->registers[command->source0].d;	//Take value from register
			std::cout << value;
			break;
		}
		//Work with char array// char in MEMORY// PRINT_CHAR_ARRAY
		case MalachiteCore::PRINT_CHAR_ARRAY:
		{
			int64_t pointer = state->registers[command->source0].i;	//Take value from register
			int64_t size = state->registers[command->source1].i;	//Take value from register

			if (pointer + size >= MAX_MEMORY_SIZE) return VMError::MEMORY_ACCESS_VIOLATION;

			char array[size];

			for (size_t i = pointer; i < pointer + size; i++) {
				putchar(state->memory[pointer + i]);
			}
			putchar('\n');  // новая строка после вывода
			}
			break;
		}
		return VMError::NO_ERROR;
	}
	VMError special_handler(VMState* state, VMCommand* command)
	{
		switch (command->operation) 
		{
		case OpCode::OP_TC_ITD_R:
			state->registers[command->destination].d = static_cast<double>(state->registers[command->destination].i);
			break;
		case OpCode::OP_TC_DTI_R:
			state->registers[command->destination].i = static_cast<int64_t>(state->registers[command->destination].d);
			break;
		case OpCode::OP_TC_UITD_R:
			state->registers[command->destination].d = static_cast<double>(state->registers[command->destination].u);
			break;
		case OpCode::OP_TC_UITI_R:
			state->registers[command->destination].i = static_cast<int64_t>(state->registers[command->destination].u);
			break;
		case OpCode::OP_TC_DTUI_R:
			state->registers[command->destination].u = static_cast<uint64_t>(state->registers[command->destination].d);
			break;
		case OpCode::OP_TC_ITUI_R:
			state->registers[command->destination].u = static_cast<uint64_t>(state->registers[command->destination].i);
			break;
		}

		return VMError::NO_ERROR;
	}
}
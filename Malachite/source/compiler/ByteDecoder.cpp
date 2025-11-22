#include "..\..\include\compiler\ByteDecoder.hpp"
#include <stack>
namespace Malachite 
{
	void Malachite::ByteDecoder::ClearState()
	{
		current_depth = StartDepth;
		regsTable.Clear();
		current_state = nullptr;
		variable_depth.clear();
		ip = 0;
		while (!value_stack.empty())
		{
			value_stack.pop();
		}
	}
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleMemoryCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand cmd = cmds[ip];
		switch (cmd.op_code)
		{
		case PseudoOpCode::Immediate:
		{
			TokenValue val = cmd.parameters[PseudoFieldNames::Get().valueID_name];
			auto free_register = regsTable.Allocate();
			if (free_register == InvalidRegister)
			{
				Logger::Get().PrintLogicError("All registers are in using. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
			}
			switch (val.type)
			{
			case Malachite::TokenValueType::VOID:
				Logger::Get().PrintTypeError("Attemp of immediating void value. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
				return result;
			case Malachite::TokenValueType::INT:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_INT, free_register, MalachiteCore::Register(val.intVal)));
				value_stack.push(ValueFrame(val, free_register));
				return result;
			case Malachite::TokenValueType::UINT:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_UINT, free_register, MalachiteCore::Register(val.uintVal)));
				value_stack.push(ValueFrame(val, free_register));
				return result;
			
			case Malachite::TokenValueType::FLOAT:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_DOUBLE, free_register, MalachiteCore::Register(val.floatVal)));
				value_stack.push(ValueFrame(val, free_register));
				return result;
			case Malachite::TokenValueType::STRING:
				Logger::Get().PrintTypeError("Attemp of direct immediating string value. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
				return result;
			case Malachite::TokenValueType::CHAR:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_INT, free_register, MalachiteCore::Register((int64_t)val.charVal)));
				value_stack.push(ValueFrame(val, free_register));
				return result;
			case Malachite::TokenValueType::BOOL:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_UINT, free_register, MalachiteCore::Register((uint64_t)val.boolVal)));
				value_stack.push(ValueFrame(val, free_register));
				return result;
			}
			break;
		}
		case PseudoOpCode::Push:
			break; 
		case PseudoOpCode::Pop:
			break;
		case PseudoOpCode::Load:
		{
			Variable& var = current_state->variables_global_table.at(cmd.parameters[PseudoFieldNames::Get().variableID_name].uintVal);
			Type& type = current_state->types_global_table.at(var.type_id);
			auto info = variable_depth.at(var.variable_id);
			auto free_register = regsTable.Allocate();
			if (free_register == InvalidRegister) 
			{
				Logger::Get().PrintLogicError("All registers are in using. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
			}
			size_t size = type.size;
			if (type.category == Type::Category::PRIMITIVE) 
			{
				//PseudoDecoder checked vars validity
				if (info.depth == current_depth)
				{
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_LOAD_LOCAL, free_register, info.stack_offset, type.size));
					value_stack.push(ValueFrame(var.variable_id, free_register));
				}
				else //
				{
					uint64_t size_and_depth = size << 32;	//0...size -> size...0
					size_and_depth |= info.depth;	//uint64_t and int64_t size...0 -> size...depth
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_LOAD_ENCLOSING_A, free_register, info.stack_offset, size_and_depth));
					value_stack.push(ValueFrame(var.variable_id, free_register));
				}
				return result;
			}
			else if (type.category == Type::Category::ALIAS)
			{
				//Thinking
			}
			else if (type.category == Type::Category::CLASS)
			{
				//Thinking
			}
			break;
		}	    
		case PseudoOpCode::Store:
		{
			Variable& var = current_state->variables_global_table.at(cmd.parameters[PseudoFieldNames::Get().variableID_name].uintVal);
			Type& type = current_state->types_global_table.at(var.type_id);
			auto info = variable_depth.at(var.variable_id);

			// We need some history about register using. As example, we have this expression: x = 2 * 3
			// 2 and 3 in values stack, but where is multiplier's result? 
			// I think we should to save to values stack value frame with fields register_to_save (or another name) and type = OPERATION_RESULT
			// It also must be when we call function or something another

			ValueFrame& vf = value_stack.top();
			value_stack.pop();
			if (vf.used_register == InvalidRegister)
			{
				Logger::Get().PrintLogicError("All registers are in using. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
			}
			size_t size = type.size;
			if (type.category == Type::Category::PRIMITIVE)
			{
				//PseudoDecoder checked vars validity
				if (info.depth == current_depth)
				{
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_STORE_LOCAL, info.stack_offset, vf.used_register, type.size));
				}
				else //
				{
					uint64_t size_and_depth = size << 32;	//0...size -> size...0
					size_and_depth |= info.depth;	//uint64_t and int64_t size...0 -> size...depth
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_STORE_ENCLOSING_A, info.stack_offset, vf.used_register, size_and_depth));
				}
				regsTable.Acquire(vf.used_register);
				return result;
			}
			else if (type.category == Type::Category::ALIAS)
			{
				//Thinking
			}
			else if (type.category == Type::Category::CLASS)
			{
				//Thinking
			}

			break;
		}    
		case PseudoOpCode::LoadRelatively:		//Functions -> OP_LOAD_ENCLOSING_R
			break;
		case PseudoOpCode::StoreRelatively:		//Functions -> OP_STORE_ENCLOSING_R
			break;
		case PseudoOpCode::LoadOffset:
			break;           
		case PseudoOpCode::StoreOffset:
			break;       
		case PseudoOpCode::LoadDirect:
			break;       
		case PseudoOpCode::StoreDirect:
			break;      
		case PseudoOpCode::GetAddress:
			break;
		default:
			break;
		}

		return result;
	}

	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleDeclaringCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand cmd = cmds[ip];
		switch (cmd.op_code)
		{
		case PseudoOpCode::DeclareVariable:
		{
			Variable& var = current_state->variables_global_table.at(cmd.parameters[PseudoFieldNames::Get().variableID_name].uintVal);
			Type& type = current_state->types_global_table.at(var.type_id);
			if (type.size == 0)
			{
				Logger::Get().PrintLogicError("Invalid type size == 0. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
				break;
			}
			VariableInfo vi;
			vi.depth = current_depth;
			vi.stack_offset = frame_size_stack.top();
			frame_size_stack.top() += type.size;
			//We need to push stack pointer
			result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_PUSH,type.size,0));	//Take trash from zero register for pulling variable's space 
			break;
		}
		case PseudoOpCode::DeclareFunction:
		{
			//Cycle with calling HandleCommand before we meet a label with value FUNCTION_END
			break;
		}
		default:
		{
			break;
		}
		return result;
		}
	}
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand pd = cmds[ip];
		//SCOPE START/END
		if (pd.op_code == PseudoOpCode::ScopeStart)
		{
			result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_CREATE_FRAME));	//Creates frame in the vm data stack
			frame_size_stack.push(0);
			current_depth++;
		}
		if (pd.op_code == PseudoOpCode::ScopeEnd)
		{
			result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_DESTROY_FRAME));	//Destroes frame in the vm data stack
			frame_size_stack.pop();
			regsTable.Clear();	//Clears register after scope's exit
			current_depth--;
		}
		//Loading/Storing

		if (pd.op_code > PseudoOpCode::START_SECTION_MEMORY_OPS && pd.op_code < PseudoOpCode::END_SECTION_MEMORY_OPS)
		{
			auto result1 = HandleMemoryCommand(cmds, ip);
			result.insert(result.end(), result1.begin(), result1.end());
		}
		//Declaring
		if (pd.op_code > PseudoOpCode::START_SECTION_DECLARING_OPS && pd.op_code < PseudoOpCode::END_SECTION_DECLARING_OPS)
		{
			auto result1 = HandleDeclaringCommand(cmds, ip);
			result.insert(result.end(), result1.begin(), result1.end());
		}

		return result;
	}

	std::vector<MalachiteCore::VMCommand> Malachite::ByteDecoder::PseudoToByte(std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> state)
	{
		ClearState();
		current_state = state.first;

		std::vector<PseudoCommand>& code = state.second;

		std::vector<MalachiteCore::VMCommand> result;

		for (; ip < code.size(); ip++) 
		{
			HandleCommand(code, ip);
		}
		return result;
	}
}



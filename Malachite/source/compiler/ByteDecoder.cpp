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
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleMemoryCommand(PseudoCommand cmd)
	{
		std::vector<MalachiteCore::VMCommand> result;
		switch (cmd.op_code)
		{
		case PseudoOpCode::Immediate:
			break;
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
			break;     
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

		return std::vector<MalachiteCore::VMCommand>();
	}

	std::vector<MalachiteCore::VMCommand> Malachite::ByteDecoder::PseudoToByte(std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> state)
	{
		ClearState();
		current_state = state.first;

		std::vector<PseudoCommand>& code = state.second;

		std::vector<MalachiteCore::VMCommand> result;

		for (; ip < code.size(); ip++) 
		{
			PseudoCommand& pd = code[ip];
			//SCOPE START/END
			if (pd.op_code == PseudoOpCode::ScopeStart) 
			{
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_CREATE_FRAME));	//Creates frame in the vm data stack
				current_depth++;
			}
			if (pd.op_code == PseudoOpCode::ScopeEnd) 
			{
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_DESTROY_FRAME));	//Destroes frame in the vm data stack
				regsTable.Clear();	//Clears register after scope's exit
				current_depth--;
			}
			//Loading/Storing

			if (pd.op_code > PseudoOpCode::START_SECTION_MEMORY_OPS && pd.op_code < PseudoOpCode::END_SECTION_MEMORY_OPS) 
			{
				auto result1 = HandleMemoryCommand(pd);
				result.insert(result.end(), result1.begin(), result1.end());
			}


		}



		return std::vector<MalachiteCore::VMCommand>();
	}
}


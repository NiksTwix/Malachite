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
				variable_depth[var.variable_id] = vi;
				if (type.category == Type::Category::PRIMITIVE) frame_size_stack.top() += type.size;
				if (type.category == Type::Category::CLASS) frame_size_stack.top() += sizeof(MalachiteCore::Pointer);
				//if (type.category == Type::Category::ALIAS) frame_size_stack.top();
				//{
				//	//letter. Alias its common pseudoname of some type, if parent type is primitive -> push type.size, if class -> push pointer size
				//}
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
		}
		return result;
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
		//Arithmetic
		if (pd.op_code > PseudoOpCode::START_SECTION_ARITHMETIC_OPS && pd.op_code < PseudoOpCode::END_SECTION_ARITHMETIC_OPS)
		{
			auto result1 = HandleArithmeticCommand(cmds, ip);
			result.insert(result.end(), result1.begin(), result1.end());
		}
		return result;
	}

	MalachiteCore::VMCommand ByteDecoder::GetVMTypeConvertionCommand(
		Type::VMAnalog first, uint64_t first_register,
		Type::VMAnalog second, uint64_t second_register,
		uint64_t& converted_register, Type::VMAnalog& result_type)
	{
		// Определяем целевой тип (более точный)
		Type::VMAnalog target_type = (first > second) ? first : second;

		// Выбираем какой регистр конвертировать
		if (first != target_type) {
			converted_register = first_register;
			result_type = target_type;
			return GetConversionCommand(first, target_type, first_register);
		}
		else {
			converted_register = second_register;
			result_type = target_type;
			return GetConversionCommand(second, target_type, second_register);
		}
	}

	MalachiteCore::VMCommand ByteDecoder::GetConversionCommand(Type::VMAnalog from, Type::VMAnalog to, uint64_t reg) {
		if (from == to) return MalachiteCore::VMCommand(MalachiteCore::OP_NOP);

		if (to == Type::VMAnalog::DOUBLE) {
			if (from == Type::VMAnalog::UINT) return MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_TC_UITD_R, reg);
			if (from == Type::VMAnalog::INT) return MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_TC_ITD_R, reg);
		}
		if (to == Type::VMAnalog::INT) {
			if (from == Type::VMAnalog::UINT) return MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_TC_UITI_R, reg);
		}
		if (to == Type::VMAnalog::UINT) {
			if (from == Type::VMAnalog::INT) return MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_TC_ITUI_R, reg);
		}
		return MalachiteCore::VMCommand(MalachiteCore::OP_NOP);
	}

	MalachiteCore::OpCode ByteDecoder::GetVMTypeConvertionCommand(Type::VMAnalog first, Type::VMAnalog second)	//First to second
	{
		if (first == second) return MalachiteCore::OP_NOP;	//If types are equal
		if (first == Type::VMAnalog::INT && second == Type::VMAnalog::UINT) return MalachiteCore::OpCode::OP_TC_ITUI_R;
		if (first == Type::VMAnalog::INT && second == Type::VMAnalog::DOUBLE) return MalachiteCore::OpCode::OP_TC_ITD_R;
		if (first == Type::VMAnalog::DOUBLE && second == Type::VMAnalog::INT) return MalachiteCore::OpCode::OP_TC_DTI_R;
		if (first == Type::VMAnalog::DOUBLE && second == Type::VMAnalog::UINT) return MalachiteCore::OpCode::OP_TC_DTUI_R;
		if (first == Type::VMAnalog::UINT && second == Type::VMAnalog::INT) return MalachiteCore::OpCode::OP_TC_UITI_R;
		if (first == Type::VMAnalog::UINT && second == Type::VMAnalog::DOUBLE) return MalachiteCore::OpCode::OP_TC_UITD_R;
		return MalachiteCore::OP_NOP;
	}

	MalachiteCore::OpCode ByteDecoder::GetVMTypedArithmeticCommand(PseudoOpCode code, Type::VMAnalog type)
	{
		if (type == Type::VMAnalog::INT) 
		{
			switch (code)
			{
			case Malachite::PseudoOpCode::Add:
				return MalachiteCore::OP_IADD_RRR;
			case Malachite::PseudoOpCode::Subtract:
				return MalachiteCore::OP_ISUB_RRR;
			case Malachite::PseudoOpCode::Multiplication:
				return MalachiteCore::OP_IMUL_RRR;
			case Malachite::PseudoOpCode::Division:
				return MalachiteCore::OP_IDIV_RRR;
			case Malachite::PseudoOpCode::Mod:
				return MalachiteCore::OP_IMOD_RRR;
			case Malachite::PseudoOpCode::Negative:
				return MalachiteCore::OP_INEG_RR;
			}
		}
		if (type == Type::VMAnalog::UINT)
		{
			switch (code)
			{
			case Malachite::PseudoOpCode::Add:
				return MalachiteCore::OP_UADD_RRR;
			case Malachite::PseudoOpCode::Subtract:
				return MalachiteCore::OP_USUB_RRR;
			case Malachite::PseudoOpCode::Multiplication:
				return MalachiteCore::OP_UMUL_RRR;
			case Malachite::PseudoOpCode::Division:
				return MalachiteCore::OP_UDIV_RRR;
			case Malachite::PseudoOpCode::Mod:
				return MalachiteCore::OP_UMOD_RRR;
			}
		}
		if (type == Type::VMAnalog::DOUBLE)
		{
			switch (code)
			{
			case Malachite::PseudoOpCode::Add:
				return MalachiteCore::OP_DADD_RRR;
			case Malachite::PseudoOpCode::Subtract:
				return MalachiteCore::OP_DSUB_RRR;
			case Malachite::PseudoOpCode::Multiplication:
				return MalachiteCore::OP_DMUL_RRR;
			case Malachite::PseudoOpCode::Division:
				return MalachiteCore::OP_DDIV_RRR;
			case Malachite::PseudoOpCode::Negative:
				return MalachiteCore::OP_DNEG_RR;
			}
		}
		return MalachiteCore::OpCode::OP_NOP;
	}

	std::vector<MalachiteCore::VMCommand> Malachite::ByteDecoder::PseudoToByte(std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> state)
	{
		ClearState();
		current_state = state.first;

		std::vector<PseudoCommand>& code = state.second;

		std::vector<MalachiteCore::VMCommand> result;
		try 
		{
			for (; ip < code.size(); ip++)
			{
				auto commands = HandleCommand(code, ip);
				result.insert(result.end(), commands.begin(), commands.end());
			}
		}
		catch (std::runtime_error& e) {
			Logger::Get().PrintLogicError(e.what(), ip);
		}
		catch (std::logic_error& e) {
			Logger::Get().PrintTypeError(e.what(), ip);
		}
		catch (std::exception& e) {
			Logger::Get().PrintLogicError("Unexpected error: " + std::string(e.what()), ip);
		}
		return result;
	}
}



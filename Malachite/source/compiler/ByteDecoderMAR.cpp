#include "..\..\include\compiler\ByteDecoder.hpp"
#include <stack>


//Memory and arithmetic code generation
namespace Malachite 
{
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleArithmeticCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		//e convert the type of the right value to the left one
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand cmd = cmds[ip];

		auto main_ari_handler = [&]() -> void
			{
				//Use secondly register of the one operand
				if (current_BDS.value_stack.size() < 2)
				{
					Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " is binary operation, but gets only one.", ip);
					return;
				}
				ValueFrame right = current_BDS.value_stack.top(); current_BDS.value_stack.pop();
				ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

				uint64_t converted_reg;
				Type::VMAnalog common_type;
				auto conv_cmd = GetVMTypeConvertionCommand(
					left.value_type, left.used_register,
					right.value_type, right.used_register,
					converted_reg, common_type
				);
				if (conv_cmd.operation != MalachiteCore::OpCode::OP_NOP) {
					result.push_back(conv_cmd);
				}

				result.push_back(MalachiteCore::VMCommand(
					GetVMTypedArithmeticCommand(cmd.op_code, common_type),
					left.used_register,
					left.used_register,
					right.used_register
				));

				current_BDS.regsTable.Release(right.used_register);
				current_BDS.value_stack.push(ValueFrame(left.used_register, common_type));
			};

		switch (cmd.op_code)
		{
		case PseudoOpCode::Add:
		{
			main_ari_handler();
			break;
		}
		case PseudoOpCode::Subtract:
		{
			main_ari_handler();
			break;
		}
		case PseudoOpCode::Multiplication:
		{
			main_ari_handler();
			break;
		}
		case PseudoOpCode::Division:
		{

			main_ari_handler();
			break;
		}
		case PseudoOpCode::Mod:
		{
			if (current_BDS.value_stack.size() < 2)
			{
				Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " is binary operation, but gets only one.", ip);
				break;
			}
			ValueFrame right = current_BDS.value_stack.top(); current_BDS.value_stack.pop();
			ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

			// Mod только для целых типов
			if ((left.value_type != Type::VMAnalog::INT && left.value_type != Type::VMAnalog::UINT) || (right.value_type != Type::VMAnalog::INT && right.value_type != Type::VMAnalog::UINT)) {
				Logger::Get().PrintTypeError("Mod operation requires integer types.", ip);
				break;
			}
			uint64_t converted_reg;
			Type::VMAnalog common_type;
			auto conv_cmd = GetVMTypeConvertionCommand(
				left.value_type, left.used_register,
				right.value_type, right.used_register,
				converted_reg, common_type
			);
			if (conv_cmd.operation != MalachiteCore::OpCode::OP_NOP) {
				result.push_back(conv_cmd);
			}

			result.push_back(MalachiteCore::VMCommand(
				GetVMTypedArithmeticCommand(cmd.op_code, common_type),
				left.used_register,
				left.used_register,
				right.used_register
			));

			current_BDS.regsTable.Release(right.used_register);
			current_BDS.value_stack.push(ValueFrame(left.used_register, common_type));
			break;
		}

		case PseudoOpCode::Negative:
		{
			if (current_BDS.value_stack.size() < 1)
			{
				Logger::Get().PrintTypeError("Negative is unary operation, but gets only zero.", ip);
				break;
			}
			ValueFrame vf_left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

			// Negative только для знаковых типов (INT, DOUBLE)
			if (vf_left.value_type != Type::VMAnalog::INT && vf_left.value_type != Type::VMAnalog::DOUBLE) {
				Logger::Get().PrintTypeError("Negative operation requires signed types (int, double).", ip);
				break;
			}

			result.push_back(MalachiteCore::VMCommand(GetVMTypedArithmeticCommand(cmd.op_code, vf_left.value_type),
				vf_left.used_register, vf_left.used_register));
			current_BDS.value_stack.push(ValueFrame(vf_left.used_register, vf_left.value_type));
			break;
		}
		}
		return result;
	}
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleLogicCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand cmd = cmds[ip];
		auto main_logic_handler = [&]() -> void
			{
				if (current_BDS.value_stack.size() < 2)
				{
					Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " is binary operation, but gets only one.", ip);
					return;
				}
				ValueFrame right = current_BDS.value_stack.top(); current_BDS.value_stack.pop();
				ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

				result.push_back(MalachiteCore::VMCommand(
					GetVMLogicCommand(cmd.op_code,Type::VMAnalog::UINT),
					left.used_register,
					left.used_register,
					right.used_register
				));

				current_BDS.regsTable.Release(right.used_register);
				current_BDS.value_stack.push(ValueFrame(left.used_register, Type::VMAnalog::UINT));	//bool 0,1
			};
		auto main_cmp_handler = [&](uint32_t flag) -> void
			{
				if (current_BDS.value_stack.size() < 2)
				{
					Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " is binary operation, but gets only one.", ip);
					return;
				}
				ValueFrame right = current_BDS.value_stack.top(); current_BDS.value_stack.pop();
				ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

				uint64_t converted_reg;
				Type::VMAnalog common_type;
				auto conv_cmd = GetVMTypeConvertionCommand(
					left.value_type, left.used_register,
					right.value_type, right.used_register,
					converted_reg, common_type
				);
				if (conv_cmd.operation != MalachiteCore::OpCode::OP_NOP) {
					result.push_back(conv_cmd);
				}

				result.push_back(MalachiteCore::VMCommand(
					GetVMLogicCommand(cmd.op_code, common_type),
					0,						//In CMP and DCMP destination is null
					left.used_register,
					right.used_register
				));
				result.push_back(MalachiteCore::VMCommand(
					MalachiteCore::OpCode::OP_GET_FLAG,
					left.used_register,
					flag
				));
				current_BDS.regsTable.Release(right.used_register);
				current_BDS.value_stack.push(ValueFrame(left.used_register, Type::VMAnalog::UINT));	//flag 0,1
			};
		switch (cmd.op_code)
		{
		case PseudoOpCode::And:
			main_logic_handler();
			break;
		case PseudoOpCode::Or:
			main_logic_handler();
			break;
		case PseudoOpCode::Not:
		{
			if (current_BDS.value_stack.size() < 1)
			{
				Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " is unary operation, but gets only zero.", ip);
				break;
			}
			ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

			result.push_back(MalachiteCore::VMCommand(
				MalachiteCore::OpCode::OP_NOT_RR,
				left.used_register,
				left.used_register
			));

			current_BDS.value_stack.push(ValueFrame(left.used_register, Type::VMAnalog::UINT));	//bool 0,1
		}
			break;
		case PseudoOpCode::BitOr:
			main_logic_handler();
			break;
		case PseudoOpCode::BitNot:
		{
			if (current_BDS.value_stack.size() < 1)
			{
				Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " is unary operation, but gets only zero.", ip);
				break;
			}
			ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();

			result.push_back(MalachiteCore::VMCommand(
				MalachiteCore::OpCode::OP_BIT_NOT_RR,
				left.used_register,
				left.used_register
			));

			current_BDS.value_stack.push(ValueFrame(left.used_register, Type::VMAnalog::UINT));	//bool 0,1
		}
			break;
		case PseudoOpCode::BitAnd:
			main_logic_handler();
			break;
		case PseudoOpCode::BitOffsetLeft:
			main_logic_handler();
			break;
		case PseudoOpCode::BitOffsetRight:
			main_logic_handler();
			break;

		//Comparings

		case PseudoOpCode::Equal:
			main_cmp_handler(MalachiteCore::FLAG::EQUAL_FLAG);
			break;
		case PseudoOpCode::NotEqual:
			main_cmp_handler(MalachiteCore::FLAG::NOT_EQUAL_FLAG);
			break;
		case PseudoOpCode::Greater:
			main_cmp_handler(MalachiteCore::FLAG::GREATER_FLAG);
			break;
		case PseudoOpCode::Less:
			main_cmp_handler(MalachiteCore::FLAG::LESS_FLAG);
			break;
		case PseudoOpCode::GreaterEqual:
			main_cmp_handler(MalachiteCore::FLAG::EQUAL_FLAG | MalachiteCore::FLAG::GREATER_FLAG);
			break;
		case PseudoOpCode::LessEqual:
			main_cmp_handler(MalachiteCore::FLAG::EQUAL_FLAG | MalachiteCore::FLAG::LESS_FLAG);
			break;
		default:
			break;
		}
		return result;
	}
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleControlFlowCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand cmd = cmds[ip];
		switch (cmd.op_code)
		{
			case PseudoOpCode::Jump:
			{
				uint64_t l_id = cmd.parameters[PseudoCodeInfo::Get().labelID_name].uintVal;

				if (!current_BDS.labels.count(l_id))
				{
					current_BDS.waiting_jumps[l_id].push_back({ ip,current_BDS.current_commands->size() });
					result.push_back(MalachiteCore::VMCommand(
						MalachiteCore::OpCode::OP_JMP,
						l_id
					));
				}
				else 
				{
					uint64_t jump_ip = current_BDS.labels[l_id];
					result.push_back(MalachiteCore::VMCommand(
						MalachiteCore::OpCode::OP_JMP,
						jump_ip
					));
				}
			}
			break;
			case PseudoOpCode::JumpIf:
			{
				if (current_BDS.value_stack.size() < 1)
				{
					Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " needs a boolean value.", ip);
					break;
				}
				ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();
				uint64_t l_id = cmd.parameters[PseudoCodeInfo::Get().labelID_name].uintVal;

				if (!current_BDS.labels.count(l_id))
				{
					current_BDS.waiting_jumps[l_id].push_back({ ip,current_BDS.current_commands->size() });
					result.push_back(MalachiteCore::VMCommand(
						MalachiteCore::OpCode::OP_JMP_CV,
						l_id,
						left.used_register
					));
				}
				else
				{
					uint64_t jump_ip = current_BDS.labels[l_id];
					result.push_back(MalachiteCore::VMCommand(
						MalachiteCore::OpCode::OP_JMP_CV,
						jump_ip,
						left.used_register));
				}
				current_BDS.regsTable.Release(left.used_register);
			}
			break;
			case PseudoOpCode::JumpNotIf:
			{
				if (current_BDS.value_stack.size() < 1)
				{
					Logger::Get().PrintTypeError(SyntaxInfo::GetPseudoString(cmd.op_code) + " needs a boolean value.", ip);
					break;
				}
				ValueFrame left = current_BDS.value_stack.top(); current_BDS.value_stack.pop();
				uint64_t l_id = cmd.parameters[PseudoCodeInfo::Get().labelID_name].uintVal;

				if (!current_BDS.labels.count(l_id))
				{
					current_BDS.waiting_jumps[l_id].push_back({ ip,current_BDS.current_commands->size() });
					result.push_back(MalachiteCore::VMCommand(
						MalachiteCore::OpCode::OP_JMP_CNV,
						l_id,
						left.used_register
					));
				}
				else
				{
					uint64_t jump_ip = current_BDS.labels[l_id];
					result.push_back(MalachiteCore::VMCommand(
						MalachiteCore::OpCode::OP_JMP_CNV,
						jump_ip,
						left.used_register));
				}
				current_BDS.regsTable.Release(left.used_register);
			}
			break;
			case PseudoOpCode::Label:
			{
				uint64_t l_id = cmd.parameters[PseudoCodeInfo::Get().labelID_name].uintVal;

				if (current_BDS.labels.count(l_id))
				{
					Logger::Get().PrintTypeError("Label with id = " + std::to_string(l_id) + " already exists.",ip);
					break;
				}

				auto last_ip = current_BDS.current_commands->size();

				current_BDS.labels[l_id] = last_ip;
				if (current_BDS.waiting_jumps.count(l_id))
				{
					for (auto& jump_ip : current_BDS.waiting_jumps[l_id])
					{
						current_BDS.current_commands->at(jump_ip.second).destination = last_ip;
					}
					current_BDS.waiting_jumps.erase(l_id);
				}
			}
			break;
		}
		return result;
	}
	std::vector<MalachiteCore::VMCommand> ByteDecoder::HandleMemoryCommand(const std::vector<PseudoCommand>& cmds, size_t ip)
	{
		std::vector<MalachiteCore::VMCommand> result;
		PseudoCommand cmd = cmds[ip];
		switch (cmd.op_code)
		{
		case PseudoOpCode::Immediate:
		{
			TokenValue val = cmd.parameters[PseudoCodeInfo::Get().valueID_name];
			auto free_register = current_BDS.regsTable.Allocate();
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
				current_BDS.value_stack.push(ValueFrame(val, free_register, Type::VMAnalog::INT));
				return result;
			case Malachite::TokenValueType::UINT:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_UINT, free_register, MalachiteCore::Register(val.uintVal)));
				current_BDS.value_stack.push(ValueFrame(val, free_register, Type::VMAnalog::UINT));
				return result;

			case Malachite::TokenValueType::FLOAT:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_DOUBLE, free_register, MalachiteCore::Register(val.floatVal)));
				current_BDS.value_stack.push(ValueFrame(val, free_register, Type::VMAnalog::DOUBLE));
				return result;
			case Malachite::TokenValueType::STRING:
				Logger::Get().PrintTypeError("Attemp of direct immediating string value. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
				return result;
			case Malachite::TokenValueType::CHAR:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_INT, free_register, MalachiteCore::Register((int64_t)val.charVal)));
				current_BDS.value_stack.push(ValueFrame(val, free_register, Type::VMAnalog::INT));
				return result;
			case Malachite::TokenValueType::BOOL:
				result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_MOV_RI_UINT, free_register, MalachiteCore::Register((uint64_t)val.boolVal)));
				current_BDS.value_stack.push(ValueFrame(val, free_register, Type::VMAnalog::UINT));
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
			Variable& var = current_BDS.current_state->variables_global_table.at(cmd.parameters[PseudoCodeInfo::Get().variableID_name].uintVal);
			Type& type = current_BDS.current_state->types_global_table.at(var.type_id);
			auto info = current_BDS.variable_depth.at(var.variable_id);
			auto free_register = current_BDS.regsTable.Allocate();
			if (free_register == InvalidRegister)
			{
				Logger::Get().PrintLogicError("All registers are in using. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
				break;
			}
			size_t size = type.size;
			if (type.category == Type::Category::PRIMITIVE)
			{
				//PseudoDecoder checked vars validity
				if (info.depth == current_BDS.current_depth)
				{
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_LOAD_LOCAL, free_register, info.stack_offset, type.size));

				}
				else //
				{
					uint64_t size_and_depth = size << 32;	//0...size -> size...0
					size_and_depth |= info.depth;	//uint64_t and int64_t size...0 -> size...depth
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_LOAD_ENCLOSING_A, free_register, info.stack_offset, size_and_depth));
				}
				if (type.vm_analog == Type::VMAnalog::NONE)
				{
					Logger::Get().PrintLogicError("Primitive type \"" + type.name + "\" hasnt analog in the Malachite Virtual Machine.Instruction pointer of pseudo code : " + std::to_string(ip), ip);
					break;
				}
				current_BDS.value_stack.push(ValueFrame(var.variable_id, free_register, type.vm_analog));
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
			Variable& var = current_BDS.current_state->variables_global_table.at(cmd.parameters[PseudoCodeInfo::Get().variableID_name].uintVal);
			Type& type = current_BDS.current_state->types_global_table.at(var.type_id);
			auto info = current_BDS.variable_depth.at(var.variable_id);

			// We need some history about register using. As example, we have this expression: x = 2 * 3
			// 2 and 3 in values stack, but where is multiplier's result? 
			// I think we should to save to values stack value frame with fields register_to_save (or another name) and type = OPERATION_RESULT
			// It also must be when we call function or something another

			ValueFrame& vf = current_BDS.value_stack.top();
			current_BDS.value_stack.pop();
			if (vf.used_register == InvalidRegister)
			{
				Logger::Get().PrintLogicError("All registers are in using. Instruction pointer of pseudo code: " + std::to_string(ip), ip);
			}
			size_t size = type.size;
			if (type.category == Type::Category::PRIMITIVE)
			{
				//PseudoDecoder checked vars validity
				if (vf.value_type != type.vm_analog) {
					// Генерируем команду конвертации
					auto conv_cmd_opcode = GetVMTypeConvertionCommand(vf.value_type, type.vm_analog);
					if (conv_cmd_opcode != MalachiteCore::OpCode::OP_NOP) {
						result.push_back(MalachiteCore::VMCommand(conv_cmd_opcode, vf.used_register));
					}
				}
				if (info.depth == current_BDS.current_depth)
				{
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_STORE_LOCAL, info.stack_offset, vf.used_register, type.size));
				}
				else //
				{
					uint64_t size_and_depth = size << 32;	//0...size -> size...0
					size_and_depth |= info.depth;	//uint64_t and int64_t size...0 -> size...depth
					result.push_back(MalachiteCore::VMCommand(MalachiteCore::OpCode::OP_STORE_ENCLOSING_A, info.stack_offset, vf.used_register, size_and_depth));
				}
				current_BDS.regsTable.Release(vf.used_register);
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
}
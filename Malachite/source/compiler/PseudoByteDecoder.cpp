#include "..\..\include\compiler\PseudoByteDecoder.hpp"
#include "..\..\include\compiler\StringOperations.hpp"


namespace Malachite 
{
	std::vector<PseudoCommand> PseudoByteDecoder::ParseForBlock(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		bool math_mode = node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_for_math;



		return std::vector<PseudoCommand>();
	}
	std::vector<PseudoCommand> PseudoByteDecoder::ParseCycles(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;
		if (node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_loop) 
		{
			//loop handling
		}
		else if (node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_while)
		{
			//while handling
		}
		else if (node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_for || node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_for_math)
		{
			result = ParseForBlock(node, state);
		}
		return result;
	}
	std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> PseudoByteDecoder::GeneratePseudoCode(const ASTNode& node)
	{
		return GeneratePseudoCode(node.children);
	}
	std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> PseudoByteDecoder::GeneratePseudoCode(const std::vector<ASTNode>& node)
	{
		auto compilation_state = std::make_shared<CompilationState>();
		std::vector<PseudoCommand> result;
		result.push_back(PseudoCommand(PseudoOpCode::ScopeStart));
		for (auto& n : node) 
		{
			auto result1 = RecursiveHandle(n, compilation_state);
			result.insert(result.end(), result1.begin(), result1.end());
		}
		result.push_back(PseudoCommand(PseudoOpCode::ScopeEnd));
		return std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>>(compilation_state, result);
	}
	std::vector<PseudoCommand> PseudoByteDecoder::RecursiveHandle(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		size_t children = node.children.size();
		std::vector<PseudoCommand> result;
		if (children > 0 && node.tokens.size() > 0) //If ASTNode has a header and the children its functional block 
		{
			auto r = HandleBasicSyntax(node,state);
			if (r.size() == 0) //Bug: Header of ast node - its common expression with children (optionality ";" had an effect) 
			{	
				auto r = ex_decoder.DecodeExpression(node, state);
				result.insert(result.end(), r.begin(), r.end());
				for (size_t i = 0; i < children; i++)
				{
					auto r = RecursiveHandle(node.children[i],state);
					result.insert(result.end(), r.begin(), r.end());
				}
			}
			else //Its real block construction (if\elif\else\while\for)
			{
				result = r;
			}
		}
		else if (children > 0 && node.tokens.size() == 0)	// visible frame without header {code....}
		{
			for (size_t i = 0; i < children; i++)
			{	
				auto r = RecursiveHandle(node.children[i],state);
				result.insert(result.end(), r.begin(), r.end());
			}
		}
		else if(children == 0 && node.tokens.size() > 0)
		{
			result = ex_decoder.DecodeExpression(node,state);
		}
		return result;
	}

	std::vector<PseudoCommand> PseudoByteDecoder::HandleBasicSyntax(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;

		if (node.tokens[0].type == TokenType::COMPILATION_LABEL && node.tokens[0].value.uintVal == (uint64_t)CompilationLabel::NODES_GROUP) 
		{
			if (auto& child = node.children[0]; child.tokens.size() > 0 && child.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_if) result = ParseConditionBlock(node,state);	//"if" is the start block
		}
		if (node.tokens[0].type == TokenType::KEYWORD)
		{
			if (auto& child = node.children[0]; child.tokens.size() > 0 && node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_op_code)
			{
				result = ParseOpCodeBlock(node,state);	//"if" is the start block
			}
			if (SyntaxInfo::GetLoopsList().count(node.tokens[0].value.strVal)) 
			{
				result = ParseCycles(node, state);
			}
		}
		

		//else if
		return result;
	}

	std::vector<PseudoCommand> PseudoByteDecoder::ParseConditionBlock(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> commands;

		bool use_end_label = node.children.size() > 1;

		uint64_t skip_label_id = use_end_label ? PseudoCodeInfo::Get().GetNewLabelID() : SIZE_MAX;	//Skip label for exit if one if/elif/else block will work (insert to end)

		for (auto& child_node : node.children)
		{
			if ((child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_if || child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_elif) && child_node.tokens.size() < 4) {		//if/elif (true) - minimum
				Logger::Get().PrintSyntaxError("Condition's checking structure (" + SyntaxInfoKeywords::Get().keyword_if + ","+ SyntaxInfoKeywords::Get().keyword_elif + ") is invalid.", child_node.line);
				continue;
			}
			if (child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_else && child_node.tokens.size() > 1 && child_node.tokens[1].value.strVal != ":") {		//else : or {}, without condition
				Logger::Get().PrintSyntaxError("Condition's checking structure (" +SyntaxInfoKeywords::Get().keyword_else + ") is invalid. Else doesnt have a condition in any form.", child_node.line);
				continue;
			}
			size_t condition_start_index = SIZE_MAX;
			size_t condition_end_index = SIZE_MAX;

			bool is_single_string = false;	//if (true): operation
			int depth = 0;
			for (int i = 0; i < child_node.tokens.size(); i++)
			{
				Token t = child_node.tokens[i];
				if (t.value.strVal == "(")
				{
					if (depth == 0)
					{
						condition_start_index = i;
					}
					depth++;
				}
				else if (t.value.strVal == ")")
				{
					depth--;
					if (depth == 0) 
					{
						condition_end_index = i;
					}
					else if (depth < 0) 
					{
						Logger::Get().PrintSyntaxError("Unexpected ')'.", child_node.line);
					}
				}
				if (t.value.strVal == ":")
				{
					if (condition_end_index == SIZE_MAX && child_node.tokens[0].value.strVal != SyntaxInfoKeywords::Get().keyword_else) {
						Logger::Get().PrintSyntaxError("Unexpected ':' without condition.", child_node.line);
					}
					else if (child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_else)
					{
						condition_start_index = 0;
						condition_end_index = i;
					}
					if (i == child_node.tokens.size() - 1) 
					{
						Logger::Get().PrintSyntaxError("Missing command after ':' in single operation mode.", child_node.line);
					}
					if (is_single_string) 
					{
						Logger::Get().PrintSyntaxError("Unexpected ':'.", child_node.line);
					}
					is_single_string = true;
				}
			}
			
			std::vector<PseudoCommand> block_commands;

			uint64_t exit_label_id = SIZE_MAX;	//Label for exit, if condition is false
			if (child_node.tokens[0].value.strVal != SyntaxInfoKeywords::Get().keyword_else)
			{
				exit_label_id = PseudoCodeInfo::Get().GetNewLabelID();
				auto condition_tokens = StringOperations::TrimVector<Token>(child_node.tokens, 1, condition_end_index);	//if (...condition...)
				auto condition_commands = ex_decoder.DecodeExpression(condition_tokens, state);
				block_commands.insert(block_commands.end(), condition_commands.begin(), condition_commands.end());
				block_commands.push_back(PseudoCommand(PseudoOpCode::JumpNotIf, { {PseudoCodeInfo::Get().labelID_name, TokenValue(exit_label_id)} }));	//op_code, where

			}
			
			if (!is_single_string) 
			{
				//Body
				for (auto& expression : child_node.children)
				{
					auto expression_commands = RecursiveHandle(expression,state);
					block_commands.insert(block_commands.end(), expression_commands.begin(), expression_commands.end());
				}
			}
			else	// if/elif (true): operation
			{
				auto operation_tokens = StringOperations::TrimVector<Token>(child_node.tokens, condition_end_index+1, child_node.tokens.size()-1);	
				auto operation_commands = ex_decoder.DecodeExpression(operation_tokens, state);
				block_commands.insert(block_commands.end(), operation_commands.begin(), operation_commands.end());
			}
			//Insert jump's label and exit's jump
			if (use_end_label) block_commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, TokenValue(skip_label_id)} }));	//op_code, where (behind the condition block)
			if (child_node.tokens[0].value.strVal != SyntaxInfoKeywords::Get().keyword_else)block_commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name, TokenValue(exit_label_id)},{PseudoCodeInfo::Get().labelMark_name, TokenValue("NextConditionLabel")} }));
			commands.insert(commands.end(), block_commands.begin(), block_commands.end());
		}

		if (use_end_label) commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name, TokenValue(skip_label_id)},{PseudoCodeInfo::Get().labelMark_name, TokenValue("SkipLabel")}}));

		return commands;
	}

	std::vector<PseudoCommand> PseudoByteDecoder::ParseOpCodeBlock(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> commands;
		commands.push_back(PseudoCommand(PseudoOpCode::OpCodeStart, { {PseudoCodeInfo::Get().sectionName_name, node.tokens.size() >= 2 ? node.tokens[1].value.strVal : ""} }));
		commands.push_back(PseudoCommand(PseudoOpCode::OpCodeAllocateBasicRegisters));
		for (size_t i = 0; i < node.children.size(); i++) 
		{
			//STRUCTURE: OpCode destination, source0,source1,imm
			if (node.children[i].tokens.size() == 0) {Logger::Get().PrintWarning("OpCode command doenst have anybody arguments. Operation will be skipped.", node.children[i].line);continue;}
			std::vector<Token> without_delimiters;
			for (auto& t : node.children[i].tokens) 
			{
				if (t.type == TokenType::COMPILATION_LABEL && t.value.uintVal == (uint64_t)CompilationLabel::OPERATION_END) break;
				if (t.type != TokenType::DELIMITER)without_delimiters.push_back(t);
			}
			if (without_delimiters.size() == 0){Logger::Get().PrintSyntaxError("Invalid opcode command.", node.children[i].line);continue;}

			//token analysis
			//first - opcode
			PseudoCommand pc;
			pc.op_code = PseudoOpCode::OpCodeCommand;
			auto opcode_str = without_delimiters.front().value.strVal;
			if (opcode_str.empty()) 
			{
				Logger::Get().PrintSyntaxError("Null opcode operation.", node.children[i].line);
				continue;
			}
			auto arguments = StringOperations::TrimVector<Token>(without_delimiters, 1, without_delimiters.size() - 1);

			//Load/Store variable

			if (opcode_str == PseudoCodeInfo::Get().opcodeStoreCommand_name && arguments.size() >= 2) //dest - variable src0 - register or imm - constant
			{
				pc.op_code = PseudoOpCode::OpCodeStoreVR;
				auto* var = state->FindVariable(arguments[0].value.strVal);
				if (!var) 
				{
					Logger::Get().PrintLogicError("Undefined variable \"" + arguments[0].value.strVal + "\" in opcode section.", node.children[i].line);
					continue;
				}
				pc.parameters[PseudoCodeInfo::Get().opcodeDestination_name] = var->variable_id;
				auto value = arguments[1];
				if (SyntaxInfo::GetOpCodeRegistersList().count(value.value.strVal)) 
				{
					pc.parameters[PseudoCodeInfo::Get().opcodeSource0_name] = value.value.strVal;
				}
				else 
				{
					pc.parameters[PseudoCodeInfo::Get().opcodeImmediate_name] = value.value;
				}
				commands.push_back(pc);
				continue;
			}
			else if (opcode_str == PseudoCodeInfo::Get().opcodeLoadCommand_name && arguments.size() >= 2) //dest - register src0 - variable
			{
				pc.op_code = PseudoOpCode::OpCodeLoadRV;
				auto* var = state->FindVariable(arguments[1].value.strVal);
				if (!var && !SyntaxInfo::GetOpCodeRegistersList().count(arguments[1].value.strVal))
				{
					Logger::Get().PrintLogicError("Undefined variable \"" + arguments[1].value.strVal + "\" in opcode section.", node.children[i].line);
					continue;
				}
				auto register_ = arguments[0];
				if (SyntaxInfo::GetOpCodeRegistersList().count(register_.value.strVal))
				{
					pc.parameters[PseudoCodeInfo::Get().opcodeDestination_name] = register_.value.strVal;
				}
				else
				{
					Logger::Get().PrintLogicError("Invalid register \"" + register_.value.strVal + "\" in opcode section.", node.children[i].line);
					continue;
				}
				pc.parameters[PseudoCodeInfo::Get().opcodeSource0_name] = var->variable_id;
				commands.push_back(pc);
				continue;
			}

			pc.parameters[PseudoCodeInfo::Get().opcodeCommandCode_name] = (uint64_t)SyntaxInfo::GetByteFromString(opcode_str);

			

			//All another cases
			for (size_t j = 0; j < arguments.size() && j < 5; j++) // j < 5 because vm command contains only 4 argument (OpCode isnt included)
			{
				std::string arg_name = PseudoCodeInfo::Get().opcodeImmediate_name;
				if (j == 0) arg_name = PseudoCodeInfo::Get().opcodeDestination_name;
				if (j == 1 && (arguments[j].type == TokenType::IDENTIFIER)) arg_name = PseudoCodeInfo::Get().opcodeSource0_name;
				if (j == 2 && (arguments[j].type == TokenType::IDENTIFIER)) arg_name = PseudoCodeInfo::Get().opcodeSource1_name;

				TokenValue value_ = arguments[j].value;

				if (arguments[j].type == TokenType::IDENTIFIER && !SyntaxInfo::GetOpCodeRegistersList().count(arguments[j].value.strVal))
				{
					auto constant = SyntaxInfo::GetOpCodeConstant(value_.strVal);
					if (constant == SIZE_MAX)
					{
						Logger::Get().PrintLogicError("Invalid opcode constant \"" + value_.strVal + "\".", node.children[i].line);
						continue;
					}
					else 
					{
						value_ = constant;
					}
				}

				if (arg_name == PseudoCodeInfo::Get().opcodeImmediate_name)pc.parameters[arg_name] = value_;
				else //check registers
				{
					if (SyntaxInfo::GetOpCodeRegistersList().count(arguments[j].value.strVal))
					{
						pc.parameters[arg_name] = value_.strVal;
					}
					else if (value_.type == TokenValueType::INT)pc.parameters[arg_name] = value_.intVal;
					else if (value_.type == TokenValueType::UINT)pc.parameters[arg_name] = value_.uintVal;
					else if (value_.strVal != "")
					{
						Logger::Get().PrintLogicError("Invalid register \"" + value_.strVal + "\" in opcode section.", node.children[i].line);
						continue;
					}
				}
				
			}
			commands.push_back(pc);
		}
		commands.push_back(PseudoCommand(PseudoOpCode::OpCodeEnd));

		return commands;
	}
	
}
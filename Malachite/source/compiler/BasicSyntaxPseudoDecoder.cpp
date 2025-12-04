#include "..\..\include\compiler\BasicSyntaxPseudoDecoder.hpp"
#include "..\..\include\compiler\StringOperations.hpp"

namespace Malachite
{
	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::ParseWhileBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)
	{
		std::vector<PseudoCommand> commands;
		if (node.tokens.size() < 4) { Logger::Get().PrintSyntaxError("Invalid while cycle. Invalid cycle header's structure.", node.tokens[0].line); return commands; }
		std::vector<Token> condition;
		int depth = 0;

	
		for (size_t i = 0; i < node.tokens.size(); i++)
		{
			Token t = node.tokens[i];
			if (t.type == TokenType::DELIMITER)
			{
				if (t.value.strVal == "(") { 
					depth++;
					if (depth == 1) continue; 
				}
				else if (t.value.strVal == ")") { depth--; if (depth == 0) break; }
			}
			if (depth > 0) condition.push_back(t);
		}
		if (condition.size() < 1) { Logger::Get().PrintSyntaxError("Invalid while cycle. Invalid cycle's condition.", node.tokens[0].line); return commands; }

		//Labels
		std::string label_while_check = StringOperations::GenerateLabel("#while_check");
		std::string label_while_end = StringOperations::GenerateLabel("#while_end");
		auto checking_label_id = PseudoCodeInfo::Get().GetNewLabelID();
		auto end_label_id = PseudoCodeInfo::Get().GetNewLabelID();

		auto condition_commands = ex_decoder.DecodeExpression(condition, state);
		//-------Checking-------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,checking_label_id},{PseudoCodeInfo::Get().labelMark_name,label_while_check} }));
		commands.insert(commands.end(), condition_commands.begin(), condition_commands.end());
		commands.push_back(PseudoCommand(PseudoOpCode::JumpNotIf, { {PseudoCodeInfo::Get().labelID_name, end_label_id} }));

		//Auto inserting OVS and CVS in ASTBuilder <----

		//-------Body-------

		std::vector<PseudoCommand> body_commands;
		for (ASTNode child : node.children) {
			auto childChain = rh(child, state);
			body_commands.insert(body_commands.end(), childChain.begin(), childChain.end());
		}

		int depth_in_body = 0;	//Is 0 because ASTBuilder inserted OPEN_VISIBLE_SCOPE in body (child nodes)  

		for (int i = 0; i < body_commands.size(); i++)
		{
			//Search unhandled break and continue
			PseudoCommand& c = body_commands[i];
			if (c.op_code == PseudoOpCode::OpenVisibleScope) depth_in_body++;
			else if (c.op_code == PseudoOpCode::CloseVisibleScope) depth_in_body--;
			if (c.op_code == PseudoOpCode::ExceptHandling)
			{	//Work's soo awful
				if (depth_in_body > 0)
				{
					commands.push_back(PseudoCommand(PseudoOpCode::CloseVisibleScopes, { {PseudoCodeInfo::Get().valueID_name,(uint64_t)depth_in_body} })); //
				}
				if (c.parameters[PseudoCodeInfo::Get().labelMark_name].strVal == SyntaxInfoKeywords::Get().keyword_break)
				{
					c.op_code = PseudoOpCode::Jump;
					c.parameters[PseudoCodeInfo::Get().labelID_name] = end_label_id;
				}
				if (c.parameters[PseudoCodeInfo::Get().labelMark_name].strVal == SyntaxInfoKeywords::Get().keyword_continue)
				{
					c.op_code = PseudoOpCode::Jump;
					c.parameters[PseudoCodeInfo::Get().labelID_name] = checking_label_id;
				}
			}
			commands.push_back(c);
		}
		commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, checking_label_id} }));			//Jump back to start
		//-----------------End-----------------------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,end_label_id},{PseudoCodeInfo::Get().labelMark_name,label_while_end } }));
		return commands;
	}
	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::ParseForBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)	//foreach and forint will be in individual handlers
	{
		std::vector<PseudoCommand> commands;
		if (node.tokens.size() < 7) { Logger::Get().PrintSyntaxError("Invalid for cycle. Invalid cycle's structure.", node.tokens[0].line); return commands; }
		std::vector<std::vector<Token>> args;
		args.push_back({});
		int arg_index = 0;
		int depth = 0;

		Token variable = node.tokens[1];
		if (variable.type != TokenType::IDENTIFIER) { Logger::Get().PrintSyntaxError("Invalid for cycle. Invalid or missed cycle's variable \"" + variable.value.strVal + "\"", node.tokens[0].line); return commands; }
		for (size_t i = 0; i < node.tokens.size(); i++)
		{
			Token t = node.tokens[i];
			if (t.type == TokenType::DELIMITER)
			{
				if (t.value.strVal == "(") 
				{ 
					depth++; 
					if (depth == 1)	continue;
				}
				else if (t.value.strVal == ")") { depth--; if (depth == 0) break; }
				if (t.value.strVal == ",")
				{
					if (depth == 0) { Logger::Get().PrintSyntaxError("Invalid for cycle. Delimiter ',' is outside.", node.tokens[0].line); return commands; }
					else if (depth == 1) { args.push_back({}); arg_index++; continue; }
				}
			}
			if (depth > 0)args[arg_index].push_back(t);
		}
		if (args.size() < 2) { Logger::Get().PrintSyntaxError("Invalid for cycle. Invalid cycle's args.", node.tokens[0].line); return commands; }
		if (args.size() == 2) args.push_back({ Token(TokenType::LITERAL, (int64_t)1) });	//Set default step

		//Labels
		std::string label_for_check = StringOperations::GenerateLabel("#for_check");
		std::string label_for_add = StringOperations::GenerateLabel("#for_add");
		std::string label_for_end = StringOperations::GenerateLabel("#for_end");
		std::string label_for_body = StringOperations::GenerateLabel("#for_body");
		std::string label_for_greater = StringOperations::GenerateLabel("#for_greater");
		//Pseudonyms of registers
		std::string c_for_start = StringOperations::GenerateLabel("#c_for_start");
		std::string c_for_end = StringOperations::GenerateLabel("#c_for_end");
		std::string c_for_step = StringOperations::GenerateLabel("#c_for_step");

		auto checking_label_id = PseudoCodeInfo::Get().GetNewLabelID();
		auto add_label_id = PseudoCodeInfo::Get().GetNewLabelID();
		auto end_label_id = PseudoCodeInfo::Get().GetNewLabelID();
		auto body_label_id = PseudoCodeInfo::Get().GetNewLabelID();
		auto greater_label_id = PseudoCodeInfo::Get().GetNewLabelID();

		auto start = ex_decoder.DecodeExpression(args[0], state);
		auto end = ex_decoder.DecodeExpression(args[1], state);
		auto step = ex_decoder.DecodeExpression(args[2], state);

		commands.insert(commands.end(), start.begin(), start.end());
		commands.push_back(PseudoCommand(PseudoOpCode::SaveToRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_start} }));
		commands.insert(commands.end(), end.begin(), end.end());
		commands.push_back(PseudoCommand(PseudoOpCode::SaveToRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_end} }));
		commands.insert(commands.end(), step.begin(), step.end());
		commands.push_back(PseudoCommand(PseudoOpCode::SaveToRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_step} }));

		//-------Declaring of cycle's variable-------
		ASTNode scope_start;
		scope_start.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPEN_VISIBLE_SCOPE, -1));
		auto  pc_scope_start = ex_decoder.DecodeExpression(scope_start, state);
		commands.push_back(pc_scope_start.back());		//For cycle is the exception in ASTBuilder (OpenVisibleScope/CloseVisibleScope auto inserting is disable)

		Variable var(variable.value.strVal, state->FindType(SyntaxInfoKeywords::Get().typemarker_int)->type_id, false);
		state->AddVariableToCurrentSpace(var);
		PseudoCommand declare_cmd(PseudoOpCode::DeclareVariable);
		declare_cmd.parameters[PseudoCodeInfo::Get().variableID_name] = var.variable_id;
		declare_cmd.parameters[PseudoCodeInfo::Get().typeID_name] = state->FindType(SyntaxInfoKeywords::Get().typemarker_int)->type_id;
		commands.push_back(declare_cmd);


		//------------Set start value----------------
		commands.push_back(PseudoCommand(PseudoOpCode::LoadFromRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_start} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Store, { {PseudoCodeInfo::Get().variableID_name,var.variable_id } }));
		//---------------Checking--------------------

		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,checking_label_id},{PseudoCodeInfo::Get().labelMark_name,label_for_check} }));
		// Step checking: step > 0 - direct, step < 0 - reversed
		commands.push_back(PseudoCommand(PseudoOpCode::LoadFromRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_step} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Immediate, { {PseudoCodeInfo::Get().valueID_name,(int64_t)0} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Less));
		commands.push_back(PseudoCommand(PseudoOpCode::JumpNotIf, { {PseudoCodeInfo::Get().labelID_name, greater_label_id} }));
		//Less-----(Reversed cycle)---[...)
		commands.push_back(PseudoCommand(PseudoOpCode::Load, { {PseudoCodeInfo::Get().variableID_name,var.variable_id } }));
		commands.push_back(PseudoCommand(PseudoOpCode::LoadFromRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_end} }));
		commands.push_back(PseudoCommand(PseudoOpCode::LessEqual));
		commands.push_back(PseudoCommand(PseudoOpCode::JumpNotIf, { {PseudoCodeInfo::Get().labelID_name, body_label_id} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, end_label_id} }));
		//Greater---(Direct cycle)----[...)
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,greater_label_id},{PseudoCodeInfo::Get().labelMark_name,label_for_greater} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Load, { {PseudoCodeInfo::Get().variableID_name,var.variable_id } }));
		commands.push_back(PseudoCommand(PseudoOpCode::LoadFromRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_end} }));
		commands.push_back(PseudoCommand(PseudoOpCode::GreaterEqual));
		commands.push_back(PseudoCommand(PseudoOpCode::JumpNotIf, { {PseudoCodeInfo::Get().labelID_name, body_label_id} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, end_label_id} }));
		//-----------------Body----------------------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,body_label_id},{PseudoCodeInfo::Get().labelMark_name,label_for_body} }));

		scope_start.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPEN_VISIBLE_SCOPE, -1));
		pc_scope_start = ex_decoder.DecodeExpression(scope_start, state);
		commands.push_back(pc_scope_start.back());

		std::vector<PseudoCommand> body_commands;
		for (ASTNode child : node.children) {
			auto childChain = rh(child, state);
			body_commands.insert(body_commands.end(), childChain.begin(), childChain.end());
		}

		int depth_in_body = 1;	//Corrected by 1 because we insert OPEN_VISIBLE_SCOPE in start of body ( they are not in the children )

		for (int i = 0; i < body_commands.size(); i++)
		{
			//Search unhandled break and continue
			PseudoCommand& c = body_commands[i];
			if (c.op_code == PseudoOpCode::OpenVisibleScope) depth_in_body++;
			else if (c.op_code == PseudoOpCode::CloseVisibleScope) depth_in_body--;
			if (c.op_code == PseudoOpCode::ExceptHandling)
			{	//Work's soo awful
				if (depth_in_body > 0)
				{
					commands.push_back(PseudoCommand(PseudoOpCode::CloseVisibleScopes, { {PseudoCodeInfo::Get().valueID_name,(uint64_t)depth_in_body} })); //
				}
				if (c.parameters[PseudoCodeInfo::Get().labelMark_name].strVal == SyntaxInfoKeywords::Get().keyword_break)
				{
					c.op_code = PseudoOpCode::Jump;
					c.parameters[PseudoCodeInfo::Get().labelID_name] = end_label_id;
				}
				if (c.parameters[PseudoCodeInfo::Get().labelMark_name].strVal == SyntaxInfoKeywords::Get().keyword_continue)
				{
					c.op_code = PseudoOpCode::Jump;
					c.parameters[PseudoCodeInfo::Get().labelID_name] = add_label_id;
				}
			}
			commands.push_back(c);
		}
		ASTNode scope_end;
		scope_end.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::CLOSE_VISIBLE_SCOPE));	//delete all from cycle's body
		auto pc_scope_end = ex_decoder.DecodeExpression(scope_end, state);
		commands.push_back(pc_scope_end.back());
		//Before label because continue has already deleted scopes | break delete's all body's visible scope's and it jumps to end_label
		//-----------------Add-----------------------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,add_label_id},{PseudoCodeInfo::Get().labelMark_name,label_for_add } }));
		commands.push_back(PseudoCommand(PseudoOpCode::Load, { {PseudoCodeInfo::Get().variableID_name,var.variable_id } }));
		commands.push_back(PseudoCommand(PseudoOpCode::LoadFromRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_step} }));
		commands.push_back(PseudoCommand(PseudoOpCode::Add));
		commands.push_back(PseudoCommand(PseudoOpCode::Store, { {PseudoCodeInfo::Get().variableID_name,var.variable_id } }));	//Save new value to variable
		commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, checking_label_id} }));			//Jump back to checking section
		//-----------------End-----------------------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,end_label_id},{PseudoCodeInfo::Get().labelMark_name,label_for_end } }));
		commands.push_back(PseudoCommand(PseudoOpCode::ReleaseRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_start} }));
		commands.push_back(PseudoCommand(PseudoOpCode::ReleaseRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_end} }));
		commands.push_back(PseudoCommand(PseudoOpCode::ReleaseRegisterWithPseudonym, { {PseudoCodeInfo::Get().pseudonym_name,c_for_step} }));
		scope_end.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::CLOSE_VISIBLE_SCOPE));	//with cycle's variable
		pc_scope_end = ex_decoder.DecodeExpression(scope_end, state);
		commands.push_back(pc_scope_end.back());
		return commands;
	}

	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::ParseLoopBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)
	{
		std::vector<PseudoCommand> commands;
		std::string label_loop_start = StringOperations::GenerateLabel("#loop_start");
		std::string label_loop_end = StringOperations::GenerateLabel("#loop_end");
		auto loop_start_label_id = PseudoCodeInfo::Get().GetNewLabelID();
		auto loop_end_label_id = PseudoCodeInfo::Get().GetNewLabelID();

		//--------------------Header--------------------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,loop_start_label_id},{PseudoCodeInfo::Get().labelMark_name,label_loop_start} }));
		//---------------------Body---------------------
		//Auto inserting OVS and CVS in ASTBuilder <----
		std::vector<PseudoCommand> body_commands;
		for (ASTNode child : node.children) {
			auto childChain = rh(child, state);
			body_commands.insert(body_commands.end(), childChain.begin(), childChain.end());
		}

		int depth_in_body = 0;	//Is 0 because ASTBuilder inserted OPEN_VISIBLE_SCOPE in body (child nodes)  

		for (int i = 0; i < body_commands.size(); i++)
		{
			//Search unhandled break and continue
			PseudoCommand& c = body_commands[i];
			if (c.op_code == PseudoOpCode::OpenVisibleScope) depth_in_body++;
			else if (c.op_code == PseudoOpCode::CloseVisibleScope) depth_in_body--;
			if (c.op_code == PseudoOpCode::ExceptHandling)
			{	//Work's soo awful
				if (depth_in_body > 0)
				{
					commands.push_back(PseudoCommand(PseudoOpCode::CloseVisibleScopes, { {PseudoCodeInfo::Get().valueID_name,(uint64_t)depth_in_body} })); //
				}
				if (c.parameters[PseudoCodeInfo::Get().labelMark_name].strVal == SyntaxInfoKeywords::Get().keyword_break)
				{
					c.op_code = PseudoOpCode::Jump;
					c.parameters[PseudoCodeInfo::Get().labelID_name] = loop_end_label_id;
				}
				if (c.parameters[PseudoCodeInfo::Get().labelMark_name].strVal == SyntaxInfoKeywords::Get().keyword_continue)
				{
					c.op_code = PseudoOpCode::Jump;
					c.parameters[PseudoCodeInfo::Get().labelID_name] = loop_start_label_id;
				}
			}
			commands.push_back(c);
		}
		commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, loop_start_label_id} }));			//Jump back to start
		//---------------------Exit---------------------
		commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name,loop_end_label_id},{PseudoCodeInfo::Get().labelMark_name,label_loop_end} }));
		return commands;
	}


	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::ParseCycles(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)
	{
		std::vector<PseudoCommand> result;
		if (node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_loop)
		{
			result = ParseLoopBlock(node, state,rh);
			//loop handling
		}
		else if (node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_while)
		{
			result = ParseWhileBlock(node, state, rh);
		}
		else if (node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_for)
		{
			result = ParseForBlock(node, state,rh);
		}
		// node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_for_math
		return result;
	}

	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::HandleBasicSyntax(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)
	{
		std::vector<PseudoCommand> result;

		if (node.tokens[0].type == TokenType::COMPILATION_LABEL && node.tokens[0].value.uintVal == (uint64_t)CompilationLabel::NODES_GROUP)
		{
			if (auto& child = node.children[0]; child.tokens.size() > 0 && child.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_if) result = ParseConditionBlock(node, state,rh);	//"if" is the start block
		}
		if (node.tokens[0].type == TokenType::KEYWORD)
		{
			if (auto& child = node.children[0]; child.tokens.size() > 0 && node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_op_code)
			{
				result = ParseOpCodeBlock(node, state,rh);	//"if" is the start block
			}
			if (SyntaxInfo::GetLoopsList().count(node.tokens[0].value.strVal))
			{
				result = ParseCycles(node, state,rh);
			}
		}


		//else if
		return result;
	}

	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::ParseConditionBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)
	{
		std::vector<PseudoCommand> commands;

		bool use_end_label = node.children.size() > 1;

		uint64_t skip_label_id = use_end_label ? PseudoCodeInfo::Get().GetNewLabelID() : SIZE_MAX;	//Skip label for exit if one if/elif/else block will work (insert to end)

		for (auto& child_node : node.children)
		{
			if ((child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_if || child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_elif) && child_node.tokens.size() < 4) {		//if/elif (true) - minimum
				Logger::Get().PrintSyntaxError("Condition's checking structure (" + SyntaxInfoKeywords::Get().keyword_if + "," + SyntaxInfoKeywords::Get().keyword_elif + ") is invalid.", child_node.line);
				continue;
			}
			if (child_node.tokens[0].value.strVal == SyntaxInfoKeywords::Get().keyword_else && child_node.tokens.size() > 1 && child_node.tokens[1].value.strVal != ":") {		//else : or {}, without condition
				Logger::Get().PrintSyntaxError("Condition's checking structure (" + SyntaxInfoKeywords::Get().keyword_else + ") is invalid. Else doesnt have a condition in any form.", child_node.line);
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
					auto expression_commands = rh(expression, state);
					block_commands.insert(block_commands.end(), expression_commands.begin(), expression_commands.end());
				}
			}
			else	// if/elif (true): operation
			{
				auto operation_tokens = StringOperations::TrimVector<Token>(child_node.tokens, condition_end_index + 2, child_node.tokens.size() - 1);		// +2 - skip :
				auto operation_commands = ex_decoder.DecodeExpression(operation_tokens, state);
				block_commands.insert(block_commands.end(), operation_commands.begin(), operation_commands.end());
			}
			//Insert jump's label and exit's jump
			if (use_end_label) block_commands.push_back(PseudoCommand(PseudoOpCode::Jump, { {PseudoCodeInfo::Get().labelID_name, TokenValue(skip_label_id)} }));	//op_code, where (behind the condition block)
			if (child_node.tokens[0].value.strVal != SyntaxInfoKeywords::Get().keyword_else)block_commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name, TokenValue(exit_label_id)},{PseudoCodeInfo::Get().labelMark_name, TokenValue("NextConditionLabel")} }));
			commands.insert(commands.end(), block_commands.begin(), block_commands.end());
		}

		if (use_end_label) commands.push_back(PseudoCommand(PseudoOpCode::Label, { {PseudoCodeInfo::Get().labelID_name, TokenValue(skip_label_id)},{PseudoCodeInfo::Get().labelMark_name, TokenValue("SkipLabel")} }));

		return commands;
	}

	std::vector<PseudoCommand> BasicSyntaxPseudoDecoder::ParseOpCodeBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh)
	{
		std::vector<PseudoCommand> commands;
		commands.push_back(PseudoCommand(PseudoOpCode::OpCodeStart, { {PseudoCodeInfo::Get().sectionName_name, node.tokens.size() >= 2 ? node.tokens[1].value.strVal : ""} }));
		commands.push_back(PseudoCommand(PseudoOpCode::OpCodeAllocateBasicRegisters));
		for (size_t i = 0; i < node.children.size(); i++)
		{
			//STRUCTURE: OpCode destination, source0,source1,imm
			if (node.children[i].tokens.size() == 0) { Logger::Get().PrintWarning("OpCode command doenst have anybody arguments. Operation will be skipped.", node.children[i].line); continue; }
			std::vector<Token> without_delimiters;
			for (auto& t : node.children[i].tokens)
			{
				if (t.type == TokenType::COMPILATION_LABEL && t.value.uintVal == (uint64_t)CompilationLabel::OPERATION_END) break;
				if (t.type != TokenType::DELIMITER)without_delimiters.push_back(t);
			}
			if (without_delimiters.size() == 0) { Logger::Get().PrintSyntaxError("Invalid opcode command.", node.children[i].line); continue; }

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
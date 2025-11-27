#include "..\..\include\compiler\ExpressionDecoder.hpp"
#include "..\..\include\compiler\StringOperations.hpp"


namespace Malachite 
{

	std::vector<TokensGroup> ExpressionDecoder::ToPostfixForm(const std::vector<Token>& original)
	{
		std::vector<TokensGroup> operations;		// i dont want use the stack, because it doens has index accept
		std::vector<TokensGroup> result;		//identifiers and literals

		for (size_t i = 0; i < original.size(); i++) 
		{
			const Token& t = original[i];

			if (t.type == TokenType::IDENTIFIER && i + 1 < original.size() && original[i + 1].type == TokenType::DELIMITER && original[i + 1].value.strVal == "(") {
				// Нашли вызов функции - создаём сложную группу
				TokensGroup func_call;
				func_call.token.line = t.line;	//for debugging
				Token label(TokenType::COMPILATION_LABEL,(uint64_t)CompilationLabel::FUNCTION_CALL,t.line);	//in arrays CompilationLabel::OFFSET_ACCESS
				func_call.tokens.push_back(TokensGroup(label));	//вспомогательная метка
				func_call.tokens.push_back(TokensGroup(t)); // имя функции

				i += 2; // пропускаем имя и "("
				std::vector<Token> args_tokens;
				int depth = 1;

				// Собираем аргументы рекурсивно
				while (i < original.size() && depth > 0) {
					if (original[i].value.strVal == "(") depth++;
					else if (original[i].value.strVal == ")") depth--;
					else if (original[i].value.strVal == "," && depth == 1) {
						// Нашли разделитель аргументов
						if (!args_tokens.empty()) {
							TokensGroup arg;
							auto arg_postfix = ToPostfixForm(args_tokens);
							arg.tokens.insert(arg.tokens.end(),
								arg_postfix.begin(), arg_postfix.end());
							args_tokens.clear();
							func_call.tokens.push_back(arg);
						}
						i++;
						continue;
					}

					if (depth > 0) {
						args_tokens.push_back(original[i]);
					}
					i++;
				}
				i--; // компенсация
				if (!args_tokens.empty()) {
					TokensGroup arg;
					auto arg_postfix = ToPostfixForm(args_tokens);
					arg.tokens.insert(arg.tokens.end(),
						arg_postfix.begin(), arg_postfix.end());
					args_tokens.clear();
					func_call.tokens.push_back(arg);
				}
				result.push_back(func_call);
				Logger::Get().PrintInfo("ToPostfix: discovered function's call \"" + func_call.tokens[0].token.value.strVal + "\" with " + std::to_string(func_call.tokens.size() - 2) + " arguments.");
			}
			else if (t.type == TokenType::IDENTIFIER || t.type == TokenType::LITERAL) {
				result.push_back(TokensGroup(t));
			}
			else if (t.type == TokenType::OPERATOR) {
				while (!operations.empty() &&
					operations.back().token.value.strVal != "(" &&  // dont push out"("
					(SyntaxInfo::GetOperationPriority(operations.back().token) >= SyntaxInfo::GetOperationPriority(t))) {
					result.push_back(operations.back());
					operations.pop_back();
				}
				operations.push_back(TokensGroup(t));
			}
			else if (t.value.strVal == "(") {  // ← Simple delimiter handling
				operations.push_back(TokensGroup(t));
			}
			else if (t.value.strVal == ")") {
				while (!operations.empty() && operations.back().token.value.strVal != "(") {
					result.push_back(operations.back());
					operations.pop_back();
				}
				if (!operations.empty()) operations.pop_back(); // deleting "("
			}
		}

		while (!operations.empty()) {
			result.push_back(operations.back());
			operations.pop_back();
		}

		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::PTP_HandleFunctionCall(const std::vector<TokensGroup>& postfix, std::shared_ptr<CompilationState> state)
	{
		//PTP-PostfixToPseude (Help method)
		//Function's call handle
		//The first is compilation label - we somehow ended up here
		if (postfix.size() == 1) { Logger::Get().PrintLogicError("Invalid function call.", postfix[0].token.line);  return std::vector<PseudoCommand>(); }
		std::string func_name = postfix[1].token.value.strVal;
		//Functions search
		auto* functions = state->FindFunctions(func_name);
		if (!functions)
		{
			Logger::Get().PrintLogicError("Functions overloadings with name \"" + func_name + "\" dont exist.", postfix[1].token.line);
			return std::vector<PseudoCommand>();
		}
		//Args handling
		std::vector<PseudoCommand> result;
		size_t args_count = postfix.size() - 2;	// 2 - it are two first token (comp. label + name)

		//Type checking later
		for (size_t i = 2; i < postfix.size(); i++)
		{
			TokensGroup tg = postfix[i];
			std::vector<TokensGroup> tgv = {tg};	//<- костыль!!!
			auto commands = PTP_HandleExpression(tgv, state);
			result.insert(result.end(), commands.begin(), commands.end()); //x*x = LOAD LOAD MUL
			result.push_back(PseudoCommand(PseudoOpCode::Push));	//Command without args -> stack principle 
			//x*x = LOAD LOAD MUL PUSH
			//Idk how ByteCoder (or ByteDecoder) will handle it 
			}
		std::vector<functionID> valid_functions;
		
		for (size_t i = 0; i < functions->size(); i++) 
		{
			Function function = state->functions_global_table[(*functions)[i]];
		
			if (function.args.size() == args_count) valid_functions.push_back((*functions)[i]);
		
		}
		if (valid_functions.size() == 0) {
			Logger::Get().PrintLogicError("Function's overloading with name \"" + func_name + "\" doesnt exist.", postfix[1].token.line);
			return std::vector<PseudoCommand>();
		}
		result.push_back(PseudoCommand(PseudoOpCode::Call, { {PseudoCodeInfo::Get().functionID_name, valid_functions[0]} }));
		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::PTP_HandleExpression(const std::vector<TokensGroup>& postfix, std::shared_ptr<CompilationState> state)
	{
		//PTP-PostfixToPseude (Help method)
		std::vector<PseudoCommand> result;
		int debug_line = -1;

		for (size_t i = 0; i < postfix.size(); i++)
		{
			TokensGroup tg = postfix[i];
			if (tg.type == TokenGroupType::SIMPLE)
			{
				Token t = tg.token;
				debug_line = t.line;
				if (t.type == TokenType::IDENTIFIER)
				{
					//Add type converting later
					if (auto* variable = state->FindVariable(t.value.strVal); variable)
					{
						result.push_back(PseudoCommand(PseudoOpCode::Load, { {PseudoCodeInfo::Get().variableID_name, variable->variable_id} }));
					}
					else
					{
						Logger::Get().PrintLogicError("Undefined identificator \"" + t.value.strVal + "\" in current scope.", t.line);
						continue;
					}
				}
				if (t.type == TokenType::OPERATOR) result.push_back(PseudoCommand(SyntaxInfo::GetOperatorPseudoCode(t)));
				if (t.type == TokenType::LITERAL) result.push_back(PseudoCommand(PseudoOpCode::Immediate, { {PseudoCodeInfo::Get().valueID_name, t.value } }));
			}
			else // Complex
			{
				if (tg.tokens.size() == 0)
				{
					Logger::Get().PrintWarning("Empty expression.", debug_line == -1 ? tg.token.line : debug_line);
					continue;
				}

				Token type = tg.tokens[0].token;

				if (type.type != TokenType::COMPILATION_LABEL) //At the current moment of development we are hz what is it
				{
					Logger::Get().PrintWarning("Invalid complex expression.", type.line );
					continue;
				}
				switch (type.value.uintVal)	//check by COMPILATION_LABEL (its first in the array)
				{
				case (uint64_t)CompilationLabel::FUNCTION_CALL:	
				{
					auto result1 = PTP_HandleFunctionCall(tg.tokens, state);
					result.insert(result.end(), result1.begin(), result1.end());
					break;
				}
				case (uint64_t)CompilationLabel::METHOD_CALL:
				{
					break;
				}
				case (uint64_t)CompilationLabel::FIELD_ACCESS:
				{
					break;
				}
				case (uint64_t)CompilationLabel::OFFSET_ACCESS:
				{
					break;
				}
				default:
					Logger::Get().PrintWarning("Invalid complex expression.", type.line);
					continue;
				}
			}
		}
		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::PostfixToPseudo(const std::vector<TokensGroup>& postfix, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;
		
		result = PTP_HandleExpression(postfix,state);

		return result;
	}
	
	std::vector<PseudoCommand> ExpressionDecoder::ProcessLeftSide(const std::vector<Token>& left, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;
		bool is_const = false;

		for (size_t i = 0; i < left.size(); i++) {
			const Token& t = left[i];

			// Проверка на const
			if (t.type == TokenType::KEYWORD && t.value.strVal == "const") {
				is_const = true;
				continue;
			}

			// Объявление переменной: "int x"
			if (t.type == TokenType::TYPE_MARKER && i + 1 < left.size() &&
				left[i + 1].type == TokenType::IDENTIFIER)
			{
				return HandleVariableDeclaration(left, i, is_const, state);
			}
			// Объявление кастомной переменной: "Vector x"
			else if ((t.type == TokenType::IDENTIFIER && state->FindType(t.value.strVal)) && i + 1 < left.size() &&
				left[i + 1].type == TokenType::IDENTIFIER)
			{
				return HandleVariableDeclaration(left, i, is_const, state); 
			}
			// Присваивание существующей переменной: "x"
			if (t.type == TokenType::IDENTIFIER) {
				return HandleVariableAssignment(t, is_const, state);
			}
		}

		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::HandleVariableDeclaration(const std::vector<Token>& left, size_t type_index, bool is_const, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;
		const Token& type_token = left[type_index];
		const Token& name_token = left[type_index + 1];

		// Проверка типа
		auto* type = state->FindType(type_token.value.strVal);
		if (!type) {
			Logger::Get().PrintTypeError("Type \"" + type_token.value.strVal + "\" doesn't exist", type_token.line);
			return result;
		}

		// Проверка на повторное объявление
		if (state->GetCurrentSpace()->variables_table.IsExists(name_token.value.strVal)) {
			Logger::Get().PrintLogicError("Redeclaring variable \"" + name_token.value.strVal + "\"", name_token.line);
			return result;
		}

		// Добавляем переменную в таблицу
		Variable var(name_token.value.strVal, type->type_id, is_const);
		state->AddVariableToSpace(state->GetCurrentSpace()->vfid,var);

		// Генерируем команду объявления
		PseudoCommand declare_cmd(PseudoOpCode::DeclareVariable);
		declare_cmd.parameters[PseudoCodeInfo::Get().variableID_name] = var.variable_id;
		declare_cmd.parameters[PseudoCodeInfo::Get().typeID_name] = type->type_id;
		result.push_back(declare_cmd);

		// Генерируем команду сохранения (значение уже в стеке от правой части)
		PseudoCommand store_cmd(PseudoOpCode::Store);		//или LOAD ARITHMETIC_OPER STORE 
		store_cmd.parameters[PseudoCodeInfo::Get().variableID_name] = var.variable_id;
		result.push_back(store_cmd);

		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::HandleVariableAssignment(const Token& var_name, bool is_const, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;

		// Проверка существования переменной
		auto* variable = state->FindVariable(var_name.value.strVal);
		if (!variable) {
			Logger::Get().PrintTypeError("Variable \"" + var_name.value.strVal + "\" is not declared", var_name.line); 
			return result;
		}

		// Проверка на const
		auto& var = *variable;
		if (var.is_const) {
			Logger::Get().PrintTypeError("Variable \"" + var_name.value.strVal + "\" is constant", var_name.line); 
			return result;
		}

		PseudoCommand store_cmd(PseudoOpCode::Store);
		store_cmd.parameters[PseudoCodeInfo::Get().variableID_name] = variable->variable_id;
		result.push_back(store_cmd);

		return result;
	}

	std::vector<PseudoCommand> ExpressionDecoder::ProcessRightSide(const std::vector<Token>& right, std::shared_ptr<CompilationState> state)
	{
		std::vector<TokensGroup> tgs = ToPostfixForm(right);


		return PostfixToPseudo(tgs,state);
	}

	std::vector<PseudoCommand> ExpressionDecoder::DecodeAssignExpression(const std::vector<Token>& left, const std::vector<Token>& right, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;

		// 1. Обрабатываем правую часть (значение)
		std::vector<PseudoCommand> right_commands = ProcessRightSide(right, state);
		result.insert(result.end(), right_commands.begin(), right_commands.end());

		// 2. Обрабатываем левую часть (куда сохранять)
		std::vector<PseudoCommand> left_commands = ProcessLeftSide(left, state);
		result.insert(result.end(), left_commands.begin(), left_commands.end());

		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::DecodeExpression(const std::vector<Token>& tokens, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;

		std::vector<Token> temp_tokens;
		for (size_t i = 0; i < tokens.size(); i++)
		{
			const Token& t = tokens[i];
			if (t.type == TokenType::COMPILATION_LABEL)
			{
				switch ((CompilationLabel)t.value.uintVal)
				{
				case CompilationLabel::SCOPE_START:
					state->PushSpace();
					result.push_back(PseudoCommand(PseudoOpCode::ScopeStart));
					break;
				case CompilationLabel::SCOPE_END:
					if (!state->HasSpaces())
					{
						Logger::Get().PrintLogicError("Unexpected scope escape of variables", t.line);
						return result;
					}
					state->PopSpace();
					result.push_back(PseudoCommand(PseudoOpCode::ScopeEnd));
					break;
				default:
					break;
				}
				return result;
			}

			if (t.type == TokenType::OPERATOR && SyntaxInfo::GetOperationPriority(t) == -1)
			{
				std::vector<Token> right = StringOperations::TrimVector<Token>(tokens, i + 1, tokens.size() - 1);
				result = DecodeAssignExpression(temp_tokens, right, state);
				temp_tokens.clear();
				break;
			}

			temp_tokens.push_back(t);
		}
		if (!temp_tokens.empty())
		{
			std::vector<PseudoCommand> commands = ProcessRightSide(temp_tokens, state);
			result.insert(result.end(), commands.begin(), commands.end());
		}
		return result;
	}
	std::vector<PseudoCommand> Malachite::ExpressionDecoder::DecodeExpression(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		return DecodeExpression(node.tokens, state);
	}
}



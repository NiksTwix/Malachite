#include "..\..\include\compiler\ArithmeticDecoder.hpp"
#include "..\..\include\compiler\StringOperations.hpp"


namespace Malachite 
{
	Type* ExpressionDecoder::FindType(std::shared_ptr<CompilationState> state, const std::string& type_name)
	{
		size_t current_depth = state->GetSpacesDepth();
		for (; current_depth >= 0; current_depth--) 
		{
			if (state->GetSpace(current_depth)->types_table.IsExists(type_name)) return &state->GetSpace(current_depth)->types_table[type_name];
		}
		return nullptr;
	}
	Variable* ExpressionDecoder::FindVariable(std::shared_ptr<CompilationState> state, const std::string& variable_name)
	{
		size_t current_depth = state->GetSpacesDepth();
		for (; current_depth >= 0; current_depth--)
		{
			if (state->GetSpace(current_depth)->variables_table.IsExists(variable_name)) return &state->GetSpace(current_depth)->variables_table[variable_name];
		}
		return nullptr;
	}
	const std::vector<functionID>* ExpressionDecoder::FindFunctions(std::shared_ptr<CompilationState> state, const std::string& function_name)
	{
		size_t current_depth = state->GetSpacesDepth();
		for (; current_depth >= 0; current_depth--)
		{
			if (state->GetSpace(current_depth)->functions_table.IsExists(function_name)) return &state->GetSpace(current_depth)->functions_table[function_name];
		}
		return nullptr;
	}

	std::vector<ExpressionDecoder::TokensGroup> ExpressionDecoder::ToPostfixForm(const std::vector<Token>& original)
	{
		std::vector<TokensGroup> operations;		// i dont want use the stack, because it doens has index accept
		std::vector<TokensGroup> result;		//identifiers and literals

		for (size_t i = 0; i < original.size(); i++) 
		{
			const Token& t = original[i];

			if (t.type == TokenType::IDENTIFIER && i + 1 < original.size() && original[i + 1].type == TokenType::DELIMITER && original[i + 1].value.strVal == "(") {
				// Нашли вызов функции - создаём сложную группу
				TokensGroup func_call;
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
							auto arg_postfix = ToPostfixForm(args_tokens);
							func_call.tokens.insert(func_call.tokens.end(),
								arg_postfix.begin(), arg_postfix.end());
							args_tokens.clear();
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

				result.push_back(func_call);
				Logger::Get().PrintInfo("ToPostfix: discovered function's call \"" + func_call.tokens[0].token.value.strVal + "\" with " + std::to_string(func_call.tokens.size() - 3) + " arguments.");
			}
			else if (t.type == TokenType::IDENTIFIER || t.type == TokenType::LITERAL) result.push_back(TokensGroup(t)); //push variable or literal to result(values) stack
			else if (t.type == TokenType::OPERATOR) 
			{
				while (!operations.empty() && (SyntaxInfo::GetOperationPriority(operations.back().token) > SyntaxInfo::GetOperationPriority(t))) {
					result.push_back(operations.back());
					operations.pop_back();
				}
				operations.push_back(TokensGroup(t));
			}
			else if (original[i + 1].type == TokenType::DELIMITER && t.value.strVal == "(") {
				operations.push_back(TokensGroup(t));
			}
			else if (t.value.strVal == ")") {
				while (!operations.empty() && operations.back().token.value.strVal != "(") {
					result.push_back(operations.back());
					operations.pop_back();
				}
				operations.pop_back(); // удаляем "("
			}
		}
		while (!operations.empty()) {
			result.push_back(operations.back());
			operations.pop_back();
		}

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
		auto* type = FindType(state, type_token.value.strVal);
		if (!type) {
			Logger::Get().PrintTypeError("Type \"" + type_token.value.strVal + "\" doesn't exist", type_token.line);
			return result;
		}

		// Проверка на повторное объявление
		if (state->GetCurrentSpace()->variables_table.IsExists(name_token.value.strVal)) {
			Logger::Get().PrintTypeError("Redeclaring variable \"" + name_token.value.strVal + "\"", name_token.line);
			return result;
		}

		// Добавляем переменную в таблицу
		Variable var(name_token.value.strVal, type->type_id, is_const);
		state->GetCurrentSpace()->variables_table.AddVariable(var);

		// Генерируем команду объявления
		PseudoCommand declare_cmd(PseudoOpCode::DeclareVariable);
		declare_cmd.parameters["name"] = name_token.value.strVal;
		declare_cmd.parameters["type"] = type->type_id;
		result.push_back(declare_cmd);

		// Генерируем команду сохранения (значение уже в стеке от правой части)
		PseudoCommand store_cmd(PseudoOpCode::Store);		//или LOAD ARITHMETIC_OPER STORE 
		store_cmd.parameters["name"] = name_token.value.strVal;
		result.push_back(store_cmd);

		return result;
	}
	std::vector<PseudoCommand> ExpressionDecoder::HandleVariableAssignment(const Token& var_name, bool is_const, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;

		// Проверка существования переменной
		auto* variable = FindVariable(state, var_name.value.strVal);
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
		store_cmd.parameters["name"] = var_name.value.strVal;
		result.push_back(store_cmd);

		return result;
	}

	std::vector<PseudoCommand> ExpressionDecoder::ProcessRightSide(const std::vector<Token>& left, std::shared_ptr<CompilationState> state)
	{



		return std::vector<PseudoCommand>();
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
	std::vector<PseudoCommand> Malachite::ExpressionDecoder::DecodeExpression(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		std::vector<PseudoCommand> result;

		std::vector<Token> temp_tokens;
		for (size_t i = 0; i < node.tokens.size(); i++) 
		{
			const Token& t = node.tokens[i];
			if (t.type == TokenType::COMPILATION_LABEL) 
			{
				switch ((CompilationLabel)t.value.uintVal)
				{
				case CompilationLabel::SCOPE_START:
					state->PushSpace();
				case CompilationLabel::SCOPE_END:
					if (!state->HasSpaces()) 
					{
						Logger::Get().PrintLogicError("Unexpected scope escape of variables", t.line);
						return result;
					}
					state->PopSpace();
				default:
					break;
				}
				return result;
			}

			if (t.type == TokenType::OPERATOR && SyntaxInfo::GetOperationPriority(t) == -1)
			{
				std::vector<Token> right = StringOperations::TrimVector<Token>(node.tokens, i,node.tokens.size() - 1);
				result = DecodeAssignExpression(temp_tokens, right, state);
				break;
			}
			
			temp_tokens.push_back(t);
		}
		return result;
	}
}



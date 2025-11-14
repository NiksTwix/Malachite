#include "..\..\include\compiler\ASTBuilder.hpp"


namespace Malachite 
{
	void ASTBuilder::if_scope_exception(Token& t)
	{
		if (scope_exceptions.count(t.value.strVal))
		{
			skip_scopes.push({ true,current_depth });
		}
	}
	ASTNode ASTBuilder::BuildAST(std::vector<Token>& tokens)
	{
		ASTNode parent;

		std::stack<ASTNode> cn_stack;

		cn_stack.push(parent);

		std::vector<Token> command_tokens;

		current_depth = 0;
		int last_line = 0;
		for (int i = 0; i < tokens.size(); i++)
		{
			Token t = tokens[i];
			last_line = t.line;
			if_scope_exception(t);

			if (t.value.strVal == "{")
			{
				if (!command_tokens.empty())
				{
					ASTNode node;
					node.tokens = command_tokens;
					node.line = command_tokens.back().line;
					command_tokens.clear();
					cn_stack.push(node);
				}
				else
				{
					// Создаём пустую ноду для блока без заголовка
					ASTNode empty_node;
					empty_node.line = t.line;  
					cn_stack.push(empty_node);
				}

				if (skip_scopes.empty() || !(skip_scopes.top().flag && skip_scopes.top().depth == current_depth))
				{
					ASTNode scope_start;
					scope_start.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::SCOPE_START, -1, current_depth));
					cn_stack.top().children.push_back(scope_start);
				}

				current_depth++;
				continue;
			}
			if (t.value.strVal == "}")
			{
				if (cn_stack.size() <= 1)
				{
					Logger::Get().PrintSyntaxError("There are extra ones }.", t.line);
					return parent;
				}

				auto node = cn_stack.top();
				cn_stack.pop();
				cn_stack.top().children.push_back(node);

				///Конец области видимости переменных
				if (skip_scopes.empty() || !(skip_scopes.top().flag && skip_scopes.top().depth + 1 == current_depth))
				{
					ASTNode scope_end;
					scope_end.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::SCOPE_END, -1, current_depth));	//TODO инрементировать, если надо
					cn_stack.top().children.push_back(scope_end);
				}
				else if (skip_scopes.top().flag && skip_scopes.top().depth + 1 == current_depth)
				{
					skip_scopes.pop();
				}
				current_depth--;
				continue;
			}
			if (t.type == TokenType::COMPILATION_LABEL && t.value.uintVal == (uint64_t)CompilationLabel::OPERATION_END)
			{
				if (command_tokens.size() != 0)
				{
					ASTNode node;
					command_tokens.push_back(t);
					node.tokens = command_tokens;
					node.line = command_tokens.back().line;
					command_tokens.clear();
					cn_stack.top().children.push_back(node);
				}
				continue;
			}
			if (current_depth < 0)
			{
				Logger::Get().PrintSyntaxError("There are extra ones } .", t.line);
				return parent;
			}
			command_tokens.push_back(t);

		}

		if (command_tokens.size() != 0)
		{
			ASTNode node;
			node.tokens = command_tokens;
			node.line = command_tokens.back().line;
			command_tokens.clear();
			cn_stack.top().children.push_back(node);
		}

		if (current_depth > 0)
		{
			Logger::Get().PrintSyntaxError("Expected } .", last_line);
			return parent;
		}

		return cn_stack.top();
	}
}
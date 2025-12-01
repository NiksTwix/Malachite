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
	ASTNode ASTBuilder::UniteToGroup(std::vector<ASTNode>& nodes, CompilationLabel label)
	{
		if (nodes.empty()) return ASTNode();
		ASTNode node;

		node.line = nodes[0].line;
		node.tokens.push_back(Token(TokenType::COMPILATION_LABEL,(uint64_t)label));	

		node.children.insert(node.children.end(), nodes.begin(), nodes.end());

		return node;
	}
	void ASTBuilder::PostprocessTree(ASTNode& node)
	{
		if (node.children.size() == 0) return;

		std::vector<ASTNode> group_of_nodes;	//IF-ELIF-ELSE, 
		std::vector<size_t> group_of_nodes_indices; //index in array

		auto insert_group_node{ [&]() -> void
			{
				if (!group_of_nodes.empty())
				{
					auto united = UniteToGroup(group_of_nodes, CompilationLabel::NODES_GROUP);
					node.children[group_of_nodes_indices[0]] = united;

					for (size_t j = 1; j < group_of_nodes_indices.size(); j++)
					{
						node.children[group_of_nodes_indices[j]] = ASTNode();	//Create clear ast node, they will not be handled
					}
					group_of_nodes.clear();
					group_of_nodes_indices.clear();
				}
			} 
		};


		for (size_t i = 0; i < node.children.size(); i++) 
		{
			ASTNode& child_node = node.children[i];

			PostprocessTree(child_node);	//Postprocessing is the first

			if (child_node.tokens.size() == 0)continue;

			if (auto type = SyntaxInfo::GetCondtionBlockPartType(child_node.tokens[0].value.strVal); type != SyntaxInfo::ConditionBlockParType::NOTHING) 
			{
				switch (type)
				{
				case Malachite::SyntaxInfo::START:
					insert_group_node();
					//Push if block as start_position
					group_of_nodes.push_back(child_node);
					group_of_nodes_indices.push_back(i);
					break;
				case Malachite::SyntaxInfo::MIDDLE:
					if (group_of_nodes.empty() || 
						(!group_of_nodes.empty() && SyntaxInfo::GetCondtionBlockPartType(group_of_nodes.back().tokens[0].value.strVal) != SyntaxInfo::ConditionBlockParType::START &&
							SyntaxInfo::GetCondtionBlockPartType(group_of_nodes.back().tokens[0].value.strVal) != SyntaxInfo::ConditionBlockParType::MIDDLE))
					{
						Logger::Get().PrintLogicError("Condition block without start command \"" + SyntaxInfoKeywords::Get().keyword_if + "\" or middle command \"" + SyntaxInfoKeywords::Get().keyword_elif + "\".", child_node.line);
						break;
					}
					//Push elif block as middle
					group_of_nodes.push_back(child_node);
					group_of_nodes_indices.push_back(i);
					break;
				case Malachite::SyntaxInfo::END:
					if (group_of_nodes.empty() || 
						(!group_of_nodes.empty() && SyntaxInfo::GetCondtionBlockPartType(group_of_nodes.back().tokens[0].value.strVal) != SyntaxInfo::ConditionBlockParType::START &&
							SyntaxInfo::GetCondtionBlockPartType(group_of_nodes.back().tokens[0].value.strVal) != SyntaxInfo::ConditionBlockParType::MIDDLE))
					{
						Logger::Get().PrintLogicError("Condition block without start command \"" + SyntaxInfoKeywords::Get().keyword_if + "\" or middle command \"" + SyntaxInfoKeywords::Get().keyword_elif + "\".", child_node.line);
						break;
					}
					//Push else block as end
					group_of_nodes.push_back(child_node);
					group_of_nodes_indices.push_back(i);
					insert_group_node();
					break;
				}
			}
			else 
			{
				insert_group_node();
			}
		}
		insert_group_node();	//last_check
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
					scope_start.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPEN_VISIBLE_SCOPE, -1, current_depth));
					cn_stack.top().children.push_back(scope_start);
				}

				current_depth++;
				continue;
			}
			if (t.value.strVal == "}")
			{
				if (cn_stack.size() <= 1)
				{
					Logger::Get().PrintSyntaxError("There are extra ones '}'.", t.line);
					return parent;
				}
				///Конец области видимости переменных

				if (skip_scopes.empty() || !(skip_scopes.top().flag && skip_scopes.top().depth + 1 == current_depth))
				{
					ASTNode scope_end;
					scope_end.tokens.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::CLOSE_VISIBLE_SCOPE, -1, current_depth));	//TODO инрементировать, если надо
					cn_stack.top().children.push_back(scope_end);
				}
				else if (skip_scopes.top().flag && skip_scopes.top().depth + 1 == current_depth)
				{
					skip_scopes.pop();
				}
				auto node = cn_stack.top();
				cn_stack.pop();
				cn_stack.top().children.push_back(node);

				
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
				Logger::Get().PrintSyntaxError("There are extra ones '}'.", t.line);
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
			Logger::Get().PrintSyntaxError("Expected '}'.", last_line);
			return parent;
		}

		return cn_stack.top();
	}
}
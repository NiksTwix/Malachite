#pragma once
#include "Definitions.hpp"
#include "Logger.hpp"
#include  <unordered_set>
#include <stack>

namespace Malachite 
{
	struct ASTNode
	{
		std::vector<Token> tokens;
		std::vector<ASTNode> children;
		int line;
	};

	class ASTBuilder 
	{
	private:
		struct SkipScope {
			bool flag = false;
			int depth = -1;
		};

		std::unordered_set<std::string> scope_exceptions = { 
			SyntaxInfoKeywords::Get().keyword_func,
			SyntaxInfoKeywords::Get().keyword_op_code,
			SyntaxInfoKeywords::Get().keyword_class,
			SyntaxInfoKeywords::Get().keyword_alias,
			SyntaxInfoKeywords::Get().keyword_for};	//ќбъ€влени€, в блоках кода которых не надо ставить автоматический SCOPE

		int current_depth = 0;

		std::stack<SkipScope> skip_scopes;	

		void if_scope_exception(Token& t);

		ASTNode UniteToGroup(std::vector<ASTNode>& nodes, CompilationLabel label);

		

	public:
		ASTNode BuildAST(std::vector<Token>& tokens);
		void PostprocessTree(ASTNode& node);
	};

}
#pragma once
#include "ExpressionDecoder.hpp"
#include <functional>



namespace Malachite 
{

	using recursive_handler = std::function<std::vector<PseudoCommand>(const ASTNode&, std::shared_ptr<CompilationState>)>;

	class BasicSyntaxPseudoDecoder 
	{
	private:

		ExpressionDecoder ex_decoder;

		std::vector<PseudoCommand> ParseConditionBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh);	//if elif else

		std::vector<PseudoCommand> ParseOpCodeBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh);	//op_code

		std::vector<PseudoCommand> ParseWhileBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh);		//while cycle
		std::vector<PseudoCommand> ParseForBlock(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh);			//for cycle

		std::vector<PseudoCommand> ParseLoopBlock(const ASTNode& node, std::shared_ptr<CompilationState> state,recursive_handler rh);			//loop cycle

		std::vector<PseudoCommand> ParseCycles(const ASTNode& node, std::shared_ptr<CompilationState> state,recursive_handler rh);
	public:
		std::vector<PseudoCommand> HandleBasicSyntax(const ASTNode& node, std::shared_ptr<CompilationState> state, recursive_handler rh);	//Checks node header content and choouses special method for current node
	};

}
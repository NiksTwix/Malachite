#pragma once
#include "ExpressionDecoder.hpp"
namespace Malachite 
{


	class PseudoByteDecoder 
	{
	private:
		std::vector<PseudoCommand> RecursiveHandle(const ASTNode& node, std::shared_ptr<CompilationState> state);
		
		//---Basic syntax structs

		std::vector<PseudoCommand> HandleBasicSyntax(const ASTNode& node, std::shared_ptr<CompilationState> state);	//Checks node header content and choouses special method for current node

		std::vector<PseudoCommand> ParseConditionBlock(const ASTNode& node, std::shared_ptr<CompilationState> state);	//if elif else

		std::vector<PseudoCommand> ParseOpCodeBlock(const ASTNode& node, std::shared_ptr<CompilationState> state);	//op_code

		std::vector<PseudoCommand> ParseWhileBlock(const ASTNode& node, std::shared_ptr<CompilationState> state);		//while cycle
		std::vector<PseudoCommand> ParseForBlock(const ASTNode& node, std::shared_ptr<CompilationState> state);			//for cycle

		std::vector<PseudoCommand> ParseCycles(const ASTNode& node, std::shared_ptr<CompilationState> state);			//for cycle

		ExpressionDecoder ex_decoder;
	public:
		std::pair<std::shared_ptr<CompilationState>,std::vector<PseudoCommand>> GeneratePseudoCode(const ASTNode& node);
		std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> GeneratePseudoCode(const std::vector<ASTNode>& node);
	};
}


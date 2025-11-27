#pragma once
#include "ExpressionDecoder.hpp"
namespace Malachite 
{


	class PseudoByteDecoder 
	{
	private:
		std::shared_ptr<CompilationState> compilation_state = nullptr;
		std::vector<PseudoCommand> RecursiveHandle(const ASTNode& node);
		
		//---Basic syntax structs

		std::vector<PseudoCommand> HandleBasicSyntax(const ASTNode& node);	//Checks node header content and choouses special method for current node

		std::vector<PseudoCommand> ParseConditionBlock(const ASTNode& node);	//if elif else

		std::vector<PseudoCommand> ParseOpCodeBlock(const ASTNode& node);	//op_code

		std::vector<PseudoCommand> ParseWhileBlock(const ASTNode& node);		//while cycle
		std::vector<PseudoCommand> ParseForBlock(const ASTNode& node);			//for cycle

		ExpressionDecoder ex_decoder;
	public:
		std::pair<std::shared_ptr<CompilationState>,std::vector<PseudoCommand>> GeneratePseudoCode(const ASTNode& node);
		std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> GeneratePseudoCode(const std::vector<ASTNode>& node);
	};
}


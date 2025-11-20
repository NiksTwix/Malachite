#pragma once
#include "ExpressionDecoder.hpp"
namespace Malachite 
{


	class PseudoByteDecoder 
	{
	private:
		std::shared_ptr<CompilationState> compilation_state = nullptr;
		std::vector<PseudoCommand> RecursiveHandle(const ASTNode& node);
		
		ExpressionDecoder ex_decoder;
	public:
		std::pair<std::shared_ptr<CompilationState>,std::vector<PseudoCommand>> GeneratePseudoCode(const ASTNode& node);
		std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> GeneratePseudoCode(const std::vector<ASTNode>& node);
	};
}


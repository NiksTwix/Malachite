#pragma once
#include "BasicSyntaxPseudoDecoder.hpp"
namespace Malachite 
{


	class PseudoByteDecoder 
	{
	private:
		std::vector<PseudoCommand> RecursiveHandle(const ASTNode& node, std::shared_ptr<CompilationState> state);
			
		ExpressionDecoder ex_decoder;

		BasicSyntaxPseudoDecoder bs_decoder;

	public:
		std::pair<std::shared_ptr<CompilationState>,std::vector<PseudoCommand>> GeneratePseudoCode(const ASTNode& node);
		std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> GeneratePseudoCode(const std::vector<ASTNode>& node);
	};
}


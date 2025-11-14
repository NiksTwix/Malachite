#pragma once
#include "ArithmeticDecoder.hpp"
namespace Malachite 
{


	class PseudoByteDecoder 
	{
	private:
		std::shared_ptr<CompilationState> compilation_state = nullptr;
		std::vector<PseudoCommand> RecursiveHandle(const ASTNode& node);
		ExpressionDecoder ex_decoder;
	public:
		std::vector<PseudoCommand> GeneratePseudoCode(const ASTNode& node);
	};
}


#include "..\..\include\compiler\PseudoByteDecoder.hpp"



namespace Malachite 
{
	std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> PseudoByteDecoder::GeneratePseudoCode(const ASTNode& node)
	{
		compilation_state = std::make_shared<CompilationState>();

		auto result = RecursiveHandle(node);

		return std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>>(compilation_state,result);
	}
	std::vector<PseudoCommand> PseudoByteDecoder::RecursiveHandle(const ASTNode& node)
	{
		size_t children = node.children.size();
		std::vector<PseudoCommand> result;
		if (children > 0 && node.tokens.size() > 0) //If ASTNode has a header and the children its functional block 
		{
			//BasicSyntaxDecoder, returns start and end commands of block

			//Go to children

			//Push end commands
		}
		else if (children > 0 && node.tokens.size() == 0)	// visible frame without header {code....}
		{
			for (size_t i = 0; i < children; i++) 
			{
				auto r = RecursiveHandle(node.children[i]);
				result.insert(result.end(), r.begin(), r.end());
			}
		}
		else if(children == 0 && node.tokens.size() > 0)
		{
			result = ex_decoder.DecodeExpression(node,compilation_state);
		}
		return result;
	}
}
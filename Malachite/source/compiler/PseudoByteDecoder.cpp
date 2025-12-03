#include "..\..\include\compiler\PseudoByteDecoder.hpp"
#include "..\..\include\compiler\StringOperations.hpp"


namespace Malachite 
{
	std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> PseudoByteDecoder::GeneratePseudoCode(const ASTNode& node)
	{
		return GeneratePseudoCode(node.children);
	}
	std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>> PseudoByteDecoder::GeneratePseudoCode(const std::vector<ASTNode>& node)
	{
		auto compilation_state = std::make_shared<CompilationState>();
		std::vector<PseudoCommand> result;
		result.push_back(PseudoCommand(PseudoOpCode::OpenVisibleScope));
		for (auto& n : node) 
		{
			auto result1 = RecursiveHandle(n, compilation_state);
			result.insert(result.end(), result1.begin(), result1.end());
		}
		result.push_back(PseudoCommand(PseudoOpCode::CloseVisibleScope));
		return std::pair<std::shared_ptr<CompilationState>, std::vector<PseudoCommand>>(compilation_state, result);
	}
	std::vector<PseudoCommand> PseudoByteDecoder::RecursiveHandle(const ASTNode& node, std::shared_ptr<CompilationState> state)
	{
		size_t children = node.children.size();
		std::vector<PseudoCommand> result;
		if (children > 0 && node.tokens.size() > 0) //If ASTNode has a header and the children its functional block 
		{
			auto r = bs_decoder.HandleBasicSyntax(node, state, [&](const ASTNode& node, std::shared_ptr<CompilationState> state) -> std::vector<PseudoCommand> {return RecursiveHandle(node, state); });
			
			if (r.size() == 0) //Bug: Header of ast node - its common expression with children (optionality ";" had an effect) 
			{	
				auto r = ex_decoder.DecodeExpression(node, state);
				result.insert(result.end(), r.begin(), r.end());
				for (size_t i = 0; i < children; i++)
				{
					auto r = RecursiveHandle(node.children[i],state);
					result.insert(result.end(), r.begin(), r.end());
				}
			}
			else //Its real block construction (if\elif\else\while\for)
			{
				result = r;
			}
		}
		else if (children > 0 && node.tokens.size() == 0)	// visible frame without header {code....}
		{
			for (size_t i = 0; i < children; i++)
			{	
				auto r = RecursiveHandle(node.children[i],state);
				result.insert(result.end(), r.begin(), r.end());
			}
		}
		else if(children == 0 && node.tokens.size() > 0)
		{
			result = ex_decoder.DecodeExpression(node,state);
		}
		return result;
	}

}
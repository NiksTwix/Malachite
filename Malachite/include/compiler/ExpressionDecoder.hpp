#pragma once
#include "Definitions.hpp"
#include "CompilationState.hpp"
#include "ASTBuilder.hpp"
#include <memory>




namespace Malachite 
{
	enum class TokenGroupType
	{
		SIMPLE,
		COMPLEX
	};
	struct TokensGroup
	{
		std::vector<TokensGroup> tokens; //for_complex
		Token token;		//for_simple
		TokenGroupType type;
		TokensGroup()
		{
			type = TokenGroupType::COMPLEX;
		}
		TokensGroup(const Token& t)
		{
			token = t;
			type = TokenGroupType::SIMPLE;
		}
		TokensGroup(const std::vector<TokensGroup>& t)
		{
			tokens = t;
			type = TokenGroupType::COMPLEX;
		}
	};

	struct ExpressionState 
	{
		Token assign_token{};
		PseudoCommand load_assign_operation{};
		std::shared_ptr<CompilationState> state{};

		int current_line{};
		bool in_assignment_context{};
	};

	class ExpressionDecoder 
	{
	private:

		//left
		std::vector<PseudoCommand>	ProcessLeftSide(const std::vector<Token>& left, ExpressionState& es);
		std::vector<PseudoCommand>	HandleVariableDeclaration(const std::vector<Token>& left, size_t type_index, bool is_const, ExpressionState& es);
		std::vector<PseudoCommand>	HandleVariableAssignment(const Token& var_name, bool is_const, ExpressionState& es);

		//right
		std::vector<PseudoCommand>	ProcessRightSide(const std::vector<Token>& right, ExpressionState& es);
		//assign
		std::vector<PseudoCommand>	DecodeAssignExpression(const std::vector<Token>& right, const std::vector<Token>& left, ExpressionState& es);

		//Postfix
		std::vector<PseudoCommand>	PTP_HandleFunctionCall(const std::vector<TokensGroup>& postfix, ExpressionState& es);	//PTP-PostfixToPseude (Help method)
		std::vector<PseudoCommand>	PTP_HandleExpression(const std::vector<TokensGroup>& postfix, ExpressionState& es);	 //PTP-PostfixToPseude (Help method)
	public:
		std::vector<TokensGroup>	ToPostfixForm(const std::vector<Token>& original);
		std::vector<PseudoCommand>	PostfixToPseudo(const std::vector<TokensGroup>& postfix, ExpressionState& es);
		std::vector<PseudoCommand>	DecodeExpression(const std::vector<Token>& tokens, std::shared_ptr<CompilationState> state);
		std::vector<PseudoCommand>	DecodeExpression(const ASTNode& node, std::shared_ptr<CompilationState> state);

	};
}
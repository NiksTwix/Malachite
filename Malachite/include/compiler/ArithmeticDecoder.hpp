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
	class ExpressionDecoder 
	{
	private:

		Type* FindType(std::shared_ptr<CompilationState> state, const std::string& type_name);
		Variable* FindVariable(std::shared_ptr<CompilationState> state, const std::string& variable_name);
		const std::vector<functionID>* FindFunctions(std::shared_ptr<CompilationState> state, const std::string& function_name);
		
		std::vector<PseudoCommand>	ProcessLeftSide(const std::vector<Token>& left,std::shared_ptr<CompilationState> state);
		std::vector<PseudoCommand>	HandleVariableDeclaration(const std::vector<Token>& left, size_t type_index, bool is_const, std::shared_ptr<CompilationState> state);
		std::vector<PseudoCommand>	HandleVariableAssignment(const Token& var_name, bool is_const, std::shared_ptr<CompilationState> state);


		std::vector<PseudoCommand>	ProcessRightSide(const std::vector<Token>& right, std::shared_ptr<CompilationState> state);
		std::vector<PseudoCommand>	DecodeAssignExpression(const std::vector<Token>& right, const std::vector<Token>& left, std::shared_ptr<CompilationState> state);

		//Postfix
		std::vector<PseudoCommand>	PTP_HandleFunctionCall(const std::vector<TokensGroup>& postfix, std::shared_ptr<CompilationState> state);	//PTP-PostfixToPseude (Help method)
		std::vector<PseudoCommand>	PTP_HandleExpression(const std::vector<TokensGroup>& postfix, std::shared_ptr<CompilationState> state);	 //PTP-PostfixToPseude (Help method)
	public:
		std::vector<TokensGroup>	ToPostfixForm(const std::vector<Token>& original);
		std::vector<PseudoCommand>	PostfixToPseudo(const std::vector<TokensGroup>& postfix, std::shared_ptr<CompilationState> state);


		std::vector<PseudoCommand>	DecodeExpression(const ASTNode& node, std::shared_ptr<CompilationState> state);
		
	};
}
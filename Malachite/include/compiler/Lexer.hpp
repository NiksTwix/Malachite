#pragma once
#include "Definitions.hpp"
#include "StringOperations.hpp"
#include <unordered_set>

namespace Malachite 
{
	class Lexer 
	{
	private:
		std::string undefined_token = std::string();
		int current_line = 0;
		int current_depth = 0;
		bool was_comment = false;
		bool is_negative_value = false; // -10

		///--------Methods
		char GetNearCharWithoutSpaces(const std::string& text, size_t start_index, int step = 1);

		TokenType GetTokenType(const std::string& token);

		TokenValue GetTokenValue(const std::string& token);

		Token CreateToken(std::string operand, int line);
		char ProcessCharLiteral(const std::string& str);
		std::string ProcessStringLiteral(const std::string& str);

		Token ProcessOperator(const std::string& text, size_t& index, std::vector<Token>& tokens);

		Token InsertOpEnd(std::vector<Token>& tokens, int current_index, const std::string& text);
	public:
		std::vector<Token> ToTokens(std::string text);
	};

}
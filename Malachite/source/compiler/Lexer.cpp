#include "..\..\include\compiler\Lexer.hpp"
#include "..\..\include\compiler\Logger.hpp"


namespace Malachite
{
	char Lexer::GetNearCharWithoutSpaces(const std::string& text, size_t start_index, int step)

	{
		std::unordered_set<char> chars = { ' ','\t','\r','\n' };
		int i = start_index + step;
		while (i >= 0 && i < text.size())
		{
			if (!chars.count(text[i])) return text[i];
			i += step;
		}

		return '\0';
	}
	TokenType Lexer::GetTokenType(const std::string& token) //Простой и быстрый способ для получения токенов
	{
		auto type = SyntaxInfo::GetTokenType(token);
		if (type != TokenType::UNDEFINED)
		{
			return type;
		}

		if (StringOperations::IsNumber(token)) return TokenType::LITERAL;

		if (token.size() >= 2 && token[0] == '"' && token[token.size() - 1] == '"') return TokenType::LITERAL;
		if (token.size() >= 2 && token[0] == '\'' && token[token.size() - 1] == '\'') return TokenType::LITERAL;
		return TokenType::IDENTIFIER;
	}
	TokenValue Lexer::GetTokenValue(const std::string& token)		//Транслирует в строки/цифры и тд
	{
		if (token == "true") return true;
		else if (token == "false") return false;

		if (StringOperations::IsNumber(token))	//TODO type management
		{
			if (is_negative_value)
			{
				is_negative_value = false;
				return -std::stod(token);
			}
			return std::stod(token);
		}
		if (token.size() >= 2 && token[0] == '\'' && token.back() == '\'') {
			return ProcessCharLiteral(token.substr(1, token.size() - 2));
		}
		if (token.size() >= 2 && token[0] == '"' && token.back() == '"') {
			return ProcessStringLiteral(token.substr(1, token.size() - 2));
		}
		return token;
	}
	Token Lexer::CreateToken(std::string operand, int line)
	{
		Token t;
		t.line = line;

		//Проверка на ключевое слово в будущем

		t.type = GetTokenType(operand);
		t.value = GetTokenValue(operand);
		t.depth = current_depth;
		return t;
	}
	char Lexer::ProcessCharLiteral(const std::string& str)
	{
		int size = str.size();
		if (size > 2 || size == 0)//Not escape and not char		//TODO Create Compiler Errors
		{
			Logger::Get().PrintSyntaxError("Invalid char.", current_line);	
			return '\0';
		}
		if (size == 1) return str[0];
		if (size == 2 && str[0] == '\\')	//Escape
		{
			switch (str[1]) {
			case 'n':return '\n';
			case 't':return '\t';
			case '"':return '"';
			case '\'': return '\'';
			case '\\': return'\\';
			}
		}
		Logger::Get().PrintSyntaxError("Invalid char.", current_line);
		return '\0';
	}
	std::string Lexer::ProcessStringLiteral(const std::string& str)
	{
		std::string result;
		result.reserve(str.size()); // Оптимизация памяти

		for (size_t i = 0; i < str.size(); ++i) {
			if (str[i] == '\\' && i + 1 < str.size()) {
				switch (str[i + 1]) {
				case 'n': result += '\n'; break;
				case 't': result += '\t'; break;
				case '"': result += '"'; break;  // Это нужно!
				case '\\': result += '\\'; break;
				default:
					// Неизвестная escape-последовательность - оставляем как есть
					result += str[i];
					continue; // не пропускаем следующий символ
				}
				i++; // Пропускаем обработанный символ
			}
			else {
				result += str[i];
			}
		}
		return result;
	}
	Token Lexer::ProcessOperator(const std::string& text, size_t& index, std::vector<Token>& tokens)
	{
		//Multichar operators: <<= and etc

		std::string operator_;
		bool is_unary = (tokens.size() == 0) || (tokens.back().type != TokenType::IDENTIFIER && tokens.back().type != TokenType::LITERAL && tokens.back().value.strVal != ")");
		for (; index < text.size(); index++)
		{
			char c = text[index];
			if (GetTokenType(std::string(1, c)) == TokenType::OPERATOR)	operator_.push_back(c);
			else break;
		}
		if (is_unary) operator_.push_back('u');
		return Token(SyntaxInfo::GetTokenType(operator_),operator_,current_line,current_depth);
	}
	Token Lexer::InsertOpEnd(std::vector<Token>& tokens, int current_index, const std::string& text)
	{
		if (tokens.size() == 0)return Token(TokenType::UNDEFINED);
		if (text[current_index] == ';') return Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPERATION_END, current_line, current_depth);

		if (tokens.back().type == TokenType::COMPILATION_LABEL || tokens.back().value.strVal == "{" && undefined_token.empty()) return Token(TokenType::UNDEFINED);

		if (tokens.back().type != TokenType::COMPILATION_LABEL && tokens.back().value.strVal != "}" && text[current_index] == '}')
		{
			return Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPERATION_END, current_line, current_depth);
		}

		if (text[current_index] == '\n' && !(GetNearCharWithoutSpaces(text, current_index, 1) == '{' || GetNearCharWithoutSpaces(text, current_index, -1) == '}'))
		{
			return Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPERATION_END, current_line, current_depth);
		}

		return Token(TokenType::UNDEFINED);
	}
	std::vector<Token> Lexer::ToTokens(std::string text)
	{
		std::vector<Token> result;
		current_line = 1;
		current_depth = 0;
		was_comment = false;
		undefined_token = std::string();
		is_negative_value = false;
		bool in_string = false;
		for (size_t i = 0; i < text.size(); i++)
		{
			char c = text[i];
			if (c == '\r') continue;
			if (c == '\n')
			{
				current_line++;
			}
			// Simple line comments
			if (!in_string)
			{
				if (c == '/' && i + 1 < text.size() && text[i + 1] == '/') {
					while (i < text.size() && text[i] != '\n')
					{
						i++;
					}
					was_comment = true;
					i--;	//чтобы вернуть \n
					continue;
				}
				//Multilines comments
				if (c == '/' && i + 1 < text.size() && text[i + 1] == '*') {
					char prev = text[i];
					i++;
					while (i < text.size())
					{
						if (prev == '*' && text[i] == '/')
						{
							was_comment = true;
							break;
						}
						prev = text[i];
						i++;
					}
					if (!was_comment) Logger::Get().PrintSyntaxError("Excepts \"*/\".", current_line);
					continue;
				}

				if ((c == ' ' || c == '\n' || c == '\t'))
				{
					if (!undefined_token.empty())
					{
						result.push_back(CreateToken(undefined_token, current_line));
					}
					undefined_token.clear();

				}

				auto t = InsertOpEnd(result, i, text);
				if (t.type != TokenType::UNDEFINED)
				{
					if (!undefined_token.empty())
					{
						result.push_back(CreateToken(undefined_token, current_line));
						undefined_token.clear();
					}
					result.push_back(t);
					if (c == ';')continue;
				}
				if (was_comment) was_comment = false;


				if (GetTokenType(std::string(1, c)) == TokenType::OPERATOR)
				{
					std::string operator_ = std::string({ c });
					if (!undefined_token.empty())	//Сразу добавляем токен перед ним, если есть
					{
						result.push_back(CreateToken(undefined_token, current_line));
						undefined_token.clear();
					}

					if (i < text.size() - 1 && GetTokenType(std::string({ c, text[i + 1] })) == TokenType::LITERAL && (result.size() == 0 || (result.back().type != TokenType::LITERAL && result.back().type != TokenType::IDENTIFIER))) //если число
					{
						is_negative_value = true;
						continue;
					}
					Token t = ProcessOperator(text, i, result);
					i--;
					if (t.type == TokenType::UNDEFINED)
					{
						Logger::Get().PrintSyntaxError("Invalid operator \"" + t.value.strVal + "\"", current_line);
						continue;
					}
					result.push_back(t);
					continue;
				}


				if (GetTokenType(std::string(1, c)) == TokenType::DELIMITER)
				{
					std::string delimiter = std::string({ c });
					if (c == '.' && StringOperations::IsNumber(undefined_token)) 
					{
						undefined_token.push_back(c);
						continue;
					}
					if (!undefined_token.empty())	//Сразу добавляем токен перед ним, если есть
					{
						result.push_back(CreateToken(undefined_token, current_line));
						undefined_token.clear();
					}
					Token t;
					t.line = current_line;
					t.type = TokenType::DELIMITER;
					t.value = delimiter;
					t.depth = current_depth;
					if (c == '{') current_depth++;
					if (c == '}')
					{
						current_depth--;
						t.depth = current_depth;
					}

					result.push_back(t);
					continue;
				}
			}


			if (c == '\"' && undefined_token.empty())
			{
				in_string = true;
				undefined_token.push_back(c);
				continue;
			}
			if (c == '\"' && !undefined_token.empty())
			{
				in_string = false;
				undefined_token.push_back(c);
				result.push_back(CreateToken(undefined_token, current_line));
				undefined_token.clear();
				continue;
			}
			if ((c == ' ' || c == '\n' || c == '\t') && !in_string) continue;
			undefined_token.push_back(c);
		}

		if (!undefined_token.empty())
		{
			Token t;
			t.line = current_line;
			t.depth = current_depth;
			t.type = GetTokenType(undefined_token);
			t.value = GetTokenValue(undefined_token);
			result.push_back(t);
			result.push_back(Token(TokenType::COMPILATION_LABEL, (uint64_t)CompilationLabel::OPERATION_END, current_line, current_depth));
		}

		return result;
	}
}
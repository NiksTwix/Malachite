#pragma once
#include <unordered_map>
#include <string>

namespace Malachite 
{
	enum class TokenValueType 
	{
		VOID = 0,
		INT,
        UINT,
		BOOL,
		FLOAT,
		STRING,
		CHAR
	};

    enum class CompilationLabel : uint8_t
    {
        OPERATION_END = 0,  //End of operation
        SCOPE_START,    //Start of variable scope
        SCOPE_END,      //End of variable scope
        FUNCTION_CALL,  //function(params)
        OFFSET_ACCESS,  //array[index]

        FIELD_ACCESS,   //For class.field
        METHOD_CALL,     //For class.method(params)
    };


    enum class TokenType {
        UNDEFINED = 0,
        IDENTIFIER,// Переменные, ключевые слова
        OPERATOR,  // +, -, =, *, /, >, <
        LITERAL,   // Числа, строки, true/false
        DELIMITER,  //{},[],,() и тд
        COMPILATION_LABEL, //Токен-метка компилятора
        TYPE_MARKER,
        KEYWORD
    };

    struct TokenValue {
        TokenValueType type;

        union {
            double floatVal;    //Float in Malachite is double 
            bool boolVal;
            int64_t intVal;
            uint64_t uintVal;
            char charVal;
        };
        std::string strVal;   // Для строк 
        // Конструкторы
        TokenValue() : type(TokenValueType::VOID), floatVal(0) {}

        TokenValue(double v) : type(TokenValueType::FLOAT), floatVal(v) {}
        TokenValue(int64_t v) : type(TokenValueType::INT), intVal(v) {}
        TokenValue(uint64_t v) : type(TokenValueType::UINT), uintVal(v) {}
        TokenValue(bool v) : type(TokenValueType::BOOL), boolVal(v) {}
        TokenValue(char v) : type(TokenValueType::CHAR), charVal(v) {}
        TokenValue(const std::string& s) : type(TokenValueType::STRING) {
            new (&strVal) std::string(s);
        }

        TokenValue(const char* s) : type(TokenValueType::STRING) {
            new (&strVal) std::string(s);
        }

        // ПРАВИЛО ПЯТИ (очень важно!)
        // Конструктор копирования
        TokenValue(const TokenValue& other) : type(other.type), uintVal(other.uintVal) {
            if (type == TokenValueType::STRING) new (&strVal) std::string(other.strVal);
        }

        // Оператор присваивания
        TokenValue& operator=(const TokenValue& other) {
            // Если текущий объект уже хранит строку, её нужно уничтожить
            if (this == &other) return *this; // Защита от самоприсваивания
            destroyCurrent();
            uintVal = other.uintVal;
            type = other.type;

            if (type == TokenValueType::STRING) new (&strVal) std::string(other.strVal);
            return *this;
        }
        TokenValue(TokenValue&& other) noexcept : type(other.type), uintVal(other.uintVal) {
            if (type == TokenValueType::STRING) new (&strVal) std::string(std::move(other.strVal));
            other.type = TokenValueType::VOID; // Чтобы деструктор other ничего не делал
        }

        // Оператор перемещающего присваивания
        TokenValue& operator=(TokenValue&& other) noexcept {
            if (this != &other) {
                destroyCurrent(); // Уничтожаем текущие данные
                type = other.type;
                uintVal = other.uintVal;
                if (type == TokenValueType::STRING) new (&strVal) std::string(std::move(other.strVal));
                other.type = TokenValueType::VOID;
            }
            return *this;
        }

        std::string ToString() const
        {
            switch (type)
            {
            case Malachite::TokenValueType::VOID:
                return "";
                break;
            case Malachite::TokenValueType::INT:
                return std::to_string(intVal);
                break;
            case Malachite::TokenValueType::UINT:
                return std::to_string(uintVal);
                break;
            case Malachite::TokenValueType::BOOL:
                return boolVal ? "true": "false";
                break;
            case Malachite::TokenValueType::FLOAT:
                return std::to_string(floatVal);
                break;
            case Malachite::TokenValueType::STRING:
                return strVal;
                break;
            case Malachite::TokenValueType::CHAR:
                return std::to_string(charVal);
                break;
            default:
                return "";
                break;
            }
        }
        static std::string GetTypeString(TokenValueType type)
        {
            switch (type)
            {
            case Malachite::TokenValueType::VOID:
                return "void";
                break;
            case Malachite::TokenValueType::INT:
                return "int";
                break;
            case Malachite::TokenValueType::UINT:
                return"uint";
                break;
            case Malachite::TokenValueType::BOOL:
                return "bool";
                break;
            case Malachite::TokenValueType::FLOAT:
                return "float";
                break;
            case Malachite::TokenValueType::STRING:
                return "string";
                break;
            case Malachite::TokenValueType::CHAR:
                return "char";
                break;
            default:
                return "undefined type";
                break;
            }
        }
        // Деструктор
        ~TokenValue() {
            destroyCurrent();
        }

    private:
        void destroyCurrent() {
            // Явно вызываем деструктор только для строки
            if (type == TokenValueType::STRING) {
                strVal.~basic_string();
            }
            // Для shared_ptr и примитивов деструктор вызывать не нужно
        }
    };



	struct Token 
	{
		TokenType type;
		TokenValue value;
		size_t line; // Для ошибок
		size_t depth;  // Уровень вложенности
	};

    //Pseudo byte code


    enum class PseudoOpCode : uint8_t
    {
        //Stack principe: lower value is left, higher is right
        Nop,
        Immediate,          // Push literal contant
        Push,               // Push to stack (for functions)
        Pop,
        Load,               //Load "variable"/pointer
        Store,              //Store "variable"/pointer
        LoadOffset,         //Load      "variable"-startpointer     "variable"/constant-offset     
        StoreOffset,        //Store    "variable"-startpointer     "variable"/constant-offset

        //For array and OOP
        LoadDirect,         //Load from direct address in memory
        StoreDirect,        //Store in direct address in memory
        GetAddress,         //Returns address in heap or stack

        ScopeStart,
        ScopeEnd,

        Label,      //label label_Type label_name

        DeclareVariable,    //Declaring of variable, parameters are name and vm_type (double/int64/uint64), but creating writting in variable table with fact type (string and another)
        DeclareFunction,    //Declaring of function, parameters are name and return type_id, code after DeclareFunction and ScopeStart is function body.
        //Arithmetic
        Add,
        Subtract,
        Multiplication,
        Division,
        Mod,
        Negative,

        // Logic
        And,
        Or,
        Not,
        Compare,
        Abode,
        Less,
        BitOr,
        BitNot,
        BitAnd,
        BitOffsetLeft,
        BitOffsetRight,

        //Control flow
        Jump,
        JumpIf,
        JumpNotIf,
        Call,
        Return
        //System calls ->  welcome to op_code {...}

        //OOP
        // Creates object -> calls constructor
    };

    struct PseudoCommand
    {
        PseudoOpCode op_code;
        std::unordered_map<std::string, TokenValue> parameters; //Example,  op_code: DeclareVariable, parameters: name-"x"
    };


    class SyntaxInfo 
    {
    private:
        static std::unordered_map<std::string, TokenType>& GetTokensMap() 
        {
            static std::unordered_map<std::string, TokenType> tokensMap = {
				{"+", TokenType::OPERATOR},
				{"-", TokenType::OPERATOR},
                {"+u", TokenType::OPERATOR},    //u-unary
                {"-u", TokenType::OPERATOR},
				{"/", TokenType::OPERATOR},
				{"*", TokenType::OPERATOR},
				{"%", TokenType::OPERATOR},
				{"=", TokenType::OPERATOR},
				{"+=", TokenType::OPERATOR},
				{"-=", TokenType::OPERATOR},
				{"/=", TokenType::OPERATOR},
				{"*=", TokenType::OPERATOR},
				{"==", TokenType::OPERATOR},
				{"!=", TokenType::OPERATOR},
				{"!", TokenType::OPERATOR},
				{">",TokenType::OPERATOR},
				{"<",TokenType::OPERATOR},
				{">=",TokenType::OPERATOR},
				{"<=",TokenType::OPERATOR},
				{"->",TokenType::OPERATOR},     //Operator of returned function's type or implication (in the future)
				{">>",TokenType::OPERATOR},
				{"<<",TokenType::OPERATOR},
				{">>=",TokenType::OPERATOR},
				{"<<=",TokenType::OPERATOR},
				{"&&",TokenType::OPERATOR},
				{"||",TokenType::OPERATOR},
                {"&&=",TokenType::OPERATOR},
                {"||=",TokenType::OPERATOR},
                {"&",TokenType::OPERATOR},
                {"~",TokenType::OPERATOR},
                {"|",TokenType::OPERATOR},
                {"&=",TokenType::OPERATOR},
                {"|=",TokenType::OPERATOR},
                {"~=",TokenType::OPERATOR},
				{"false", TokenType::LITERAL},
				{"true", TokenType::LITERAL},
				{"(", TokenType::DELIMITER},
				{")", TokenType::DELIMITER},
				{"{", TokenType::DELIMITER},
				{"}", TokenType::DELIMITER},
				{"[", TokenType::DELIMITER},
				{"]", TokenType::DELIMITER},
				{":", TokenType::DELIMITER},    //delimiter for inheritance
				{",", TokenType::DELIMITER},
                {".", TokenType::DELIMITER},
				{";",TokenType::DELIMITER},
				{"int", TokenType::TYPE_MARKER},		//int64
                {"bool", TokenType::TYPE_MARKER},
                {"char", TokenType::TYPE_MARKER},
                {"uint", TokenType::TYPE_MARKER},
				{"float", TokenType::TYPE_MARKER},		//double
				{"void", TokenType::TYPE_MARKER},
				{"if", TokenType::KEYWORD},
				{"elif", TokenType::KEYWORD},
				{"else", TokenType::KEYWORD},
				{"while", TokenType::KEYWORD},
				{"for", TokenType::KEYWORD},
				{"continue", TokenType::KEYWORD},
				{"break", TokenType::KEYWORD},
				{"func", TokenType::KEYWORD},
				{"return", TokenType::KEYWORD},
				{"op_code",TokenType::KEYWORD},
				{"import",TokenType::KEYWORD},
				{"meta",TokenType::KEYWORD},
				{"class",TokenType::KEYWORD},
				{"alias",TokenType::KEYWORD},
				{"new",TokenType::KEYWORD},
                {"delete",TokenType::KEYWORD},
                {"namespace",TokenType::KEYWORD},
                {"const",TokenType::KEYWORD},
            };
            return tokensMap;
        }
        static std::unordered_map<std::string, int>& GetTokensOperationsPriorityMap()
        {
            static std::unordered_map<std::string, int> tokensMap = {
                // Уровень 9: скобки (обрабатываются отдельно)
                {"(", -1}, {")", -1},  // special cases

                // Уровень 8: унарные операторы
                {"!", 8}, {"~", 8}, {"+u", 8}, {"-u", 8},  // унарные + и -, а также битовое НЕ u-unary

                // Уровень 7: мультипликативные
                {"*", 7}, {"/", 7}, {"%", 7},

                // Уровень 6: аддитивные 
                {"+", 6}, {"-", 6},            // бинарные + и -

                // Уровень 5: битовые сдвиги
                {"<<", 5}, {">>", 5},

                // Уровень 4: сравнения
                {"<", 4}, {"<=", 4}, {">", 4}, {">=", 4},

                // Уровень 3: равенства
                {"==", 3}, {"!=", 3},

                // Уровень 2: битовые И и Not
                {"&", 2},

                // Уровень 1: битовые ИЛИ/XOR
                {"|", 1},

                // Уровень 0: логические И/ИЛИ
                {"&&", 0}, {"||", 0},

                // Уровень -1: присваивания (самый низкий приоритет)
                {"=", -1}, {"+=", -1}, {"-=", -1}, {"*=", -1},
                {"/=", -1}, {"%=", -1}, {"&=", -1}, {"|=", -1},
                {"<<=", -1}, {">>=", -1}, {"&&=", -1}, {"||=", -1}, 
                {"~=", -1}
            };
            return tokensMap;
        }

        static std::unordered_map<std::string, PseudoOpCode>& GetOperatorsPseudoMap() 
        {
            static std::unordered_map<std::string, PseudoOpCode> tokensMap =
            {
                {"+",PseudoOpCode::Add},
                {"-",PseudoOpCode::Subtract},
                {"*",PseudoOpCode::Multiplication},
                {"/",PseudoOpCode::Division},
                {"%",PseudoOpCode::Mod},
                {"-u",PseudoOpCode::Negative},
                {"&&",PseudoOpCode::And},
                {"||",PseudoOpCode::Or},
                {"!",PseudoOpCode::Not},
                {"==",PseudoOpCode::Compare},
                {"|",PseudoOpCode::BitOr},
                {"~",PseudoOpCode::BitNot},
                {"&",PseudoOpCode::BitAnd},
                {"<<",PseudoOpCode::BitOffsetLeft},
                {">>",PseudoOpCode::BitOffsetRight},
                {">",PseudoOpCode::Abode},
                {"<",PseudoOpCode::Less},
                //{"->",PseudoOpCode::Implication},
            };
            return tokensMap;
        }

    public:
        static TokenType GetTokenType(const std::string& string_token) 
        {
            auto map = GetTokensMap();
            return map.count(string_token) ? map[string_token]: TokenType::UNDEFINED;
        }
        static int GetOperationPriority(const Token& token) 
        {
            auto map = GetTokensOperationsPriorityMap();
            return map.count(token.value.strVal) ? map[token.value.strVal] : -1;
        }
        static PseudoOpCode GetOperatorPseudoCode(const Token& token)
        {
            auto map = GetOperatorsPseudoMap();
            return map.count(token.value.strVal) ? map[token.value.strVal] : PseudoOpCode::Nop;
        }
    };

    
    
}
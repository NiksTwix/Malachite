#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "..\core\operations.h"

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

        NODES_GROUP,
        EXCEPT_HANDLING,    //Command is except handling 

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
        ExceptHandling, //Compilation command
        START_SECTION_MEMORY_OPS,

        Immediate,          // Push literal contant
        Push,               // Push to stack (for functions)
        Pop,
        Load,               //Load "variable"/pointer
        Store,              //Store "variable"/pointer
        LoadRelatively,               //Load "variable"/pointer with offset in stack (if and cycles in functions)
        StoreRelatively,              //Store "variable"/pointer with offset in stack (if and cycles in functions)
        LoadOffset,         //Load      "variable"-startpointer     "variable"/constant-offset     
        StoreOffset,        //Store    "variable"-startpointer     "variable"/constant-offset

        //For array and OOP
        LoadDirect,         //Load from direct address in memory
        StoreDirect,        //Store in direct address in memory
        GetAddress,         //Returns address in heap or stack
        END_SECTION_MEMORY_OPS,


        //Debug
        ScopeStart,
        ScopeEnd,

        //Declaring
        START_SECTION_DECLARING_OPS,
        DeclareVariable,    //Declaring of variable, parameters are name and vm_type (double/int64/uint64), but creating writting in variable table with fact type (string and another)
        DeclareFunction,    //Declaring of function, parameters are name and return type_id, code after DeclareFunction and ScopeStart is function body.
        DeclareFunctionEnd,
        END_SECTION_DECLARING_OPS,
        //Arithmetic
        START_SECTION_ARITHMETIC_OPS,
        Add,
        Subtract,
        Multiplication,
        Division,
        Mod,
        Negative,
        END_SECTION_ARITHMETIC_OPS,
        // Logic
        START_SECTION_LOGIC_OPS,
        And,
        Or,
        Not,
        Equal,
        NotEqual,
        Greater,
        Less,
        GreaterEqual,
        LessEqual,
        BitOr,
        BitNot,
        BitAnd,
        BitOffsetLeft,
        BitOffsetRight,
        END_SECTION_LOGIC_OPS,
        //Control flow
        START_SECTION_CONTROL_FLOW_OPS,
        Label,      //label label_Type label_name
        Jump,
        JumpIf,
        JumpNotIf,
        Call,
        Return,
        END_SECTION_CONTROL_FLOW_OPS,

        OpCodeStart,
        OpCodeAllocateBasicRegisters,    //RA-RH, for byte decoder!
        OpCodeCommand,     //In parameters: OP_CODE, destination, source0,source1
        OpCodeStoreVR,
        OpCodeLoadRV,
        OpCodeEnd,

        //System calls ->  welcome to op_code {...}

        //OOP
        // Creates object -> calls constructor
    };

    struct PseudoCommand
    {
        PseudoOpCode op_code{};
        std::unordered_map<std::string, TokenValue> parameters{}; //Example,  op_code: DeclareVariable, parameters: name-"x"
    };

    struct SyntaxInfoKeywords //If want custom keywords on russian - use koi-8
    {
        const std::string keyword_if        = "if";
        const std::string keyword_elif      = "elif";
        const std::string keyword_else      = "else";
        const std::string keyword_while     = "while";
        const std::string keyword_for       = "for";
        const std::string keyword_for_math  = "forint";
        const std::string keyword_loop      = "loop";
        const std::string keyword_continue  = "continue";
        const std::string keyword_break     = "break";
        const std::string keyword_func      = "func";
        const std::string keyword_return    = "return";
        const std::string keyword_op_code   = "op_code";
        const std::string keyword_import    = "import";
        const std::string keyword_meta      = "meta";
        const std::string keyword_class     = "class";
        const std::string keyword_alias     = "alias";
        const std::string keyword_new       = "new";
        const std::string keyword_delete    = "delete";
        const std::string keyword_namespace = "namespace";
        const std::string keyword_const     = "const";

        const std::string typemarker_int    = "int";
        const std::string typemarker_uint   = "uint";
        const std::string typemarker_float  = "float";
        const std::string typemarker_bool   = "bool";
        const std::string typemarker_char   = "char";
        const std::string typemarker_void   = "void";

        const std::string literal_true      = "true";
        const std::string literal_false     = "false";

        static SyntaxInfoKeywords& Get() 
        {
            static SyntaxInfoKeywords keywords;
            return keywords;
        }

    }; 

    class SyntaxInfo 
    {
    private:
        static std::unordered_map<std::string, TokenType>& GetTokensMap() 
        {
            static std::unordered_map<std::string, TokenType> tokensMap = {
				{"+", TokenType::OPERATOR},{"-", TokenType::OPERATOR},
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
                {"!", TokenType::OPERATOR},     // 
				{"!u", TokenType::OPERATOR},    //  Its result of lexer
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
				{SyntaxInfoKeywords::Get().literal_true, TokenType::LITERAL},
				{SyntaxInfoKeywords::Get().literal_false, TokenType::LITERAL},
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
				{SyntaxInfoKeywords::Get().typemarker_int, TokenType::TYPE_MARKER},		//int64
                {SyntaxInfoKeywords::Get().typemarker_uint, TokenType::TYPE_MARKER},    //uint64
                {SyntaxInfoKeywords::Get().typemarker_float, TokenType::TYPE_MARKER},   //double
                {SyntaxInfoKeywords::Get().typemarker_char, TokenType::TYPE_MARKER},
				{SyntaxInfoKeywords::Get().typemarker_bool, TokenType::TYPE_MARKER},		
				{SyntaxInfoKeywords::Get().typemarker_void, TokenType::TYPE_MARKER},
				{SyntaxInfoKeywords::Get().keyword_if, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_elif, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_else, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_while, TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_for, TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_loop, TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_for_math, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_continue, TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_break, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_func, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_return, TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_op_code,TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_import,TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_meta,TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_class,TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_alias,TokenType::KEYWORD},
				{SyntaxInfoKeywords::Get().keyword_new,TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_delete,TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_namespace,TokenType::KEYWORD},
                {SyntaxInfoKeywords::Get().keyword_const,TokenType::KEYWORD},
            };
            return tokensMap;
        }
        static std::unordered_map<std::string, int>& GetTokensOperationsPriorityMap()
        {
            static std::unordered_map<std::string, int> tokensMap = {
                // Уровень 9: скобки (обрабатываются отдельно)
                {"(", -1}, {")", -1},  // special cases

                // Уровень 8: унарные операторы
                {"!u", 8}, {"~", 8}, {"+u", 8}, {"-u", 8},  // унарные + и -, а также битовое НЕ u-unary

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
                {"!u",PseudoOpCode::Not},
                {"==",PseudoOpCode::Equal},
                {"!=",PseudoOpCode::NotEqual},
                {"|",PseudoOpCode::BitOr},
                {"~",PseudoOpCode::BitNot},
                {"&",PseudoOpCode::BitAnd},
                {"<<",PseudoOpCode::BitOffsetLeft},
                {">>",PseudoOpCode::BitOffsetRight},
                {">",PseudoOpCode::Greater},
                {"<",PseudoOpCode::Less},
                {">=",PseudoOpCode::GreaterEqual},
                {"<=",PseudoOpCode::LessEqual},
                //{"->",PseudoOpCode::Implication},
            };
            return tokensMap;
        }

        static std::unordered_map<PseudoOpCode, std::string>& GetStringPseudoMap() 
        {
            static std::unordered_map<PseudoOpCode, std::string> PseudoOpCodeToString = {
                {PseudoOpCode::Nop, "Nop"},
                {PseudoOpCode::Immediate, "Immediate"},
                {PseudoOpCode::Push, "Push"},
                {PseudoOpCode::Pop, "Pop"},
                {PseudoOpCode::Load, "Load"},
                {PseudoOpCode::Store, "Store"},
                {PseudoOpCode::LoadOffset, "LoadOffset"},
                {PseudoOpCode::StoreOffset, "StoreOffset"},
                {PseudoOpCode::LoadDirect, "LoadDirect"},
                {PseudoOpCode::StoreDirect, "StoreDirect"},
                {PseudoOpCode::GetAddress, "GetAddress"},
                {PseudoOpCode::ScopeStart, "ScopeStart"},
                {PseudoOpCode::ScopeEnd, "ScopeEnd"},
                {PseudoOpCode::Label, "Label"},
                {PseudoOpCode::DeclareVariable, "DeclareVariable"},
                {PseudoOpCode::DeclareFunction, "DeclareFunction"},
                {PseudoOpCode::Add, "Add"},
                {PseudoOpCode::Subtract, "Subtract"},
                {PseudoOpCode::Multiplication, "Multiplication"},
                {PseudoOpCode::Division, "Division"},
                {PseudoOpCode::Mod, "Mod"},
                {PseudoOpCode::Negative, "Negative"},
                {PseudoOpCode::And, "And"},
                {PseudoOpCode::Or, "Or"},
                {PseudoOpCode::Not, "Not"},
                {PseudoOpCode::Equal, "Equal"},
                {PseudoOpCode::NotEqual, "NotEqual"},
                {PseudoOpCode::Greater, "Greater"},
                {PseudoOpCode::Less, "Less"},
                {PseudoOpCode::GreaterEqual, "GreaterEqual"},
                {PseudoOpCode::LessEqual, "LessEqual"},
                {PseudoOpCode::BitOr, "BitOr"},
                {PseudoOpCode::BitNot, "BitNot"},
                {PseudoOpCode::BitAnd, "BitAnd"},
                {PseudoOpCode::BitOffsetLeft, "BitOffsetLeft"},
                {PseudoOpCode::BitOffsetRight, "BitOffsetRight"},
                {PseudoOpCode::Jump, "Jump"},
                {PseudoOpCode::JumpIf, "JumpIf"},
                {PseudoOpCode::JumpNotIf, "JumpNotIf"},
                {PseudoOpCode::Call, "Call"},
                {PseudoOpCode::Return, "Return"},
                {PseudoOpCode::OpCodeStart, "OpCodeStart"},
                {PseudoOpCode::OpCodeEnd, "OpCodeEnd"},
                {PseudoOpCode::OpCodeStoreVR, "OpCodeStoreVR"},
                {PseudoOpCode::OpCodeLoadRV, "OpCodeLoadRV"},
                {PseudoOpCode::OpCodeCommand, "OpCodeCommand"},
                {PseudoOpCode::OpCodeAllocateBasicRegisters, "OpCodeAllocateBasicRegisters"},
            };

            return PseudoOpCodeToString;
        }

        static std::unordered_map<MalachiteCore::OpCode, std::string>& GetOpCodeStrMap() 
        {
            static std::unordered_map<MalachiteCore::OpCode, std::string> OpCodeToString = {
                {MalachiteCore::OP_NOP, "OP_NOP"},

                // Arithmetic [1-30]
                {MalachiteCore::OP_IADD_RRR, "OP_IADD_RRR"},
                {MalachiteCore::OP_ISUB_RRR, "OP_ISUB_RRR"},
                {MalachiteCore::OP_IMUL_RRR, "OP_IMUL_RRR"},
                {MalachiteCore::OP_IDIV_RRR, "OP_IDIV_RRR"},
                {MalachiteCore::OP_IMOD_RRR, "OP_IMOD_RRR"},
                {MalachiteCore::OP_UADD_RRR, "OP_UADD_RRR"},
                {MalachiteCore::OP_USUB_RRR, "OP_USUB_RRR"},
                {MalachiteCore::OP_UMUL_RRR, "OP_UMUL_RRR"},
                {MalachiteCore::OP_UDIV_RRR, "OP_UDIV_RRR"},
                {MalachiteCore::OP_UMOD_RRR, "OP_UMOD_RRR"},
                {MalachiteCore::OP_INEG_RR, "OP_INEG_RRR"},
                {MalachiteCore::OP_DADD_RRR, "OP_DADD_RRR"},
                {MalachiteCore::OP_DSUB_RRR, "OP_DSUB_RRR"},
                {MalachiteCore::OP_DMUL_RRR, "OP_DMUL_RRR"},
                {MalachiteCore::OP_DDIV_RRR, "OP_DDIV_RRR"},
                {MalachiteCore::OP_DNEG_RR, "OP_DNEG_RR"},

                // Logic [31-60]
                {MalachiteCore::OP_AND_RRR, "OP_AND_RRR"},
                {MalachiteCore::OP_OR_RRR, "OP_OR_RRR"},
                {MalachiteCore::OP_NOT_RR, "OP_NOT_RR"},
                {MalachiteCore::OP_CMP_RR, "OP_CMP_RR"},
                {MalachiteCore::OP_DCMP_RR, "OP_DCMP_RR"},
                {MalachiteCore::OP_GET_FLAG, "OP_GET_FLAG"},
                {MalachiteCore::OP_BIT_OR_RRR, "OP_BIT_OR_RRR"},
                {MalachiteCore::OP_BIT_NOT_RR, "OP_BIT_NOT_RR"},
                {MalachiteCore::OP_BIT_AND_RRR, "OP_BIT_AND_RRR"},
                {MalachiteCore::OP_BIT_OFFSET_LEFT_RRR, "OP_BIT_OFFSET_LEFT_RRR"},
                {MalachiteCore::OP_BIT_OFFSET_RIGHT_RRR, "OP_BIT_OFFSET_RIGHT_RRR"},

                // Memory [61-80]
                {MalachiteCore::OP_LOAD_RM, "OP_LOAD_RM"},
                {MalachiteCore::OP_STORE_MR, "OP_STORE_MR"},
                {MalachiteCore::OP_MOV_RR, "OP_MOV_RR"},
                {MalachiteCore::OP_MOV_RI_INT, "OP_MOV_RI_INT"},
                {MalachiteCore::OP_MOV_RI_UINT, "OP_MOV_RI_UINT"},
                {MalachiteCore::OP_MOV_RI_DOUBLE, "OP_MOV_RI_DOUBLE"},
                {MalachiteCore::OP_CREATE_FRAME, "OP_CREATE_FRAME"},
                {MalachiteCore::OP_DESTROY_FRAME, "OP_DESTROY_FRAME"},
                {MalachiteCore::OP_PUSH, "OP_PUSH"},
                {MalachiteCore::OP_POP, "OP_POP"},
                {MalachiteCore::OP_LOAD_LOCAL, "OP_LOAD_LOCAL"},
                {MalachiteCore::OP_STORE_LOCAL, "OP_STORE_LOCAL"},
                {MalachiteCore::OP_STORE_ENCLOSING_A, "OP_STORE_ENCLOSING_A"},
                {MalachiteCore::OP_LOAD_ENCLOSING_A, "OP_LOAD_ENCLOSING_A"},
                {MalachiteCore::OP_STORE_ENCLOSING_R, "OP_STORE_ENCLOSING_R"},
                {MalachiteCore::OP_LOAD_ENCLOSING_R, "OP_LOAD_ENCLOSING_R"},
                {MalachiteCore::OP_ALLOCATE_MEMORY, "OP_ALLOCATE_MEMORY"},
                {MalachiteCore::OP_FREE_MEMORY, "OP_FREE_MEMORY"},

                // Control flow [91-120]
                {MalachiteCore::OP_JMP, "OP_JMP"},
                {MalachiteCore::OP_JMP_CV, "OP_JMP_CV"},
                {MalachiteCore::OP_JMP_CNV, "OP_JMP_CNV"},
                {MalachiteCore::OP_CALL, "OP_CALL"},
                {MalachiteCore::OP_RET, "OP_RET"},
                {MalachiteCore::OP_HALT, "OP_HALT"},

                // System Calls [121]
                {MalachiteCore::OP_SYSTEM_CALL, "OP_SYSTEM_CALL"},

                // Type Conversion [122-127]
                {MalachiteCore::OP_TC_ITD_R, "OP_TC_ITD_R"},
                {MalachiteCore::OP_TC_DTI_R, "OP_TC_DTI_R"},
                {MalachiteCore::OP_TC_UITD_R, "OP_TC_UITD_R"},
                {MalachiteCore::OP_TC_UITI_R, "OP_TC_UITI_R"},
                {MalachiteCore::OP_TC_DTUI_R, "OP_TC_DTUI_R"},
                {MalachiteCore::OP_TC_ITUI_R, "OP_TC_ITUI_R"}
            };
            return OpCodeToString;
        }
        static std::unordered_map< std::string, MalachiteCore::OpCode>& GetStrOpCodeMap()
        {
            static std::unordered_map< std::string, MalachiteCore::OpCode> StringToOpCode{};

            if (StringToOpCode.size() == 0)     //not inited
            {
                for (auto& pair_ : GetOpCodeStrMap()) 
                {
                    StringToOpCode.insert({pair_.second,pair_.first});
                }
            }
            return StringToOpCode;
        }
        static std::unordered_map< std::string, uint64_t>& GetOpCodeConstantsMap()
        {
            static std::unordered_map< std::string,uint64_t> StringToOpCodeConstant =
            {   
                //Syscalls
                {"PRINT_INT", MalachiteCore::SysCall::PRINT_INT},
                {"PRINT_UINT", MalachiteCore::SysCall::PRINT_UINT},
                {"PRINT_DOUBLE", MalachiteCore::SysCall::PRINT_DOUBLE},
                {"PRINT_CHAR", MalachiteCore::SysCall::PRINT_CHAR},
                {"PRINT_CHAR_ARRAY", MalachiteCore::SysCall::PRINT_CHAR_ARRAY},
            };

            return StringToOpCodeConstant;
        }
    public:
        #pragma region  GetMethods
        static TokenType GetTokenType(const std::string& string_token)
        {
            auto map = GetTokensMap();
            return map.count(string_token) ? map[string_token] : TokenType::UNDEFINED;
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
        static std::string GetPseudoString(const PseudoOpCode& code)
        {
            auto map = GetStringPseudoMap();
            return map.count(code) ? map[code] : "Nop";
        }
        static std::string GetByteString(const MalachiteCore::OpCode& code)
        {
            auto map = GetOpCodeStrMap();
            return map.count(code) ? map[code] : "Nop";
        }
        static MalachiteCore::OpCode GetByteFromString(const std::string& code)
        {
            auto map = GetStrOpCodeMap();
            return map.count(code) ? map[code] : MalachiteCore::OpCode::OP_NOP;
        }

        static uint64_t GetOpCodeConstant(const std::string& code)
        {
            auto map = GetOpCodeConstantsMap();
            return map.count(code) ? map[code] : SIZE_MAX;
        }



        static PseudoOpCode GetOperationFromComplexAssign(const std::string& code)
        {
            static std::unordered_map<std::string, PseudoOpCode> assigns = {
                {"=",  PseudoOpCode::Store},
                {"+=", PseudoOpCode::Add},
                {"-=", PseudoOpCode::Subtract},
                {"*=", PseudoOpCode::Multiplication},
                {"/=", PseudoOpCode::Division},
                {"%=", PseudoOpCode::Mod},
                {"&=", PseudoOpCode::BitAnd},
                {"|=", PseudoOpCode::BitOr},
                {"<<=",PseudoOpCode::BitOffsetLeft},
                {">>=",PseudoOpCode::BitOffsetRight},
                {"&&=",PseudoOpCode::And},
                {"||=",PseudoOpCode::Or},
                {"~=", PseudoOpCode::BitNot},
            };
            return assigns.count(code) ? assigns[code] : PseudoOpCode::Nop;
        }
        

        #pragma endregion

        
        enum ConditionBlockParType {NOTHING,START,MIDDLE,END};    //If - start, elif - middle, else - end
        
        static ConditionBlockParType GetCondtionBlockPartType(const std::string& token_str) 
        {
            if (token_str == SyntaxInfoKeywords::Get().keyword_if) return ConditionBlockParType::START;
            if (token_str == SyntaxInfoKeywords::Get().keyword_elif) return ConditionBlockParType::MIDDLE;
            if (token_str == SyntaxInfoKeywords::Get().keyword_else) return ConditionBlockParType::END;

            return ConditionBlockParType::NOTHING;
        }

        static std::unordered_set<std::string> GetOpCodeRegistersList()
        {
            static std::unordered_set<std::string> opCodeRegisters = { "RA","RB","RC","RD","RE","RF","RG","RH" };
            return opCodeRegisters;
        }
        static std::unordered_set<std::string> GetLoopsList()
        {
            static std::unordered_set<std::string> LoopsList = { SyntaxInfoKeywords::Get().keyword_loop,SyntaxInfoKeywords::Get().keyword_while,SyntaxInfoKeywords::Get().keyword_for,SyntaxInfoKeywords::Get().keyword_for_math};
            return LoopsList;
        }
        
    };

    class PseudoCodeInfo 
    {
    private:
        uint64_t global_label_id = 1;
    public:
        const std::string variableID_name = "ID";
        const std::string typeID_name = "Type";
        const std::string functionID_name = "ID";
        const std::string valueID_name = "Value";
        const std::string labelID_name = "ID";
        const std::string labelMark_name = "Mark";
        const std::string sectionName_name = "SectionName";

        const std::string flag_name = "Flag";

        const std::string opcodeCommandCode_name = "OpCode";
        const std::string opcodeDestination_name = "Destination";
        const std::string opcodeSource0_name = "Source0";
        const std::string opcodeSource1_name = "Source1";
        const std::string opcodeImmediate_name = "Immediate";

        const std::string opcodeStoreCommand_name = "STORE_VR";
        const std::string opcodeLoadCommand_name = "LOAD_RV";
        uint64_t GetNewLabelID()
        {
            return global_label_id++;
        }

        static PseudoCodeInfo& Get() 
        {
            static PseudoCodeInfo pfn;
            return pfn;
        }
    };
    
}
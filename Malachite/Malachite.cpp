// Malachite.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "include/core/functions.h"
#include "include/compiler/Lexer.hpp"
#include "include/compiler/ASTBuilder.hpp"
#include "include/compiler/PseudoByteDecoder.hpp"
#include <vector>
using namespace MalachiteCore;

int main()
{
	VMState state;


	//test
	//ыuint64_t value = 't';
	//ыvalue <<= 8;
	//ыvalue |= 's';
	//ыvalue <<= 8;
	//ыvalue |= 'e';
	//ыvalue <<= 8;
	//ыvalue |= 't';
	//ы
	//ыstd::vector<VMCommand> commands =
	//ы{
	//ы	VMCommand(OpCode::OP_MOV_RI_INT,0,Register(value)),
	//ы	VMCommand(OpCode::OP_MOV_RI_INT,2,Register((uint64_t)4)),
	//ы	VMCommand(OpCode::OP_STORE_MR,0,0,4),
	//ы	VMCommand(OpCode::OP_SYSTEM_CALL,SysCall::PRINT_CHAR_ARRAY,1,2)
	//ы};
	//ыexecute(&state, commands.data(), 4);

	std::string code = R"CODE(
	{
		int x = 239 *568 + 34
		int y = 239 *568 + 34 - x
	}
	

)CODE";

	Malachite::Lexer lexer;
	auto tokens = lexer.ToTokens(code);
	Malachite::ASTBuilder astbuilder;
	auto tree = astbuilder.BuildAST(tokens);
	Malachite::PseudoByteDecoder pbd;
	auto r = pbd.GeneratePseudoCode(tree.children);

	for (auto& t : r.second) 
	{
		std::string output;
		output += Malachite::SyntaxInfo::GetPseudoString(t.op_code) + " ";
		for (auto& t1 : t.parameters) 
		{
			output += t1.first + ":" + t1.second.ToString() + " ";
		}
		Malachite::Logger::Get().PrintInfo(output);
	}

}

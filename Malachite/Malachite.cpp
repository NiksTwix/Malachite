// Malachite.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "include/core/functions.h"
#include "include/compiler/Lexer.hpp"
#include "include/compiler/ASTBuilder.hpp"
#include "include/compiler/PseudoByteDecoder.hpp"
#include "include/compiler/ByteDecoder.hpp"
#include <vector>
using namespace MalachiteCore;

int main()
{
	


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

	if (x > 0) 
	{
		if (x > 0) 
		{
			
		}
		elif (x > 0) 
		{
			
		}
		else
		{
			
		}
	}

    if (x > 0) 
	{
		
	}
    else
	{
		
	}

    if (x > 0) 
	{
		
	}
    elif (x > 0) 
	{
		
	}
    else
	{
		
	}
	
	
	

)CODE";

	Malachite::Lexer lexer;
	auto tokens = lexer.ToTokens(code);
	Malachite::ASTBuilder astbuilder;
	auto tree = astbuilder.BuildAST(tokens);
	astbuilder.PostprocessTree(tree);
	int i = 0;
	//Malachite::PseudoByteDecoder pbd;
	//auto r = pbd.GeneratePseudoCode(tree);
	//Malachite::ByteDecoder bd;
	//auto r1 = bd.PseudoToByte(r);
	//std::cout << "PseudoByteCode-------------------------------\n";
	//for (auto& t : r.second) 
	//{
	//	std::string output;
	//	output += Malachite::SyntaxInfo::GetPseudoString(t.op_code) + " ";
	//	for (auto& t1 : t.parameters) 
	//	{
	//		output += t1.first + ":" + t1.second.ToString() + " ";
	//	}
	//	Malachite::Logger::Get().PrintInfo(output);
	//}
	//std::cout << "ByteCode-------------------------------------\n";
	//for (auto& t1 : r1)
	//{
	//	std::string output;
	//	output += Malachite::SyntaxInfo::GetByteString(t1.operation) + "  ";
	//	output += std::to_string(t1.destination) + "\t";
	//	output += std::to_string(t1.source0) + "\t";
	//	output += std::to_string(t1.source1) + "\t";
	//	output += std::to_string(t1.immediate.i) + "I|\t";
	//	output += std::to_string(t1.immediate.u) + "U|\t";
	//	output += std::to_string(t1.immediate.d) + "D|\t";
	//	Malachite::Logger::Get().PrintInfo(output);
	//}
	//
	//VMState state;
	//MalachiteCore::VMError err = execute(&state, r1.data(), r1.size());
	//
	//if (err)
	//{
	//	auto r = state.error_stack.top();
	//	std::cout << "Error:" << (uint16_t)err << " " << "IP:" << r.ip << "\n";
	//}
	//
	//
	//std::cout << "i" << " " << "u" << " " << "d" << "\n";
	//for (int i = 0; i < 10; i++) 
	//{
	//	auto reg = state.registers[i];
	//	std::cout << reg.i << "," << reg.u << "," << reg.d << "\n";
	//}
}

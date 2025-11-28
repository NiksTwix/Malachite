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
	std::string code = R"CODE(
	int x = 2000;
	
	if (x == 2000)
	{
		op_code	hello_world
		{
			OP_MOV_RI_INT RA, 'H'
			OP_MOV_RI_INT RB, 'e'
			OP_MOV_RI_INT RC, 'l'
			OP_MOV_RI_INT RD, 'l'
			OP_MOV_RI_INT RE, 'o'
			OP_MOV_RI_INT RF, ' '
			OP_MOV_RI_INT RG, 'w'
			OP_MOV_RI_INT RH, 'o'
			OP_SYSTEM_CALL 2 RA
			OP_SYSTEM_CALL 2 RB
			OP_SYSTEM_CALL 2 RC
			OP_SYSTEM_CALL 2 RD
			OP_SYSTEM_CALL 2 RE
			OP_SYSTEM_CALL 2 RF
			OP_SYSTEM_CALL 2 RG
			OP_SYSTEM_CALL 2 RH
			OP_MOV_RI_INT RA, 'r'
			OP_MOV_RI_INT RB, 'l'
			OP_MOV_RI_INT RC, 'd'
			OP_SYSTEM_CALL 2 RA
			OP_SYSTEM_CALL 2 RB
			OP_SYSTEM_CALL 2 RC
		}
	}
	else
	{
		op_code	no
		{
			OP_MOV_RI_INT RA, 'N'
			OP_MOV_RI_INT RB, '0'
			OP_MOV_RI_INT RC, ':'
			OP_MOV_RI_INT RD, '('
			OP_SYSTEM_CALL 2 RA
			OP_SYSTEM_CALL 2 RB
			OP_SYSTEM_CALL 2 RC
			OP_SYSTEM_CALL 2 RD
		}	
	}

)CODE";

	Malachite::Lexer lexer;
	auto tokens = lexer.ToTokens(code);
	Malachite::ASTBuilder astbuilder;
	auto tree = astbuilder.BuildAST(tokens);
	astbuilder.PostprocessTree(tree);
	int i = 0;
	Malachite::PseudoByteDecoder pbd;
	auto r = pbd.GeneratePseudoCode(tree);
	Malachite::ByteDecoder bd;
	auto r1 = bd.PseudoToByte(r);

	std::cout << "MalachiteTest--------------------------------\n";

	std::cout << "SourceCode-----------------------------------\n";
	std::cout << code << "\n";
	std::cout << "PseudoByteCode-------------------------------\n";
	std::cout << "Size: " << r.second.size() << "\n";
	int index = 0;
	for (auto& t : r.second) 
	{
		std::string output;
		output += Malachite::SyntaxInfo::GetPseudoString(t.op_code) + " ";
		for (auto& t1 : t.parameters) 
		{
			output += t1.first + ":" + t1.second.ToString() + " ";
		}
		Malachite::Logger::Get().PrintInfo(output,index);
		index++;
	}
	std::cout << "ByteCode-------------------------------------\n";
	index = 0;
	for (auto& t1 : r1)
	{
		std::string output;
		output += Malachite::SyntaxInfo::GetByteString(t1.operation) + "  ";
		output += std::to_string(t1.destination) + "\t";
		output += std::to_string(t1.source0) + "\t";
		output += std::to_string(t1.source1) + "\t";
		output += std::to_string(t1.immediate.i) + "I|\t";
		output += std::to_string(t1.immediate.u) + "U|\t";
		output += std::to_string(t1.immediate.d) + "D|\t";
		Malachite::Logger::Get().PrintInfo(output,index);
		index++;
	}
	
	VMState state;
	MalachiteCore::VMError err = execute(&state, r1.data(), r1.size());
	
	//if (err)
	//{
	//	auto r = state.error_stack.top();
	//	std::cout << "Error:" << (uint16_t)err << " " << "IP:" << r.ip << "\n";
	//}
	//
	//std::cout << "Registers dump-------------------------------------\n";
	//std::cout << "integer\t\t\tunsigned integer\t\t\tdouble\n";
	//for (int i = 0; i < 10; i++) 
	//{
	//	auto reg = state.registers[i];
	//	std::cout << reg.i << "\t\t\t" << reg.u << "\t\t\t" << reg.d << "\n";
	//}
}

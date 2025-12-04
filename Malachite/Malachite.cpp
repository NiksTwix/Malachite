// Malachite.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include "include/core/functions.h"
#include "include/compiler/Lexer.hpp"
#include "include/compiler/ASTBuilder.hpp"
#include "include/compiler/PseudoByteDecoder.hpp"
#include "include/compiler/ByteDecoder.hpp"
#include <vector>
#include <chrono>
using namespace MalachiteCore;

int main()
{
	std::string code = R"CODE(
int x = 100
int y = 100;
while (x < 300 && (y*y) >= 10000)
{
	x += 1
	if (x > 150 && x < 180): continue
	elif (x > 210 && x < 220) { continue }
	elif (x == 250)
	{
		op_code
		{
			OP_MOV_RI_INT RA, '|'
			OP_SYSTEM_CALL PRINT_CHAR, RA
		}
		break;
	}
	else
	{
		op_code
		{
			LOAD_RV RA, x
			OP_SYSTEM_CALL PRINT_INT, RA
			OP_MOV_RI_INT RA, ' '
			OP_SYSTEM_CALL PRINT_CHAR, RA
		}
	}
}
x = 5000
op_code
{
		LOAD_RV RA, x
		OP_SYSTEM_CALL PRINT_INT, RA
		OP_MOV_RI_INT RA, ' '
		OP_SYSTEM_CALL PRINT_CHAR, RA
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
	int depth = 0;
	for (auto& t : r.second) 
	{
		std::string output;
		if (t.op_code == Malachite::PseudoOpCode::OpenVisibleScope) depth++;
		for (int i = 0; i < depth; i++) output += "    ";
		if (t.op_code == Malachite::PseudoOpCode::CloseVisibleScope) depth--;
		output += Malachite::SyntaxInfo::GetPseudoString(t.op_code) + " ";
		for (auto& t1 : t.parameters) 
		{
			output += t1.first + ":" + t1.second.ToString() + " ";
		}
		Malachite::Logger::Get().PrintInfo(output,index);
		index++;
	}
	std::cout << "Global table of variables-------------------------------\n";
	for (auto& t : r.first->variables_global_table)
	{
		std::cout << t.second.variable_id << "|" << t.second.name << "|" << t.second.type_id << "\n";

	}
	std::cout << "ByteCode-------------------------------------\n";
	index = 0;
	depth = 0;
	for (auto& t1 : r1)
	{
		std::string output;
		if (t1.operation == MalachiteCore::OpCode::OP_CREATE_FRAME) depth++;
		for (int i = 0; i < depth; i++) output += "     ";
		if (t1.operation == MalachiteCore::OpCode::OP_DESTROY_FRAME) depth--;
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
	
	std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
	VMState state;
	MalachiteCore::VMError err = execute(&state, r1.data(), r1.size());
	std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
	if (err)
	{
		auto r = state.error_stack.top();
		std::cout << "Error:" << (uint16_t)err << " " << "IP:" << r.ip << "\n";
	}
	std::cout << "Duration : " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t) << "\n";
	//std::cout << "Registers dump-------------------------------------\n";
	//std::cout << "integer\t\t\tunsigned integer\t\t\tdouble\n";
	//for (int i = 0; i < 10; i++) 
	//{
	//	auto reg = state.registers[i];
	//	std::cout << reg.i << "\t\t\t" << reg.u << "\t\t\t" << reg.d << "\n";
	//}



}

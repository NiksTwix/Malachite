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
// 3. БЫСТРАЯ ПРОВЕРКА АРИФМЕТИКИ
int a = 7
int b = 3
int c = 0

c = a * b - (a + b) / 2  // 7*3 - (7+3)/2 = 21 - 5 = 16

op_code {
    OP_MOV_RI_INT RA, '\n'
    OP_SYSTEM_CALL PRINT_CHAR, RA
    OP_MOV_RI_INT RA, 'C'
    OP_SYSTEM_CALL PRINT_CHAR, RA  
    OP_MOV_RI_INT RA, '='
    OP_SYSTEM_CALL PRINT_CHAR, RA
    LOAD_RV RA, c
    OP_SYSTEM_CALL PRINT_INT, RA  // 16?
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
	
	VMState state;
	MalachiteCore::VMError err = execute(&state, r1.data(), r1.size());
	
	if (err)
	{
		auto r = state.error_stack.top();
		std::cout << "Error:" << (uint16_t)err << " " << "IP:" << r.ip << "\n";
	}
	
	//std::cout << "Registers dump-------------------------------------\n";
	//std::cout << "integer\t\t\tunsigned integer\t\t\tdouble\n";
	//for (int i = 0; i < 10; i++) 
	//{
	//	auto reg = state.registers[i];
	//	std::cout << reg.i << "\t\t\t" << reg.u << "\t\t\t" << reg.d << "\n";
	//}



}

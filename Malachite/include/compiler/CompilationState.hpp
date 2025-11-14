#pragma once
#include <unordered_map>
#include <vector>
#include "Definitions.hpp"
#include <stdexcept>

namespace Malachite 
{
	using typeID = size_t;
	using functionID = size_t;

	struct Variable 
	{
		const std::string name;
		const typeID type_id;
		union {
			double d;
			int64_t i;
			uint64_t u;
		};
		uint64_t stack_offset;
		const bool is_const = false;
		Variable() : name(""), type_id(0) {}
		Variable(const std::string& name, typeID type_id, bool is_const = false) : name(name), type_id(type_id), is_const(is_const) {}
		Variable(const std::string& name, typeID type_id, uint64_t stack_offset, bool is_const = false) : name(name), type_id(type_id), stack_offset(stack_offset), is_const(is_const) {}
	};
	struct Function
	{
	private:
		static functionID GetGlobalID() { static functionID global_id = 1; return global_id++; }		
	public:
		const typeID return_type;
		const functionID function_id;
		const std::string name;
		std::vector<Variable> args;	//для сравнения и проверки при попытке вызова
		Function(const std::string& name, typeID return_type, std::vector<Variable>& args) : function_id(GetGlobalID()), name(name), args(args), return_type(return_type){}
		Function() : return_type(0), function_id(0),name("function") {}
	};
	struct VariableTable
	{
		std::unordered_map<std::string, Variable> variables_str;

		bool IsExists(std::string name) const
		{
			return variables_str.count(name);
		}
		Variable& operator[](std::string variable_name)
		{
			if (!variables_str.count(variable_name)) throw std::runtime_error("Type with name=" + variable_name + " isnt exists.");
			return variables_str[variable_name];
		}
		void AddVariable(Variable& var)
		{
			if (var.type_id == 0) throw std::runtime_error("Variable with name=" + var.name + " is invalid.");
			if (variables_str.count(var.name)) throw std::runtime_error("Variable with name=" + var.name + " already exists.");
			variables_str.emplace(var.name, std::move(var));
		}
	};

	struct FunctionsTable 
	{
		std::unordered_map<std::string, std::vector<functionID>> functions_str;	//vector for overloading
		std::unordered_map<functionID, Function> functions;
		//type tree and etc
	public:
		Function& operator[](functionID id)
		{
			if (!functions.count(id)) throw std::runtime_error("Function with id=" + std::to_string(id) + " isnt exists.");
			return functions[id];
		}
		const std::vector<functionID>& operator[](std::string func_name)
		{
			if (!functions_str.count(func_name)) throw std::runtime_error("Functions with name=" + func_name + " arnt exists.");
			return functions_str[func_name];
		}
		bool IsExists(std::string func_name) const
		{
			return functions_str.count(func_name);
		}
		bool IsExists(functionID id) const
		{
			return functions.count(id);
		}
		void AddFunction(Function& func)
		{
			if (func.function_id == 0) throw std::runtime_error("Function with id=" + std::to_string(func.function_id) + " is invalid.");
			if (functions.count(func.function_id)) throw std::runtime_error("Function with id=" + std::to_string(func.function_id) + " already exists.");
			functions_str[func.name].push_back(func.function_id);
			functions.emplace(func.function_id, std::move(func));
		}
	};

	struct Type 
	{
	private:
		static typeID GetGlobalID() { static typeID global_id = 1; return global_id++; }		//0 is basic for int and double
	public:
		enum class Category {PRIMITIVE,ALIAS,CLASS};
		const Category category;
		const typeID type_id;
		const typeID parent_type_id;
		const std::string name = "type";

		FunctionsTable methods_table;
		VariableTable fields_table;

		size_t size;

		Type(Category category, const std::string& name, typeID parent_type_id) : category(category), type_id(GetGlobalID()),  name(name), parent_type_id(parent_type_id){}
		Type(Category category, const std::string& name) : category(category), type_id(GetGlobalID()), name(name), parent_type_id(0) {}
		Type() : category(Category::PRIMITIVE), type_id(0), parent_type_id(0) {}
	};

	struct TypesTable		//Table for user types
	{
	private:
		std::unordered_map<typeID, Type> types;
		std::unordered_map<std::string, typeID> types_str;
		//type tree and etc
	public:
		Type& operator[](typeID id)
		{
			if (!types.count(id)) throw std::runtime_error("Type with id=" + std::to_string(id) + " isnt exists.");
			return types[id];
		}
		Type& operator[](std::string type_name)
		{
			if (!types_str.count(type_name)) throw std::runtime_error("Type with name=" + type_name + " isnt exists.");
			return types[types_str[type_name]];
		}
		bool IsExists(std::string type_name) const 
		{
			return types_str.count(type_name);
		}
		bool IsExists(typeID id) const
		{
			return types.count(id);
		}
		void AddType(Type& type) 
		{
			if (type.type_id == 0) throw std::runtime_error("Type with id=" + std::to_string(type.type_id) + " is invalid.");
			if (types.count(type.type_id)) throw std::runtime_error("Type with id=" + std::to_string(type.type_id) + " already exists.");
			if (types_str.count(type.name)) throw std::runtime_error("Type with name=" + type.name + " already exists.");

			types_str.emplace(type.name, type.type_id);
			types.emplace(type.type_id, std::move(type));
		}
	};

	struct VisibleFrame  //Кадр области видимости
	{
		VariableTable variables_table;
		TypesTable types_table;
		FunctionsTable functions_table;
	};

	//Пока что без именованных пространств
	//struct Namespace 
	//{
	//	std::string name;
	//	std::vector<VisibleFrame> frames; 
	//	std::unordered_map<std::string, Namespace> children;
	//
	//	Namespace() 
	//	{
	//		frames.emplace_back();	//Create first frame
	//	}
	//};

	
	struct CompilationState 
	{
		std::string program_name = "program";
		std::vector<VisibleFrame> spaces;	//если компилятор не находит type или переменную в нынешнем scope, то поднимается на 1 и ищет уже, переменная depth в декодере

		//std::vector<PseudoCommand> pseudo_commands;

		CompilationState() 
		{
			spaces.emplace_back();
			Type void_type(Type::Category::PRIMITIVE, "void");
			void_type.size = 1;
			spaces[0].types_table.AddType(void_type);
			Type int_type(Type::Category::PRIMITIVE, "int");
			int_type.size = 8;
			spaces[0].types_table.AddType(int_type);
			Type float_type(Type::Category::PRIMITIVE, "float");
			float_type.size = 8;
			spaces[0].types_table.AddType(float_type);
			Type bool_type(Type::Category::PRIMITIVE, "bool");
			bool_type.size = 1;
			spaces[0].types_table.AddType(bool_type);
			Type char_type(Type::Category::PRIMITIVE, "char");
			char_type.size = 1;
			spaces[0].types_table.AddType(char_type);
		}
		VisibleFrame* GetSpace(size_t depth = 0) { return depth < spaces.size() ? &spaces[depth]: nullptr; }
		VisibleFrame* GetCurrentSpace() { return spaces.size() > 0 ? &spaces[spaces.size()] : nullptr; }
		void PushSpace() { spaces.emplace_back(); }
		void PopSpace() { if (spaces.size() > 0) spaces.pop_back(); }
		bool HasSpaces() { return spaces.size() > 0; }
		size_t GetSpacesDepth() { return spaces.size()-1; }

	};
}
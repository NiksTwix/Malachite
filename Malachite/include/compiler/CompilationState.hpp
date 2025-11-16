#pragma once
#include <unordered_map>
#include <vector>
#include "Definitions.hpp"
#include <stdexcept>
#include <memory>
namespace Malachite 
{
	using typeID = size_t;
	using functionID = size_t;
	using variableID = size_t;
	using visibleFrameID = size_t;
	struct Variable 
	{
	private:
		static variableID GetGlobalID() { static variableID global_id = 1; return global_id++; }
	public:
		variableID variable_id;
		std::string name;
		typeID type_id;
		union {
			double d;
			int64_t i;
			uint64_t u;
		};
		uint64_t stack_offset;
		
		bool is_const = false;
		Variable() : name(""), type_id(0) {}
		Variable(const std::string& name, typeID type_id, bool is_const = false) : variable_id(GetGlobalID()), name(name), type_id(type_id), is_const(is_const) {}
		Variable(const std::string& name, typeID type_id, uint64_t stack_offset, bool is_const = false) : variable_id(GetGlobalID()),name(name), type_id(type_id), stack_offset(stack_offset), is_const(is_const) {}

		Variable(const Variable& other) : variable_id(other.variable_id),name(other.name), type_id(other.type_id), stack_offset(other.stack_offset),  is_const(other.is_const)
		{
			u = other.u;
		}
	};
	struct Function
	{
	private:
		static functionID GetGlobalID() { static functionID global_id = 1; return global_id++; }		
	public:
		typeID return_type;
		functionID function_id;
		std::string name;
		std::vector<Variable> args;	//для сравнения и проверки при попытке вызова
		Function(const std::string& name, typeID return_type, std::vector<Variable>& args) : function_id(GetGlobalID()), name(name), args(args), return_type(return_type){}
		Function() : return_type(0), function_id(0),name("function") {}
		Function(const Function& other) : function_id(other.function_id), name(other.name), return_type(other.return_type)
		{
			args = other.args;
		}
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
		Category category;
		typeID type_id;
		typeID parent_type_id;
		std::string name = "type";

		FunctionsTable methods_table;
		VariableTable fields_table;

		size_t size;

		Type(Category category, const std::string& name, typeID parent_type_id) : category(category), type_id(GetGlobalID()), parent_type_id(parent_type_id), name(name) {}
		Type(Category category, const std::string& name) : category(category), type_id(GetGlobalID()), parent_type_id(0), name(name) {}
		Type() : category(Category::PRIMITIVE), type_id(0), parent_type_id(0) {}

		Type(const Type& other) : category(other.category), type_id(other.type_id), parent_type_id(other.parent_type_id), name(other.name)
		{
			methods_table = other.methods_table;
			fields_table = other.fields_table;
		}

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
	private:
		static visibleFrameID GetGlobalID() { static visibleFrameID global_id = 1; return global_id++; }
	public:
		VariableTable variables_table;
		TypesTable types_table;
		FunctionsTable functions_table;
		visibleFrameID vfid;
		VisibleFrame() : vfid(GetGlobalID()) {}
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

		//For checks and recursion
		std::vector<std::shared_ptr<VisibleFrame>> spaces;	//если компилятор не находит type или переменную в нынешнем scope, то поднимается на 1 и ищет уже там, переменная depth в декодере

		//Global information
		std::unordered_map<typeID, Type> types_global_table;
		std::unordered_map<visibleFrameID, std::shared_ptr<VisibleFrame>> visible_frames_global_table;
		std::unordered_map<variableID, Variable> variables_global_table;
		std::unordered_map<functionID, Function> functions_global_table;
		//std::vector<PseudoCommand> pseudo_commands;

		CompilationState() 
		{
			spaces.emplace_back();
			Type void_type(Type::Category::PRIMITIVE, "void");
			void_type.size = 1;
			AddTypeToSpace(0, void_type);
			Type int_type(Type::Category::PRIMITIVE, "int");
			int_type.size = 8;
			AddTypeToSpace(0, int_type);
			Type float_type(Type::Category::PRIMITIVE, "float");
			float_type.size = 8;
			AddTypeToSpace(0, float_type);
			Type bool_type(Type::Category::PRIMITIVE, "bool");
			bool_type.size = 1;
			AddTypeToSpace(0, bool_type);
			Type char_type(Type::Category::PRIMITIVE, "char");
			char_type.size = 1;
			AddTypeToSpace(0, char_type);
		}

		void AddTypeToSpace(size_t space, Type& type)
		{
			VisibleFrame* vf = GetSpace(space);
			if (!space) return;
			vf->types_table.AddType(type);
			types_global_table.emplace(type.type_id, Type(type));	//adds to global_table
		}
		void AddVariableToSpace(size_t space, Variable& variable)
		{
			VisibleFrame* vf = GetSpace(space);
			if (!space) return;
			vf->variables_table.AddVariable(variable);
			variables_global_table.emplace(variable.variable_id, Variable(variable));	//adds to global_table
		}
		void AddTypeToSpace(size_t space, Function& function)
		{
			VisibleFrame* vf = GetSpace(space);
			if (!space) return;
			vf->functions_table.AddFunction(function);
			functions_global_table.emplace(function.function_id, Function(function));	//adds to global_table
		}

		VisibleFrame* GetSpace(size_t depth = 0) { return depth < spaces.size() ? spaces[depth].get() : nullptr; }
		VisibleFrame* GetCurrentSpace() { return spaces.size() > 0 ? spaces[spaces.size()].get() : nullptr; }
		void PushSpace() 
		{ 
			std::shared_ptr<VisibleFrame> vf = std::make_shared<VisibleFrame>();
			spaces.push_back(vf);
			visible_frames_global_table.insert({ vf->vfid,vf });
		}
		void PopSpace() { if (spaces.size() > 0) spaces.pop_back(); }
		bool HasSpaces() { return spaces.size() > 0; }
		size_t GetSpacesDepth() { return spaces.size()-1; }

	};
}
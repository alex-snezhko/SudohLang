#pragma once
#include <string>
#include <set>
#include <vector>
#include <regex>

// keeps track of all variables and functions defined/used in a Sudoh source file
class NameManager
{
	// simple struct to encapsulate function information
	struct SudohFunction
	{
		std::string name;
		int numParams;

		// overloaded < operator for std::set ordering
		bool operator<(const SudohFunction& other) const
		{
			if (name == other.name)
			{
				return numParams < other.numParams;
			}
			return name < other.name;
		}
	};

	// simple struct to encapsulate function call information
	struct SudohFunctionCall
	{
		SudohFunction func;
		size_t tokenNum;
	};

	// list of variables that have been declared in each scope; variables in scope 0 are in index 0, etc
	std::vector<std::set<std::string>> varsInScopeN;

	// a set of all functions that are available to be used
	std::set<SudohFunction> functionsDefined = {
		{ "print", 1 }, { "printLine", 1 }, { "length", 1 },
		{ "string", 1 }, { "integer", 1 }, { "random", 0 },
		{ "remove", 2 }, { "append", 2 }, { "insert", 3 }
	};
	// a list of programmer defined functions; does not include built-in functions as in functionsDefined
	std::vector<SudohFunction> newFunctions;
	// a list of all functions that the programmer has attempted to call
	std::vector<SudohFunctionCall> functionsUsed;

	// keep buffer of variables to then inject into scope when specified scope is entered;
	// used for function params and 'for' loop iteration variables
	std::vector<std::string> varsToInject;
	int injectionScope;

public:
	static const std::regex NAME_RE;

	NameManager() : injectionScope(0) {}
	static bool validName(const std::string& name);

	bool varExists(const std::string& name, bool inFunction);
	void addVar(const std::string& name);
	void addVarToNextScope(const std::string& name);
	void addFunction(const std::string& name, int numParams);
	void addFunctionCall(const std::string& name, int numParams, size_t tokenNum);

	void advanceScope();
	void endScope();

	void checkFuncCallsValid();

	const std::vector<SudohFunction>& getFunctionsDefined();
};
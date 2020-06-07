#ifndef NAME_MANAGER_H
#define NAME_MANAGER_H

#include <string>
#include <set>
#include <vector>
#include <regex>

// keeps track of all variables and procedures defined/used in a Sudoh source file
class NameManager
{
	// simple struct to encapsulate procedure information
	struct SudohProcedure
	{
		std::string name;
		int numParams;

		// overloaded < operator for std::set ordering
		bool operator<(const SudohProcedure& other) const;
	};

	// simple struct to encapsulate procedure call information
	struct SudohProcedureCall
	{
		SudohProcedure proc;
		size_t tokenNum;
	};

	// list of variables that have been declared in each scope; variables in scope 0 are in index 0, etc
	std::vector<std::set<std::string>> varsInScopeN;

	// a set of all procedures that are available to be used
	std::set<SudohProcedure> proceduresDefined = {
		{ "input", 0 }, { "print", 1 }, { "printLine", 1 }, { "length", 1 },
		{ "string", 1 }, { "integer", 1 }, { "number", 1 }, { "ascii", 1 },
		{ "random", 1 }, { "remove", 2 }, { "removeLast", 1 }, { "append", 2 },
		{ "insert", 3 }, { "range", 3 }, { "type", 1 }, { "pow", 2 },
		{ "cos", 1 }, { "sin", 1 }, { "tan", 1 }, { "acos", 1 },
		{ "asin", 1 }, { "atan", 1 }, { "atan2", 2 }, { "log", 2 }
	};
	// a list of programmer defined procedures in this source file; does not include
	// built-in/imported procedures as in proceduresDefined
	std::vector<SudohProcedure> newProcedures;
	// a list of all procedures that the programmer has attempted to call
	std::vector<SudohProcedureCall> proceduresUsed;

	// keep buffer of variables to then inject into scope when specified scope is entered;
	// used for procedure params and 'for' loop iteration variables
	std::vector<std::string> varsToInject;
	int injectionScope;

public:
	static const std::regex NAME_RE;

	NameManager() : injectionScope(0) {}
	static bool validName(const std::string& name);

	bool varExists(const std::string& name, bool inFunction);
	void addVar(const std::string& name);
	void addVarToNextScope(const std::string& name);
	void addProcedure(const std::string& name, int numParams);
	void importProcedures(const std::vector<SudohProcedure>& newProcs, const std::string& fileName);
	void addProcedureCall(const std::string& name, int numParams, size_t tokenNum);

	void advanceScope();
	void endScope();

	void checkProcCallsValid();

	const std::vector<SudohProcedure>& getProceduresDefined();
};

#endif

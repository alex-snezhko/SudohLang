#include "name_manager.h"
#include "syntax_ex.h"

const std::regex NameManager::NAME_RE = std::regex("[_a-zA-Z][_a-zA-Z0-9]*");

// overloaded < operator for std::set ordering
//bool NameManager::SudohFunction::operator<(const SudohFunction& other) const
//{
//	if (name == other.name)
//	{
//		return numParams < other.numParams;
//	}
//	return name < other.name;
//}

// checks if the given name is a valid variable/function name
bool NameManager::validName(const std::string& name)
{
	static const std::set<std::string> KEYWORDS = {
		"if", "then", "else", "do", "not", "true", "false", "null", "repeat", "while", "until",
		"for", "each", "return", "break", "continue", "mod", "function", "and", "or", "including" // TODO complete set
	};

	return std::regex_match(name, NAME_RE) && KEYWORDS.count(name) == 0;
}

// return whether a variable of given name exists at current scope
bool NameManager::varExists(const std::string& name, bool inFunction)
{
	for (size_t i = inFunction; i < varsInScopeN.size(); i++)
	{
		if (varsInScopeN[i].count(name) != 0)
		{
			return true;
		}
	}
	return false;
}

// advances the current scope, adding more space to place new variables at top scope
void NameManager::advanceScope()
{
	// add new scope for new variables to be placed into
	varsInScopeN.push_back(std::set<std::string>());

	// inject variables into scope from varsToInject buffer if needed
	if (!varsToInject.empty())
	{
		for (const std::string& e : varsToInject)
		{
			varsInScopeN.back().insert(e);
		}
		varsToInject.clear();
	}
}

// end of current scope, variables declared at top scope no longer valid
void NameManager::endScope()
{
	varsInScopeN.pop_back();
}

// add a variable to the top scope
void NameManager::addVar(const std::string& name)
{
	varsInScopeN.back().insert(name);
}

// place a new variable into buffer to be injected into next scope
void NameManager::addVarToNextScope(const std::string& name)
{
	if (std::find(varsToInject.begin(), varsToInject.end(), name) != varsToInject.end())
	{
		throw SyntaxException("function cannot have multiple parameters with same name");
	}
	injectionScope = varsInScopeN.size();
	varsToInject.push_back(name);
}

// check if all function calls made correspond to valid functions that have been declared
void NameManager::checkFuncCallsValid()
{
	// check that all function calls made in code are valid; this cannot be done while parsing because
	// functions must not be declared before being used (as in C++ for instance)
	for (const SudohFunctionCall& e : functionsUsed)
	{
		auto matching = functionsDefined.find(e.func);
		if (matching == functionsDefined.end())
		{
			throw SyntaxException("attempted use of undeclared function '" + e.func.name + "' accepting " +
				std::to_string(e.func.numParams) + " parameter(s)");
		}
		if (matching->numParams != e.func.numParams)
		{
			throw SyntaxException("expected " + std::to_string(matching->numParams) + " parameter(s) for function '" +
				matching->name + "' but got " + std::to_string(e.func.numParams));
		}
	}
}

// add a function to the list of functions that have been declared
void NameManager::addFunction(const std::string& name, int numParams)
{
	SudohFunction func = { name, numParams };
	if (functionsDefined.count(func) != 0)
	{
		throw SyntaxException("function named '" + name + "' taking " + std::to_string(numParams) +
			" parameter(s) has already been defined");
	}
	functionsDefined.insert(func);
	newFunctions.push_back(func);
}

// add a function call to the list of all function calls made
void NameManager::addFunctionCall(const std::string& name, int numParams, size_t tokenNum)
{
	functionsUsed.push_back({ { name, numParams }, tokenNum });
}

const std::vector<NameManager::SudohFunction>& NameManager::getFunctionsDefined()
{
	return newFunctions;
}

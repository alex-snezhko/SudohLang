//#include "sudohc.h"
//#include <iostream>
//#include <regex>
//#include <set>
//#include <map>
//#include <algorithm>
//
//enum class ParsedType { number, boolean, string, list, map, null, any };
//// returns string representation of a ParsedType enum
//std::string typeToString(const ParsedType t)
//{
//	switch (t)
//	{
//	case ParsedType::number:
//		return "number";
//	case ParsedType::boolean:
//		return "boolean";
//	case ParsedType::string:
//		return "string";
//	case ParsedType::list:
//		return "list";
//	case ParsedType::map:
//		return "map";
//	case ParsedType::null:
//		return "null";
//	}
//}
//
//class TranspiledBuffer
//{
//	// string which contains all statements that are not inside of a function; to be placed into main function
//	std::string transpiledMain;
//	// string which contains all function definitions
//	std::string transpiledFunctions;
//	// buffer string which will be written to one of the above transpiled strings at the end of a line
//	std::string uncommittedTrans;
//
//public:
//	// forward declaration of functions when needed
//	void commitLine(bool inFunction, int currStatementScope);
//	void appendToBuffer(const std::string& append);
//};
//
//class TokenIterator
//{
//	// list of tokens to be accessed by parsing functions
//	std::vector<Token> tokens;
//	// current index into tokens list
//	size_t tokenNum;
//
//public:
//	// returns string of the token that the parser is currently on
//	const std::string& nextToken();
//	int nextTokenScope(int currStatementScope);
//};
//
//class NameManager
//{
//	// list of variables that have been declared in each scope; variables in scope 0 are in index 0, etc
//	std::vector<std::set<std::string>> varsInScopeN;
//	// use to check for name conflicts
//	std::set<Token> allVariables;
//
//	// simple struct to encapsulate function information
//	struct SudohFunction
//	{
//		std::string name;
//		int numParams;
//
//		// overloaded < operator for std::set ordering
//		bool operator<(const SudohFunction& other) const
//		{
//			return name < other.name;
//		}
//	};
//	struct SudohFunctionCall
//	{
//		SudohFunction func;
//		size_t tokenNum;
//	};
//
//	// a set of all functions that are available to be used
//	std::set<SudohFunction> functionsDefined = {
//		{ "print", 1 }, { "printLine", 1 }, { "length", 1 },
//		{ "string", 1 }, { "integer", 1 }, { "random", 0 },
//		{ "remove", 2 }, { "append", 2 }, { "insert", 3 }
//	};
//	// a list of programmer defined functions; does not include built-in functions as in functionsDefined
//	std::vector<SudohFunction> newFunctions;
//	// a list of all functions that the programmer has attempted to call
//	std::vector<SudohFunctionCall> functionsUsed;
//
//public:
//	bool varExists(const std::string& name);
//};
//
//class Parser
//{
//	TranspiledBuffer trans;
//	TokenIterator tokens;
//	NameManager names;
//	// flag for determining whether the parser is currently inside of a function
//	bool inFunction;
//	int currStatementScope;
//
//	// adds a string to the uncommitted transpiled C++ code buffer and advances tokenNum
//	void appendAndAdvance(const std::string append);
//	void endOfLine();
//
//	bool parseNextLine(std::set<bool (*)()>& extraRules);
//	void parseBlock(std::set<bool (*)()>&);
//	bool parseStructure(bool (*&)(), void (*&)(int, std::set<bool (*)()>&));
//	bool parseAssignment();
//	bool parseFuncCall();
//	bool parseVar(bool);
//	enum struct VarParseMode { mayBeNew, mustExist, functionParam, forVar, forEachVar };
//	bool parseVarName(VarParseMode mode);
//	bool parseExpr(ParsedType&);
//	bool parseExpr();
//	void parseExpr(std::vector<ParsedType>);
//	bool parseTerm(ParsedType&);
//	void parseExprBinary(ParsedType&);
//	int parseCommaSep(bool (*)());
//
//public:
//	Parser(const char* fileName);
//};

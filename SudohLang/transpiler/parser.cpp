#include "sudohc.h"
#include <iostream>
#include <regex>
#include <set>
#include <map>

enum class ParsedType { number, boolean, string, list, map, null, any };
// returns string representation of a ParsedType enum
std::string typeToString(const ParsedType& t)
{
	switch (t)
	{
	case ParsedType::number:
		return "number";
	case ParsedType::boolean:
		return "boolean";
	case ParsedType::string:
		return "string";
	case ParsedType::list:
		return "list";
	case ParsedType::map:
		return "map";
	case ParsedType::null:
		return "null";
	}
}

const std::regex NAME_RE = std::regex("[_a-zA-Z][_a-zA-Z0-9]*");
const std::regex NUMBER_RE = std::regex("-?[0-9]+(\.[0-9]+)?");
const std::regex STRING_RE = std::regex("\".*\"");

const std::set<std::string> keywords = {
	"if", "then", "else", "do", "not", "true", "false", "null", "repeat", "while",
	"until", "for", "return", "break", "continue", "mod", "function", "and", "or" // TODO
};

std::vector<Token> tokens;
int tokenNum;
const std::string* fileContents;


std::vector<std::set<std::string>> varsInScopeN;
int currScopeLevel;

struct SudohFunction
{
	std::string name;
	int numParams;

	// overloaded < operator for std::set ordering
	bool operator<(const SudohFunction& other) const
	{
		return name < other.name;
	}
};
// a set of all functions that are available to be used
std::set<SudohFunction> functionsDefined = {
	{ "print", 1 }, { "printLine", 1 }, { "length", 1 },
	{ "string", 1 }, { "number", 1 }, { "random", 0 }
};
// a list of programmer defined functions; does not include built-in functions as in functionsDefined
std::vector<SudohFunction> newFunctions;
// a set of all functions that the programmer has attempted to call
std::set<SudohFunction> functionsUsed;
// flag for determining whether the parser is currently inside of a function
bool inFunction;

// string which contains all statements that are not inside of a function; to be placed into main function
std::string transpiledMain;
// string which contains all function definitions
std::string transpiledFunctions;
// buffer string which will be written to one of the above transpiled strings at the end of a line
std::string uncommittedTrans;


void commitLine();
bool parseNextLine(std::set<bool (*)()>&);
void parseBlock(std::set<bool (*)()>&);
bool parseStructure(bool (*&)(), void (*&)(int, std::set<bool (*)()>&));
bool parseAssignment();
bool parseFuncCall();
bool parseVar();
ParsedType discard;
bool parseExpr(ParsedType & = discard);
bool parseTerm(ParsedType&);
void parseExprBinary(ParsedType&);
bool parseVal(ParsedType&);
int parseCommaSep(bool (*)());
void skipToNextLine();

const std::string END = "";
// TODO maybe change working with tokens to pointers
const std::string& nextToken(bool advance, bool skipWhitespace = true)
{
	int init = tokenNum;

	int endScope = currScopeLevel;
	if (skipWhitespace)
	{
		while (tokenNum < tokens.size())
		{
			if (tokens[tokenNum].token == "\n")
			{
				endScope = 0;
			}
			else if (tokens[tokenNum].token == "\t")
			{
				endScope++;
			}
			else
			{
				break;
			}
			tokenNum++;
		}
	}
	if (tokenNum == tokens.size())
	{
		return END;
	}

	if (endScope < currScopeLevel)
	{
		//TODO parserError("first line of multiline statement must be indented at or below level of successive lines");
	}

	const std::string& ret = tokens[tokenNum].token;
	tokenNum = advance ? tokenNum + 1 : init;
	return ret;
}

// returns scope level of next line
void skipToNextLine()
{
	const std::string* token = &nextToken(false, false);

	if (*token == "\n")
	{
		int scope;
		do
		{
			tokenNum++;
			scope = 0;

			token = &nextToken(false, false);
			while (*token == "\t")
			{
				tokenNum++;
				scope++;
				token = &nextToken(false, false);
				if (*token == END)
				{
					currScopeLevel = 0;
					return;
				}
			}

		} while (*token == "\n" || token->substr(0, 2) == "//");

		currScopeLevel = scope;
	}
	if (currScopeLevel == -1)
	{
		currScopeLevel = 0;
	}
}

void parserError(const std::string message)
{
	const std::string& contents = *fileContents;
	int fileCharIdx = tokens[tokenNum].fileCharNum;

	int beginLine = fileCharIdx, endLine = fileCharIdx;
	while (beginLine != 0 && contents[beginLine - 1] != '\n' && contents[beginLine - 1] != '\t')
	{
		beginLine--;
	}
	while (endLine != contents.length() && contents[endLine] != '\n')
	{
		endLine++;
	}
	std::string line = contents.substr(beginLine, endLine - beginLine);

	std::cout << "Compiler error on line " << tokens[tokenNum].lineNum << ": " << message << std::endl
		<< "\t" << line << std::endl << "\t";
	for (int i = 0; i < fileCharIdx - beginLine; i++)
	{
		std::cout << " ";
	}
	std::cout << "^" << std::endl;
}

//  +-----------------------+
//  |   Parsing functions   |
//  +-----------------------+

// returns token number broken at
int parse(const std::string& contents, std::string& transpiled)
{
	tokens = tokenize(contents);
	fileContents = &contents;
	inFunction = false;

	skipToNextLine();
	if (currScopeLevel != 0)
	{
		parserError("first line of file must not be indented");
	}

	currScopeLevel = -1;

	uncommittedTrans = "int main()";
	commitLine();

	std::set<bool (*)()> extraRules = {};
	parseBlock(extraRules);

	for (const SudohFunction& e : functionsUsed)
	{
		auto matching = functionsDefined.find(e);
		if (matching == functionsDefined.end())
		{
			parserError("attempted use of undeclared function " + e.name);
		}
		if (matching->numParams != e.numParams)
		{
			parserError("expected " + std::to_string(matching->numParams) +
				" parameters for function '" + matching->name + "'");
		}
	}

	// finalize transpiled content
	transpiled = "#include \"sudoh.h\"\n\n";
	if (newFunctions.size() != 0)
	{
		for (const SudohFunction& e : newFunctions)
		{
			transpiled += "Variable " + e.name + "(";
			for (int i = 0; i < e.numParams; i++)
			{
				transpiled += std::string("Variable&") + (i < e.numParams - 1 ? ", " : "");
			}
			transpiled += ");\n";
		}

		transpiled += "\n" + transpiledFunctions + "\n";
	}
	transpiled += transpiledMain;

	return 0; //TODO debug
}

void commitLine()
{
	// determine whether to write new line to inside of main or to global scope (for functions)
	std::string& commitTo = inFunction ? transpiledFunctions : transpiledMain;

	for (int i = 0; i < currScopeLevel + !inFunction; i++)
	{
		commitTo += "\t";
	}
	commitTo += uncommittedTrans + "\n";
	uncommittedTrans = "";
}

void assertEndOfLine()
{
	const std::string& token = nextToken(false, false);
	if (token != "\n" && token != END)
	{
		parserError("each statement/program element must be on a new line");
	}
}

bool parseNextLine(std::set<bool (*)()>& extraRules)
{
	if (nextToken(false, false) == END)
	{
		return false;
	}

	if (parseFuncCall() || parseAssignment())
	{
		uncommittedTrans += ";";
		assertEndOfLine();
		commitLine();
		return true;
	}

	void (*parseAfter)(int, std::set<bool (*)()>&) = nullptr;
	bool (*extraRule)() = nullptr;
	if (parseStructure(extraRule, parseAfter))
	{
		assertEndOfLine();
		commitLine();

		int scope = currScopeLevel;
		if (extraRule && extraRules.count(extraRule) == 0)
		{
			extraRules.insert(extraRule);
			parseBlock(extraRules);
			extraRules.erase(extraRule);
		}
		else
		{
			parseBlock(extraRules);
		}

		if (parseAfter)
		{
			parseAfter(scope, extraRules);
		}
		return true;
	}

	for (auto& e : extraRules)
	{
		if (e())
		{
			assertEndOfLine();
			commitLine();
			return true;
		}
	}

	parserError("invalid line");
}

void parseBlock(std::set<bool (*)()>& extraRules)
{
	uncommittedTrans = "{";
	commitLine();

	int blockScope = currScopeLevel + 1;
	skipToNextLine();
	if (currScopeLevel < blockScope)
	{
		parserError("empty block not allowed");
	}

	varsInScopeN.push_back(std::set<std::string>());

	while (currScopeLevel == blockScope && parseNextLine(extraRules))
	{
		skipToNextLine();
	}

	if (inFunction && blockScope == 1)
	{
		uncommittedTrans = "\treturn null;";
		commitLine();
	}

	if (currScopeLevel > blockScope)
	{
		parserError("illegal attempt to increase indentation level");
	}

	int temp = currScopeLevel;
	currScopeLevel = blockScope - 1;
	uncommittedTrans = "}";
	commitLine();
	currScopeLevel = temp;

	varsInScopeN.pop_back();
}

enum struct VarStatus { invalid, newVar, existingVar };
enum struct VarParseMode { mayBeNew, mustExist, functionParam };

// modes:	can be new (normal variable assignment)
//			cannot be new (using variable)
//			will accept any (function params) (do not search scope 0)
VarStatus parseVarName(VarParseMode mode)
{
	const std::string& name = nextToken(false);

	// do not accept keyword as valid variable name
	if (std::regex_match(name, NAME_RE) && keywords.count(name) == 0)
	{
		tokenNum++;
		// keep buffer of function parameters to then inject into scope when function is entered
		static std::vector<std::string> functionParams;

		// add variable to functionParams buffer if this is a function param
		if (mode == VarParseMode::functionParam)
		{
			functionParams.push_back(name);
			uncommittedTrans += "Variable& _" + name;
			return VarStatus::newVar;
		}
		else
		{
			// inject function parameters into current scope if function has just been entered
			if (currScopeLevel == 1 && !functionParams.empty())
			{
				for (const std::string& e : functionParams)
				{
					varsInScopeN[1].insert(e);
				}
				functionParams.clear();
			}

			// find whether a variable with this name already exists
			bool exists = false;
			for (const std::set<std::string>& e : varsInScopeN)
			{
				if (e.count(name) != 0)
				{
					exists = true;
					break;
				}
			}

			if (!exists)
			{
				if (mode == VarParseMode::mustExist)
				{
					parserError("use of undeclared variable " + name);
				}
				varsInScopeN[currScopeLevel].insert(name);
				uncommittedTrans += "Variable ";
			}
			uncommittedTrans += "_" + name;

			return exists ? VarStatus::existingVar : VarStatus::newVar;
		}
	}
	return VarStatus::invalid;
}

bool parseMapEntry()
{
	uncommittedTrans += "{ ";
	if (!parseExpr())
	{
		parserError("expected expression as map key");
	}
	uncommittedTrans += ", ";
	if (nextToken(true) != "<-")
	{
		parserError("map entry must be of form <key> <- <value>");
	}
	if (!parseExpr())
	{
		parserError("expected expression as map value");
	}
	uncommittedTrans += " }";
	return true;
}

bool extraParseInsideLoop()
{
	const std::string& t = nextToken(false);
	if (t == "break" || t == "continue")
	{
		uncommittedTrans += t + ";";
		tokenNum++;
		return true;
	}
	return false;
}

bool extraParseFunction()
{
	if (nextToken(false) == "return")
	{
		uncommittedTrans = "return ";
		tokenNum++;
		parseExpr();
		uncommittedTrans += ";";
		return true;
	}
	return false;
}

void parseAfterIf(int scope, std::set<bool (*)()>& extraRules)
{
	bool elseReached = false;
	while (currScopeLevel == scope)
	{
		if (nextToken(false) == "else")
		{
			uncommittedTrans = "else";
			tokenNum++;
			if (nextToken(false) == "if")
			{
				tokenNum++;
				if (elseReached)
				{
					parserError("'else' block cannot be followed by 'else if' block");
				}

				uncommittedTrans += " if (";
				ParsedType type;
				if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
				{
					if (nextToken(true) == "then")
					{
						uncommittedTrans += ")";
						assertEndOfLine();
						commitLine();

						parseBlock(extraRules);
						continue;
					}
					parserError("expected 'then'");
				}
				parserError("expected condition");
			}
			if (elseReached)
			{
				parserError("multiple 'else' blocks not allowed");
			}
			elseReached = true;

			assertEndOfLine();
			commitLine();
			parseBlock(extraRules);
			continue;
		}
		return;
	}
}

void parseAfterRepeat(int scope, std::set<bool (*)()>& extraRules)
{
	if (currScopeLevel == scope)
	{
		const std::string& token = nextToken(true);
		if (token == "while" || token == "until")
		{
			uncommittedTrans = std::string("while (") + (token == "until" ? "!(" : "");
			ParsedType type;
			if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
			{
				uncommittedTrans += token == "until" ? "))" : ")";
				assertEndOfLine();
			}
			parserError("expected condition");
		}
	}
	parserError("expected 'while' or 'until' condition after 'repeat' block");
}

bool parseStructure(bool (*&additionalRule)(), void (*&parseAfter)(int, std::set<bool (*)()>&))
{
	int init = tokenNum;
	const std::string* token = &nextToken(true);
	if (*token == "if")
	{
		uncommittedTrans += "if (";
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken(true) == "then")
			{
				uncommittedTrans += ")";
				parseAfter = parseAfterIf;
				return true;
			}
			parserError("expected 'then'");
		}
		parserError("expected condition");
	}

	if (*token == "while" || *token == "until")
	{
		uncommittedTrans += std::string("while (") + (*token == "until" ? "!(" : "");
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken(true) == "do")
			{
				uncommittedTrans += *token == "until" ? "))" : ")";
				additionalRule = extraParseInsideLoop;
				return true;
			}
			parserError("expected 'do'");
		}
		parserError("expected condition");
	}

	if (*token == "for")
	{
		uncommittedTrans += "for (";

		const std::string& forVar = nextToken(false);
		// for i <- n1 (down)? to n2 do
		//     ^
		if (parseVarName(VarParseMode::mayBeNew) != VarStatus::invalid)
		{
			// for i <- n1 (down)? to n2 do
			//       ^
			if (nextToken(true) == "<-")
			{
				uncommittedTrans += " = ";
				// for i <- n1 (down)? to n2 do
				//          ^
				ParsedType type;
				if (parseExpr(type) && (type == ParsedType::number || type == ParsedType::any))
				{
					uncommittedTrans += "; _" + forVar;
					// for i <- n1 (down)? to n2 do
					//              ^
					token = &nextToken(true);
					bool down = false;
					if (*token == "to" || (down = true, *token == "down" && nextToken(true) == "to"))
					{
						uncommittedTrans += down ? " >= " : " <= ";
						// for i <- n1 (down)? to n2 do
						//                        ^
						if (parseExpr(type) && (type == ParsedType::number || type == ParsedType::any))
						{
							// for i <- n1 (down)? to n2 do
							//                           ^
							if (nextToken(true) == "do")
							{
								uncommittedTrans += "; _" + forVar + " = _" + forVar + (down ? " - 1)" : " + 1)");
								additionalRule = extraParseInsideLoop;
								return true;
							}
							parserError("expected 'do'");
						}
						parserError("final value of 'for' loop should be numeric expression");
					}
					parserError("expected 'to' or 'down to'");
				}
				parserError("value of 'for' loop variable should be numeric expression");
			}
			parserError("expected initial value assignment to variable in 'for'");
		}
		parserError("expected variable name after 'for'");
		// TODO implement range based for loop
	}

	if (*token == "repeat")
	{
		uncommittedTrans = "do";
		additionalRule = extraParseInsideLoop;
		parseAfter = parseAfterRepeat;
		return true;
	}

	if (*token == "function")
	{
		if (inFunction)
		{
			parserError("nested function illegal");
		}

		const std::string& funcName = nextToken(true);
		if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
		{
			uncommittedTrans += "Variable _" + funcName + "(";
			if (nextToken(true) == "(")
			{
				inFunction = true;
				int numParams = parseCommaSep([] { return parseVarName(VarParseMode::functionParam) != VarStatus::invalid; });

				if (nextToken(true) == ")")
				{
					uncommittedTrans += ")";
					SudohFunction func = { funcName, numParams };
					functionsDefined.insert(func);
					newFunctions.push_back(func);

					additionalRule = extraParseFunction;
					parseAfter = [](auto, auto) { inFunction = false; };
					return true;
				}
				parserError("closing parenthesis expected");
			}
			parserError("expected argument list after function declaration");
		}
		parserError("invalid function name");
	}

	tokenNum = init;
	return false;
}

bool parseAssignment()
{
	// determine whether or not this is a valid lvalue
	VarStatus status = parseVarName(VarParseMode::mayBeNew);
	if (status != VarStatus::invalid)
	{
		while (nextToken(false) == "[")
		{
			uncommittedTrans += "[";
			if (status == VarStatus::newVar)
			{
				parserError("cannot access map value of undefined variable");
			}

			tokenNum++;
			if (parseExpr())
			{
				if (nextToken(true) == "]")
				{
					uncommittedTrans += "]";
					continue;
				}
				parserError("expected ']'");
			}
			parserError("expected expression inside bracket");
		}

		if (nextToken(true) == "<-")
		{
			uncommittedTrans += " = ";
			if (parseExpr())
			{
				return true;
			}
			parserError("expected expression");
		}
		return false;
	}

	return false;
}

bool parseFuncCallParam()
{
	int initLen = uncommittedTrans.length();

	ParsedType t;
	bool ret = parseExpr(t);
	if (ret && t != ParsedType::any)
	{
		static int tmpNum = 0;
		std::string tmpVarName = "__tmp" + std::to_string(tmpNum++);

		std::string initUncommitted = uncommittedTrans;
		uncommittedTrans = "Variable " + tmpVarName + " = " + uncommittedTrans.substr(initLen) + ";";
		commitLine();

		uncommittedTrans = initUncommitted.substr(0, initLen) + tmpVarName;
	}
	return ret;
}

bool parseFuncCall()
{
	const std::string& funcName = nextToken(false);
	if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
	{
		tokenNum++;
		if (nextToken(false) == "(")
		{
			tokenNum++;
			uncommittedTrans += "_" + funcName + "(";
			int numParams = parseCommaSep(parseFuncCallParam);

			if (nextToken(true) == ")")
			{
				functionsUsed.insert({ funcName, numParams });
				uncommittedTrans += ")";
				return true;
			}
			parserError("expected closing parenthesis for function call");
		}
		tokenNum--;
		return false;
	}
	return false;
}

bool parseVar()
{
	if (parseVarName(VarParseMode::mustExist) != VarStatus::invalid)
	{
		while (nextToken(false) == "[")
		{
			tokenNum++;
			uncommittedTrans += "[";
			if (parseExpr())
			{
				if (nextToken(true) == "]")
				{
					uncommittedTrans += "]";
					continue;
				}
				parserError("expected closing bracket");
			}
			parserError("expected expression inside bracket");
		}

		return true;
	}
	return false;
}



//  +-------------------------+
//  |   Expressiong parsing   |
//  |   functions             |
//  +-------------------------+

bool parseExpr(ParsedType& t)
{
	if (parseTerm(t))
	{
		parseExprBinary(t);
		return true;
	}
	return false;
}

bool parseTerm(ParsedType& t)
{
	int init = tokenNum;
	ParsedType type;
	const std::string& token = nextToken(true);

	// check for unary prefix operators such as -, not
	/*if (token == "-") TODO decide whether to remove
	{
		uncommittedTrans += "-";
		if (parseExpr(type))
		{
			if (type == ParsedType::number || type == ParsedType::any)
			{
				return t = type, true;
			}
			parserError("cannot apply '-' unary operator to non-numeric item");
		}
		parserError("expected expression after '-'");
	}*/
	if (token == "not")
	{
		uncommittedTrans += "!";
		if (parseExpr(type))
		{
			if (type == ParsedType::boolean || type == ParsedType::any)
			{
				return t = type, true;
			}
			parserError("cannot apply 'not' operator to non-boolean item");
		}
		parserError("expected expression after 'not'");
	}

	// check for parenthesized expression
	if (token == "(")
	{
		uncommittedTrans += "(";
		if (parseExpr(type))
		{
			if (nextToken(true) == ")")
			{
				uncommittedTrans += ")";
				return t = type, true;
			}
			return tokenNum = init, false;
		}
		return tokenNum = init, false;
	}

	tokenNum = init;
	return parseVal(t);
}

bool parseVal(ParsedType& t)
{
	int init = tokenNum;
	std::string initStr = uncommittedTrans;

	const std::string& token = nextToken(true);

	// check for boolean
	if (token == "true" || token == "false")
	{
		uncommittedTrans += token;
		t = ParsedType::boolean;
		return true;
	}

	// check for number
	if (std::regex_match(token, NUMBER_RE))
	{
		uncommittedTrans += token;
		t = ParsedType::number;
		return true;
	}

	// check for string
	if (std::regex_match(token, STRING_RE))
	{
		uncommittedTrans += "String(" + token + ")";
		t = ParsedType::string;
		return true;
	}

	// check for list
	if (token == "[")
	{
		uncommittedTrans += "List{ ";
		parseCommaSep([] { return parseExpr(); });
		if (nextToken(true) == "]")
		{
			uncommittedTrans += " }";
			t = ParsedType::list;
			return true;
		}
		parserError("expected end of list");
	}

	// check for map
	if (token == "{")
	{
		uncommittedTrans += "Map{ ";
		// flag to keep track of whether another entry is needed (after comma)
		bool lastComma = false;

		parseCommaSep(parseMapEntry);

		if (nextToken(true) == "}")
		{
			uncommittedTrans += " }";
			t = ParsedType::map;
			return true;
		}
		parserError("expected closing curly brace");
	}

	// check for null value
	if (token == "null")
	{
		uncommittedTrans += token;
		t = ParsedType::null;
		return true;
	}

	// check if this is valid variable-type expression indicating 'any' type
	tokenNum = init;
	if (parseFuncCall() || parseVar())
	{
		t = ParsedType::any;
		return true;
	}

	// failed all checks, this is not a value
	uncommittedTrans = initStr;
	tokenNum = init;
	return false;
}

void checkValidBinary(const std::string binaryOp, const std::string transBinOp,
	ParsedType& leftType, const std::map<ParsedType, std::set<ParsedType>>& allowedOps)
{
	uncommittedTrans += " " + transBinOp + " ";

	ParsedType rightType;
	if (!parseTerm(rightType))
	{
		return;
	}

	if (leftType == ParsedType::any)
	{
		parseExprBinary(leftType);
		return;
	}

	for (const auto& e : allowedOps)
	{
		if (leftType == e.first && (rightType == ParsedType::any || e.second.count(rightType) != 0))
		{
			parseExprBinary(leftType);
			return;
		}
	}

	parserError("'" + binaryOp + "' cannot be applied to types " +
		typeToString(leftType) + " and " + typeToString(rightType));
}

void parseExprBinary(ParsedType& type)
{
	int init = tokenNum;

	typedef std::map<ParsedType, std::set<ParsedType>> OpMap;

	static const std::set<ParsedType> all = {
		ParsedType::number, ParsedType::boolean, ParsedType::string,
		ParsedType::list, ParsedType::map, ParsedType::null
	};

	// lists of valid type pairs for each binary operation
	static const OpMap plus = {
		{ ParsedType::number, { ParsedType::number } },
		{ ParsedType::string, all },
		{ ParsedType::list, all }
	};
	static const OpMap arithmetic = { { ParsedType::number, { ParsedType::number } } };
	static const OpMap boolOps = { { ParsedType::boolean, { ParsedType::boolean } } };
	static const OpMap comparisonOps = {
		{ ParsedType::number, { ParsedType::number } },
		{ ParsedType::boolean, { ParsedType::boolean } },
		{ ParsedType::string, { ParsedType::string } },
		{ ParsedType::list, { ParsedType::list } },
		{ ParsedType::map, { ParsedType::map } }
	};

	const std::string& token = nextToken(true);
	ParsedType rightType;
	if (token == "+")
	{
		checkValidBinary(token, token, type, plus);
	}
	else if (token == "-" || token == "*" || token == "/" || token == "mod")
	{
		checkValidBinary(token, token == "mod" ? "%" : token, type, arithmetic);
	}
	else if (token == "and" || token == "or")
	{
		checkValidBinary(token, token == "and" ? "&&" : "||", type, boolOps);
	}
	else if (token == "=" || token == "!=" || token == "<" || token == "<=" || token == ">" || token == ">=")
	{
		// TODO make this change the type to boolean somehow
		// TODO idea- need higher precedence for comparison than others (to parse 1+2=2+2 correctly for instance)
		checkValidBinary(token, token == "=" ? "==" : token, type, comparisonOps);
	}
	else
	{
		// epsilon also accepted
		tokenNum = init;
	}
}

// parses a comma-separated list of items and returns number of items found
int parseCommaSep(bool (*checkFunc)())
{
	int numItems = 0;
	if (checkFunc())
	{
		numItems++;
		// loop to handle further comma-separated items
		while (nextToken(false) == ",")
		{
			uncommittedTrans += ", ";
			tokenNum++;
			if (checkFunc())
			{
				numItems++;
				continue;
			}
			parserError("expected item after comma");
		}
	}

	return numItems;
}

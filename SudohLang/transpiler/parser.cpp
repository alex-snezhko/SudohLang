#include "sudohc.h"
#include <iostream>
#include <regex>
#include <set>
#include <map>

enum class ParsedType { number, boolean, string, list, map, null, any };
// returns string representation of a ParsedType enum
std::string typeToString(const ParsedType t)
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
	"until", "for", "each", "return", "break", "continue", "mod", "function", "and", "or" // TODO complete set
};

std::vector<Token> tokens;
int tokenNum;

std::vector<std::set<std::string>> varsInScopeN;
int currStatementScope;

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
bool parseVar(bool);
ParsedType discard;
bool parseExpr(ParsedType & = discard);
bool parseTerm(ParsedType&);
void parseExprBinary(ParsedType&);
bool parseVal(ParsedType&);
int parseCommaSep(bool (*)());

const std::string& nextToken()
{
	return tokens[tokenNum].tokenString;
}

void appendAndAdvance(const std::string append)
{
	if (tokenNum < tokens.size())
	{
		tokenNum++;
	}
	uncommittedTrans += append;
}

// returns scope level of next line
int nextTokenScope()
{
	int scope = currStatementScope;
	if (scope == -1)
	{
		return 0;
	}
	const std::string* token = &nextToken();
	while (*token == "\n" || *token == "\t")
	{
		if (*token == "\n")
		{
			scope = 0;
		}
		else if (*token == "\t")
		{
			scope++;
		}
		tokenNum++;
		token = &nextToken();
	}
	if (*token == END)
	{
		return 0;
	}
	return scope;
}

//  +-----------------------+
//  |   Parsing functions   |
//  +-----------------------+

// returns token number broken at
bool parse(const std::string& contents, std::string& transpiled)
{
	try
	{
		tokens = tokenize(contents);
		tokenNum = 0;
		inFunction = false;

		if (nextTokenScope() != 0)
		{
			throw SyntaxException("base scope must not be indented");
		}

		currStatementScope = -1;

		uncommittedTrans = "int main()";
		commitLine();

		std::set<bool (*)()> extraRules = {};
		parseBlock(extraRules);

		for (const SudohFunction& e : functionsUsed)
		{
			auto matching = functionsDefined.find(e);
			if (matching == functionsDefined.end())
			{
				throw SyntaxException("attempted use of undeclared function " + e.name);
			}
			if (matching->numParams != e.numParams)
			{
				// TODO call at right token num
				throw SyntaxException("expected " + std::to_string(matching->numParams) + " parameters for function '" +
					matching->name + "' but got " + std::to_string(e.numParams));
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

			transpiled += "\n" + transpiledFunctions;
		}
		transpiled += transpiledMain;
		return true;
	}
	catch (SyntaxException& e)
	{
		size_t fileCharIdx = tokens[tokenNum].fileCharNum;

		size_t beginLine = fileCharIdx, endLine = fileCharIdx;
		while (beginLine != 0 && contents[beginLine - 1] != '\n' && contents[beginLine - 1] != '\t')
		{
			beginLine--;
		}
		while (endLine != contents.length() && contents[endLine] != '\n')
		{
			endLine++;
		}
		std::string line = contents.substr(beginLine, endLine - beginLine);

		std::cout << "Syntax error on line " << tokens[tokenNum].lineNum << ": " << e.what() << "\n\t" << line << "\n\t";
		for (int i = 0; i < fileCharIdx - beginLine; i++)
		{
			std::cout << " ";
		}
		std::cout << "^\nAborting compilation.\n";
		return false;
	}
}

void commitLine()
{
	// determine whether to write new line to inside of main or to global scope (for functions)
	std::string& commitTo = inFunction ? transpiledFunctions : transpiledMain;

	for (int i = 0; i < currStatementScope + !inFunction; i++)
	{
		commitTo += "\t";
	}
	commitTo += uncommittedTrans + "\n";
	uncommittedTrans = "";
}

void endOfLine()
{
	const std::string& token = nextToken();
	if (token != "\n" && token != END)
	{
		throw SyntaxException("each statement/program element must be on a new line");
	}
	commitLine();
}

bool parseNextLine(std::set<bool (*)()>& extraRules)
{
	if (nextToken() == END)
	{
		return false;
	}

	if (parseFuncCall() || parseAssignment())
	{
		uncommittedTrans += ";";
		endOfLine();
		return true;
	}

	void (*parseAfter)(int, std::set<bool (*)()>&) = nullptr;
	bool (*extraRule)() = nullptr;
	if (parseStructure(extraRule, parseAfter))
	{
		endOfLine();

		int scope = currStatementScope;
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
			endOfLine();
			return true;
		}
	}

	throw SyntaxException("invalid line");
	return true;
}

void parseBlock(std::set<bool (*)()>& extraRules)
{
	uncommittedTrans = "{";
	commitLine();

	int blockScope = currStatementScope + 1;
	currStatementScope = nextTokenScope();
	if (currStatementScope < blockScope)
	{
		throw SyntaxException("empty block not allowed");
	}

	varsInScopeN.push_back(std::set<std::string>());

	while (currStatementScope == blockScope && parseNextLine(extraRules))
	{
		currStatementScope = nextTokenScope();
	}

	if (inFunction && blockScope == 1)
	{
		uncommittedTrans = "\treturn null;";
		commitLine();
	}

	if (currStatementScope > blockScope)
	{
		throw SyntaxException("illegal attempt to increase indentation level");
	}

	int temp = currStatementScope;
	currStatementScope = blockScope - 1;
	uncommittedTrans = "}";
	commitLine();
	currStatementScope = temp;

	varsInScopeN.pop_back();
}

enum struct VarParseMode { mayBeNew, mustExist, functionParam, forVar, forEachVar };

bool varExists(const std::string& name)
{
	for (const std::set<std::string>& e : varsInScopeN)
	{
		if (e.count(name) != 0)
		{
			return true;
		}
	}
	return false;
}

// modes:	can be new (normal variable assignment)
//			cannot be new (using variable)
//			will accept any (function params) (do not search scope 0)
bool parseVarName(VarParseMode mode)
{
	const std::string& name = nextToken();

	// do not accept keyword as valid variable name
	if (std::regex_match(name, NAME_RE) && keywords.count(name) == 0)
	{
		// keep buffer of variables to then inject into scope when specified scope is entered;
		// used for function params and 'for' loop iteration variables
		static std::vector<std::string> varsToInject;
		static int injectionScope;

		if (currStatementScope == injectionScope && !varsToInject.empty())
		{
			for (const std::string& e : varsToInject)
			{
				varsInScopeN[currStatementScope].insert(e);
			}
			varsToInject.clear();
		}

		switch (mode)
		{
		case VarParseMode::mayBeNew:
			if (varExists(name))
			{
				appendAndAdvance("_" + name);
				break;
			}
			varsInScopeN[currStatementScope].insert(name);
			appendAndAdvance("Variable _" + name);
			break;
		case VarParseMode::mustExist:
			if (varExists(name))
			{
				appendAndAdvance("_" + name);
				break;
			}
			throw SyntaxException("use of undeclared variable " + name);

		case VarParseMode::forVar:
			if (varExists(name))
			{
				appendAndAdvance("_" + name);
				break;
			}
			injectionScope = currStatementScope + 1;
			varsToInject.push_back(name);
			appendAndAdvance("Variable _" + name);
			break;
		case VarParseMode::forEachVar:
			if (varExists(name))
			{
				throw SyntaxException("'for each' iteration variable must be a new variable");
			}
		case VarParseMode::functionParam:
			injectionScope = currStatementScope + 1;
			varsToInject.push_back(name);
			appendAndAdvance("Variable& _" + name);
			break;
		}
		return true;
	}
	return false;
}

bool parseMapEntry()
{
	uncommittedTrans += "{ ";
	if (nextToken() == "}")
	{
		return false;
	}
	if (parseExpr())
	{
		if (nextToken() == "<-")
		{
			appendAndAdvance(", ");
			if (parseExpr())
			{
				uncommittedTrans += " }";
				return true;
			}
			throw SyntaxException("expected expression as map value");
		}
		throw SyntaxException("map entry must be of form <key> <- <value>");
	}
	throw SyntaxException("expected expression as map key");
}

bool extraParseInsideLoop()
{
	const std::string& t = nextToken();
	if (t == "break" || t == "continue")
	{
		appendAndAdvance(t + ";");
		return true;
	}
	return false;
}

bool extraParseFunction()
{
	if (nextToken() == "return")
	{
		appendAndAdvance("return ");
		if (nextToken() != "\n")
		{
			parseExpr();
		}
		else
		{
			uncommittedTrans += "null";
		}
		uncommittedTrans += ";";
		return true;
	}
	return false;
}

void parseAfterIf(int scope, std::set<bool (*)()>& extraRules)
{
	bool elseReached = false;
	while (currStatementScope == scope)
	{
		if (nextToken() == "else")
		{
			appendAndAdvance("else");
			if (nextToken() == "if")
			{
				if (elseReached)
				{
					throw SyntaxException("'else' block cannot be followed by 'else if' block");
				}

				appendAndAdvance(" if (");
				ParsedType type;
				if (parseExpr(type))
				{
					if (type == ParsedType::boolean || type == ParsedType::any)
					{
						if (nextToken() == "then")
						{
							appendAndAdvance(")");

							endOfLine();
							parseBlock(extraRules);
							continue;
						}
						throw SyntaxException("expected 'then'");
					}
					throw SyntaxException("'else if' condition must be of boolean type");
				}
				throw SyntaxException("expected condition");
			}
			if (elseReached)
			{
				throw SyntaxException("multiple 'else' blocks not allowed");
			}
			
			endOfLine();
			elseReached = true;

			parseBlock(extraRules);
			continue;
		}
		return;
	}
}

void parseAfterRepeat(int scope, std::set<bool (*)()>& extraRules)
{
	if (currStatementScope == scope)
	{
		const std::string& token = nextToken();
		if (token == "while" || token == "until")
		{
			appendAndAdvance("while (" + std::string(token == "until" ? "!(" : ""));
			ParsedType type;
			if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
			{
				uncommittedTrans += token == "until" ? "));" : ");";
				endOfLine();
				return;
			}
			throw SyntaxException("expected boolean condition");
		}
	}
	throw SyntaxException("expected 'while' or 'until' condition after 'repeat' block");
}

bool parseStructure(bool (*&additionalRule)(), void (*&parseAfter)(int, std::set<bool (*)()>&))
{
	const std::string* token = &nextToken();
	if (*token == "if")
	{
		appendAndAdvance("if (");
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken() == "then")
			{
				appendAndAdvance(")");
				parseAfter = parseAfterIf;
				return true;
			}
			throw SyntaxException("expected 'then'");
		}
		throw SyntaxException("expected boolean condition");
	}

	if (*token == "while" || *token == "until")
	{
		appendAndAdvance("while (" + std::string(*token == "until" ? "!(" : ""));
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken() == "do")
			{
				appendAndAdvance(*token == "until" ? "))" : ")");
				additionalRule = extraParseInsideLoop;
				return true;
			}
			throw SyntaxException("expected 'do'");
		}
		throw SyntaxException("expected boolean condition");
	}

	if (*token == "for")
	{
		appendAndAdvance("for (");

		const std::string& forVar = nextToken();
		// for i <- [n] (down)? to [n] do
		//     ^
		if (parseVarName(VarParseMode::forVar))
		{
			// for i <- [n] (down)? to [n] do
			//       ^
			if (nextToken() == "<-")
			{
				appendAndAdvance(" = ");
				// for i <- [n] (down)? to [n] do
				//           ^
				ParsedType type;
				if (parseExpr(type) && (type == ParsedType::number || type == ParsedType::any))
				{
					uncommittedTrans += "; _" + forVar;
					// for i <- [n] (down)? to [n] do
					//              ^
					bool down = false;
					if (nextToken() == "down")
					{
						down = true;
						tokenNum++;
					}
					if (nextToken() == "to")
					{
						appendAndAdvance(down ? " >= " : " <= ");
						// for i <- [n] (down)? to [n] do
						//                          ^
						if (parseExpr(type) && (type == ParsedType::number || type == ParsedType::any))
						{
							// for i <- [n] (down)? to [n] do
							//                             ^
							if (nextToken() == "do")
							{
								appendAndAdvance("; _" + forVar + " = _" + forVar + (down ? " - 1)" : " + 1)"));
								additionalRule = extraParseInsideLoop;
								return true;
							}
							throw SyntaxException("expected 'do'");
						}
						throw SyntaxException("final value of 'for' loop variable should be a numeric expression");
					}
					throw SyntaxException("expected 'to' or 'down to'");
				}
				throw SyntaxException("initial value of 'for' loop variable should be a numeric expression");
			}
			throw SyntaxException("expected initial value assignment to variable in 'for'");
		}

		// for each e in [x] do
		//     ^
		if (nextToken() == "each")
		{
			tokenNum++;
			// for each e in [x] do
			//          ^
			if (parseVarName(VarParseMode::forEachVar))
			{
				// for each e in [x] do
				//            ^
				if (nextToken() == "in")
				{
					appendAndAdvance(" : ");
					static std::set<ParsedType> valid = {
						ParsedType::string, ParsedType::list,
						ParsedType::map, ParsedType::any
					};

					// for each e in [x] do
					//                ^
					ParsedType type;
					if (parseExpr(type) && valid.count(type) != 0)
					{
						// for each e in [x] do
						//                   ^
						if (nextToken() == "do")
						{
							appendAndAdvance(")");
							additionalRule = extraParseInsideLoop;
							return true;
						}
						throw SyntaxException("expected 'do'");
					}
					throw SyntaxException("'for each' iteratable object should be a collection: (string, list, or map)");
				}
				throw SyntaxException("expected 'in'");
			}
			throw SyntaxException("expected declaration of 'for each' loop iteration variable");
		}
		throw SyntaxException("invalid 'for' loop statement");
	}

	if (*token == "repeat")
	{
		appendAndAdvance("do");
		additionalRule = extraParseInsideLoop;
		parseAfter = parseAfterRepeat;
		return true;
	}

	if (*token == "function")
	{
		if (inFunction)
		{
			throw SyntaxException("nested function illegal");
		}

		appendAndAdvance("Variable ");
		const std::string& funcName = nextToken();
		if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
		{
			appendAndAdvance("_" + funcName);
			if (nextToken() == "(")
			{
				appendAndAdvance("(");
				inFunction = true;
				int numParams = parseCommaSep([] { return parseVarName(VarParseMode::functionParam); });

				if (nextToken() == ")")
				{
					appendAndAdvance(")");
					SudohFunction func = { funcName, numParams };
					functionsDefined.insert(func);
					newFunctions.push_back(func);

					additionalRule = extraParseFunction;
					parseAfter = [](auto, auto) { commitLine(); inFunction = false; };
					return true;
				}
				throw SyntaxException("closing parenthesis expected");
			}
			throw SyntaxException("expected argument list after function declaration");
		}
		throw SyntaxException("invalid function name");
	}

	return false;
}

bool parseAssignment()
{
	const std::string& name = nextToken();
	if (std::regex_match(name, NAME_RE) && keywords.count(name) == 0)
	{
		parseVar(true);

		if (nextToken() == "<-")
		{
			appendAndAdvance(" = ");
			if (parseExpr())
			{
				return true;
			}
			throw SyntaxException("expected expression");
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
	const std::string& funcName = nextToken();
	if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
	{
		tokenNum++;
		if (nextToken() == "(")
		{
			appendAndAdvance("_" + funcName + "(");
			int numParams = parseCommaSep(parseFuncCallParam);

			if (nextToken() == ")")
			{
				appendAndAdvance(")");
				functionsUsed.insert({ funcName, numParams });
				return true;
			}
			throw SyntaxException("expected closing parenthesis for function call");
		}
		tokenNum--;
		return false;
	}
	return false;
}

bool parseVar(bool lvalue)
{
	bool exists = varExists(nextToken());
	if (parseVarName(lvalue ? VarParseMode::mayBeNew : VarParseMode::mustExist))
	{
		while (nextToken() == "[")
		{
			if (!exists)
			{
				throw SyntaxException("cannot get map value of undeclared variable");
			}
			appendAndAdvance(lvalue ? "[" : "at(");
			if (parseExpr())
			{
				if (nextToken() == "]")
				{
					appendAndAdvance(lvalue ? "]" : ")");
					continue;
				}
				throw SyntaxException("expected closing bracket");
			}
			throw SyntaxException("expected expression inside bracket");
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
	const std::string& token = nextToken();

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
			throw SyntaxException("cannot apply '-' unary operator to non-numeric item");
		}
		throw SyntaxException("expected expression after '-'");
	}*/
	if (token == "not")
	{
		appendAndAdvance("!");
		if (parseExpr(t))
		{
			if (t == ParsedType::boolean || t == ParsedType::any)
			{
				return true;
			}
			throw SyntaxException("cannot apply 'not' operator to non-boolean item");
		}
		throw SyntaxException("expected expression after 'not'");
	}

	// check for parenthesized expression
	if (token == "(")
	{
		appendAndAdvance("(");
		if (parseExpr(t))
		{
			if (nextToken() == ")")
			{
				appendAndAdvance(")");
				return true;
			}
			throw SyntaxException("expected closing parenthesis");
		}
		return false;
	}

	return parseVal(t);
}

bool parseVal(ParsedType& t)
{
	const std::string& token = nextToken();

	// check for boolean
	if (token == "true" || token == "false")
	{
		appendAndAdvance(token);
		t = ParsedType::boolean;
		return true;
	}

	// check for number
	if (std::regex_match(token, NUMBER_RE))
	{
		appendAndAdvance(token);
		t = ParsedType::number;
		return true;
	}

	// check for string
	if (std::regex_match(token, STRING_RE))
	{
		appendAndAdvance("std::string(" + token + ")");
		t = ParsedType::string;
		return true;
	}

	// check for null value
	if (token == "null")
	{
		appendAndAdvance("null");
		t = ParsedType::null;
		return true;
	}

	// check for list
	if (token == "[")
	{
		appendAndAdvance("List{ ");
		parseCommaSep([] { return parseExpr(); });
		if (nextToken() == "]")
		{
			if (nextTokenScope() < currStatementScope)
			{
				throw SyntaxException("list closing brace indentation may not be less than indentation of opening brace");
			}
			appendAndAdvance(" }");
			t = ParsedType::list;
			return true;
		}
		throw SyntaxException("expected closing ']'");
	}

	// check for map
	if (token == "{")
	{
		appendAndAdvance("Map{ ");
		parseCommaSep(parseMapEntry);

		if (nextToken() == "}")
		{
			if (nextTokenScope() < currStatementScope)
			{
				throw SyntaxException("map closing brace indentation may not be less than indentation of opening brace");
			}
			appendAndAdvance(" }");
			t = ParsedType::map;
			return true;
		}
		throw SyntaxException("expected closing '}'");
	}

	// check if this is valid variable-type expression indicating 'any' type
	if (parseFuncCall() || parseVar(false))
	{
		t = ParsedType::any;
		return true;
	}

	return false;
}

typedef std::map<ParsedType, std::set<ParsedType>> Operations;

void checkBinary(const std::string translatedBinOp, ParsedType& leftType, const Operations& allowedOps)
{
	const std::string& sudohBinOp = nextToken();
	appendAndAdvance(" " + translatedBinOp + " ");

	if (nextToken() == "\n" && nextTokenScope() <= currStatementScope)
	{
		throw SyntaxException("subsequent lines of multiline arithmetic expression or compound "
			"condition must be indented above level of first line");
	}

	ParsedType rightType;
	if (parseTerm(rightType))
	{
		if (leftType == ParsedType::any || rightType == ParsedType::any)
		{
			parseExprBinary(leftType);
			return;
		}

		for (const auto& e : allowedOps)
		{
			if (leftType == e.first && e.second.count(rightType) != 0)
			{
				parseExprBinary(leftType);
				return;
			}
		}

		throw SyntaxException("'" + sudohBinOp + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}
}

void parseExprBinary(ParsedType& type)
{
	static const std::set<ParsedType> ALL = {
		ParsedType::number, ParsedType::boolean, ParsedType::string,
		ParsedType::list, ParsedType::map, ParsedType::null
	};

	const std::string& token = nextToken();
	if (token == "+")
	{
		checkBinary(token, type, {
			{ ParsedType::number, { ParsedType::number } },
			{ ParsedType::string, ALL },
			{ ParsedType::list, ALL }
		});
	}
	else if (token == "-" || token == "*" || token == "/" || token == "mod")
	{
		checkBinary(token == "mod" ? "%" : token, type, {
			{ ParsedType::number, { ParsedType::number } }
		});
	}
	else if (token == "and" || token == "or")
	{
		checkBinary(token == "and" ? "&&" : "||", type, {
			{ ParsedType::boolean, { ParsedType::boolean } }
		});
	}
	else if (token == "=" || token == "!=" || token == "<" || token == "<=" || token == ">" || token == ">=")
	{
		// TODO make this change the type to boolean somehow
		// idea- need higher precedence for comparison than others (to parse 1+2=2+2 correctly for instance)
		checkBinary(token == "=" ? "==" : token, type, {
			{ ParsedType::number, { ParsedType::number } },
			{ ParsedType::boolean, { ParsedType::boolean } },
			{ ParsedType::string, { ParsedType::string } },
			{ ParsedType::list, { ParsedType::list } },
			{ ParsedType::map, { ParsedType::map } }
		});
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
		while (nextToken() == ",")
		{
			appendAndAdvance(", ");
			if (nextToken() == "\n" && nextTokenScope() <= currStatementScope)
			{
				throw SyntaxException("subsequent lines in multiline comma-separated list must be "
					"indented at same level or higher than original line");
			}
			if (checkFunc())
			{
				numItems++;
				continue;
			}
			throw SyntaxException("expected item after comma");
		}
	}

	return numItems;
}

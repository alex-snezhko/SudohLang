#include "sudohc.h"
#include <iostream>
#include <regex>
#include <set>
#include <map>
#include <algorithm>

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
	default:
		return "any";
	}
}

const std::regex NAME_RE = std::regex("[_a-zA-Z][_a-zA-Z0-9]*");
const std::regex NUMBER_RE = std::regex("-?[0-9]+(\.[0-9]+)?");
const std::regex STRING_RE = std::regex("\".*\"");

const std::set<std::string> keywords = {
	"if", "then", "else", "do", "not", "true", "false", "null", "repeat", "while",
	"until", "for", "each", "return", "break", "continue", "mod", "function", "and", "or" // TODO complete set
};

// list of tokens to be accessed by parsing functions
std::vector<Token> tokens;
// current index into tokens list
size_t tokenNum;

// list of variables that have been declared in each scope; variables in scope 0 are in index 0, etc
std::vector<std::set<std::string>> varsInScopeN;
// scope that parser is currently in
int currStatementScope;

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
// a set of all functions that are available to be used
std::set<SudohFunction> functionsDefined = {
	{ "print", 1 }, { "printLine", 1 }, { "length", 1 },
	{ "string", 1 }, { "integer", 1 }, { "random", 0 },
	{ "remove", 2 }, { "append", 2 }, { "insert", 3 }
};
// a list of programmer defined functions; does not include built-in functions as in functionsDefined
std::vector<SudohFunction> newFunctions;

struct SudohFunctionCall
{
	SudohFunction func;
	size_t tokenNum;
};
// a list of all functions that the programmer has attempted to call
std::vector<SudohFunctionCall> functionsUsed;

// keep buffer of variables to then inject into scope when specified scope is entered;
// used for function params and 'for' loop iteration variables
static std::vector<std::string> varsToInject;
static int injectionScope;

// flag for determining whether the parser is currently inside of a function
bool inFunction;

// string which contains all statements that are not inside of a function; to be placed into main function
std::string transpiledMain;
// string which contains all function definitions
std::string transpiledFunctions;
// buffer string which will be written to one of the above transpiled strings at the end of a line
std::string uncommittedTrans;

// forward declaration of functions when needed
void commitLine();
bool parseNextLine(std::set<bool (*)()>&);
void parseBlock(std::set<bool (*)()>&);
bool parseStructure(bool (*&)(), void (*&)(int, std::set<bool (*)()>&));
bool parseAssignment();
bool parseFuncCall();
bool parseVar(bool);
void addVarToNextScope(const std::string& name);
void injectVarsIntoScope();
void parseExpr(ParsedType&);
void parseExpr();
void parseExpr(std::vector<ParsedType>);
void parseTerm(ParsedType&);
void parseExprBinary(ParsedType&);
int parseCommaSep(void (*)(), const std::string);

// returns string of the token that the parser is currently on
const std::string& currToken()
{
	return tokens[tokenNum].tokenString;
}

// adds a string to the uncommitted transpiled C++ code buffer and advances tokenNum
void appendAndAdvance(const std::string append)
{
	if (tokenNum < tokens.size())
	{
		tokenNum++;
	}
	uncommittedTrans += append;
}

// returns scope level of next significant token (no whitespace)
int nextTokenScope()
{
	int scope = currStatementScope;
	// special case for beginning of parse when scope is set to -1
	if (scope == -1)
	{
		return 0;
	}
	const std::string* token = &currToken();
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
		token = &currToken();
	}
	// count end of input as being is scope 0
	if (*token == END)
	{
		return 0;
	}
	return scope;
}

void maybeMultiline()
{
	if (currToken() == "\n" && nextTokenScope() <= currStatementScope)
	{
		throw SyntaxException("indentation of first line of multiline statement "
			"must be less than indentation of following lines", tokenNum);
	}
}

//  +-----------------------+
//  |   Parsing functions   |
//  +-----------------------+

// main function for initiating transpilation; returns whether transpilation was successful or not
bool parse(const std::string& contents, std::string& transpiled)
{
	try
	{
		// initialize needed data for parse/transpilation
		tokenize(contents, tokens);
		tokenNum = 0;
		inFunction = false;

		if (nextTokenScope() != 0)
		{
			throw SyntaxException("base scope must not be indented", tokenNum);
		}

		// initialize currStatementScope to -1 because global Sudoh code will be treated as a block
		// for parseBlock(), which will then automatically search for code in the current scope + 1, which
		// will correctly search for global code in scope 0
		currStatementScope = -1;

		uncommittedTrans = "int main()";
		commitLine();

		// parse global block; extraRules will contain a set of extra parse rules allowed when necessary
		// (such as allowing 'break', 'continue' when inside a loop); no extra rules initially
		std::set<bool (*)()> extraRules = {};
		parseBlock(extraRules);

		// check that all function calls made in code are valid; this cannot be done while parsing because
		// functions must not be declared before being used (as in C++ for instance)
		for (const SudohFunctionCall& e : functionsUsed)
		{
			auto matching = functionsDefined.find(e.func);
			if (matching == functionsDefined.end())
			{
				tokenNum = e.tokenNum;
				throw SyntaxException("attempted use of undeclared function '" + e.func.name + "' accepting " +
					std::to_string(e.func.numParams) + " parameter(s)", tokenNum);
			}
			if (matching->numParams != e.func.numParams)
			{
				tokenNum = e.tokenNum;
				throw SyntaxException("expected " + std::to_string(matching->numParams) + " parameters for function '" +
					matching->name + "' but got " + std::to_string(e.func.numParams), tokenNum);
			}
		}

		// combine all transpiled content;
		// structure - #include, function forward declarations, function definitions, main
		transpiled = "#include \"sudoh.h\"\n\n";
		if (newFunctions.size() != 0)
		{
			for (const SudohFunction& e : newFunctions)
			{
				transpiled += "var f_" + e.name + "(";
				for (int i = 0; i < e.numParams; i++)
				{
					transpiled += std::string("var") + (i < e.numParams - 1 ? ", " : "");
				}
				transpiled += ");\n";
			}

			transpiled += "\n" + transpiledFunctions;
		}
		transpiled += transpiledMain;
		return true;
	}
	catch (SyntaxException& e) // will catch a syntax error and print the error
	{
		size_t token = e.getTokenNum();
		size_t fileCharNum = tokens[token].fileCharNum;

		// get the entire line that the current token is on
		size_t beginLine = fileCharNum, endLine = beginLine;
		while (beginLine != 0 && contents[beginLine - 1] != '\n' && contents[beginLine - 1] != '\t')
		{
			beginLine--;
		}
		while (endLine != contents.length() && contents[endLine] != '\n')
		{
			endLine++;
		}
		std::string line = contents.substr(beginLine, endLine - beginLine);

		// print the error message and indicate parse failure
		std::cout << "Syntax error on line " << tokens[token].lineNum << ": " << e.what() << "\n\t" << line << "\n\t";
		for (int i = 0; i < fileCharNum - beginLine; i++)
		{
			std::cout << " ";
		}
		std::cout << "^\nAborting compilation.\n";
		return false;
	}
}

// adds uncommited transpilation buffer to a transpiled content string after entire
// line has been checked, and clears buffer
void commitLine()
{
	// determine whether to write new line to inside of main or to global scope (for functions)
	std::string& commitTo = inFunction ? transpiledFunctions : transpiledMain;

	// correctly indent new line
	for (int i = 0; i < currStatementScope + !inFunction; i++)
	{
		commitTo += "\t";
	}
	commitTo += uncommittedTrans + "\n";
	uncommittedTrans = "";
}

// assert that the next token is the end of a line, and commit the line
void endOfLine()
{
	const std::string& token = currToken();
	if (token != "\n" && token != END)
	{
		throw SyntaxException("expected end of line", tokenNum);
	}
	commitLine();
}

// parses one single line in the Sudoh code and commits a transpiled version it if is well-formed
// returns false when end token is reached as flag to stop searching for new lines
bool parseNextLine(std::set<bool (*)()>& extraRules)
{
	if (currToken() == END)
	{
		return false;
	}

	// first check if this line is a standalone function call or assignment statement
	if (parseFuncCall() || parseAssignment())
	{
		uncommittedTrans += ";";
		endOfLine();
		return true;
	}

	// check if this line is a structure (loop declaration, if statement, function declaration)
	void (*parseAfter)(int, std::set<bool (*)()>&) = nullptr; // content to parse after structure block e.g. 'else' after 'if' block
	bool (*extraRule)() = nullptr; // extra rule for current structure e.g. allow 'break' in loop
	if (parseStructure(extraRule, parseAfter))
	{
		endOfLine();

		int scope = currStatementScope;
		// if extra parsing rule specified then parse inner block with extra rule
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

		// if extra parse after block specified then do it
		if (parseAfter)
		{
			parseAfter(scope, extraRules);
		}
		return true;
	}

	// if there are any extra rules specified at this point then check them
	for (auto& e : extraRules)
	{
		if (e())
		{
			endOfLine();
			return true;
		}
	}

	// valid lines will have returned from function at this point
	throw SyntaxException("invalid line", tokenNum);
}

// parse a block one scope level higher than current scope
void parseBlock(std::set<bool (*)()>& extraRules)
{
	uncommittedTrans = "{";
	commitLine();

	int blockScope = currStatementScope + 1;
	currStatementScope = nextTokenScope();
	if (currStatementScope < blockScope)
	{
		throw SyntaxException("empty block not allowed", tokenNum);
	}

	// add new scope for new variables to be placed into
	varsInScopeN.push_back(std::set<std::string>());
	injectVarsIntoScope();

	// parse each line in the block
	while (currStatementScope == blockScope && parseNextLine(extraRules))
	{
		currStatementScope = nextTokenScope();
	}

	// block must be exactly one scope higher than initial scope
	if (currStatementScope > blockScope)
	{
		throw SyntaxException("illegal attempt to increase indentation level", tokenNum);
	}

	// special case to add after function definition
	if (inFunction && blockScope == 1)
	{
		uncommittedTrans = "\treturn null;";
		commitLine();
	}

	// add closing curly brace to indicate C++ end of block
	int temp = currStatementScope;
	currStatementScope = blockScope - 1;
	uncommittedTrans = "}";
	commitLine();
	currStatementScope = temp;

	// new scope for variables no longer exists; remove it
	varsInScopeN.pop_back();
}

// return whether a variable of given name exists at current scope
bool varExists(const std::string& name)
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

void addVarToNextScope(const std::string& name)
{
	injectionScope = currStatementScope + 1;
	varsToInject.push_back(name);
}

void injectVarsIntoScope()
{
	if (currStatementScope == injectionScope && !varsToInject.empty())
	{
		for (const std::string& e : varsToInject)
		{
			varsInScopeN[currStatementScope].insert(e);
		}
		varsToInject.clear();
	}
}

// parse a variable identifier and add to variables list (or handle otherwise if needed)
//
// modes:	can be new (normal variable assignment)
//			cannot be new (accessing variable in expression)
//			function parameters - will accept any name; add to next scope
//			'for' loop iteration variable - may exist or may need to add to next scope
//			'for each' loop iteration variable - may not exist; add to next scope
enum struct VarParseMode { mayBeNew, mustExist, functionParam, forVar, forEachVar };
bool parseVarName(VarParseMode mode)
{
	const std::string& name = currToken();

	// do not accept keyword as valid variable name
	if (std::regex_match(name, NAME_RE) && keywords.count(name) == 0)
	{
		switch (mode)
		{
		case VarParseMode::mayBeNew:
			if (varExists(name))
			{
				appendAndAdvance("_" + name);
				break;
			}
			varsInScopeN[currStatementScope].insert(name);
			appendAndAdvance("var _" + name);
			break;
		case VarParseMode::mustExist:
			if (varExists(name))
			{
				appendAndAdvance("_" + name);
				break;
			}
			throw SyntaxException("use of undeclared variable " + name, tokenNum);

		case VarParseMode::forVar:
			if (varExists(name))
			{
				appendAndAdvance("_" + name);
				break;
			}
			addVarToNextScope(name);
			appendAndAdvance("var _" + name);
			break;
		case VarParseMode::forEachVar:
			if (varExists(name))
			{
				throw SyntaxException("'for each' iteration variable must be a new variable", tokenNum);
			}
			addVarToNextScope(name);
			appendAndAdvance("var _" + name);
			break;
		case VarParseMode::functionParam:
			if (std::find(varsToInject.begin(), varsToInject.end(), name) != varsToInject.end())
			{
				throw SyntaxException("function cannot have multiple parameters with same name", tokenNum);
			}
			addVarToNextScope(name);
			appendAndAdvance("var _" + name);
			break;
		}

		return true;
	}
	return false;
}

// parse a single entry in a possibly comma-seperated map
void parseMapEntry()
{
	// map entries must be in the form of <expression> <- <expression>

	uncommittedTrans += "{ ";

	parseExpr();
	if (currToken() != "<-")
	{
		throw SyntaxException("map entry must be of form <key> <- <value>", tokenNum);
	}
	appendAndAdvance(", ");
	parseExpr();

	uncommittedTrans += " }";
}

// extra rules for parsing statements inside of a loop
bool extraParseInsideLoop()
{
	const std::string& t = currToken();
	if (t == "break" || t == "continue")
	{
		appendAndAdvance(t + ";");
		return true;
	}
	return false;
}

// extra rules for parsing statements inside of a function
bool extraParseFunction()
{
	if (currToken() == "return")
	{
		appendAndAdvance("return ");
		if (currToken() != "\n")
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

// extra rules for parsing statements after an 'if' block
void parseAfterIf(int scope, std::set<bool (*)()>& extraRules)
{
	// allow appearances of 'else' or 'else if' blocks; no 'else if' may appear after an 'else'
	bool elseReached = false;
	while (currStatementScope == scope)
	{
		if (currToken() == "else")
		{
			if (elseReached)
			{
				throw SyntaxException("'else' block already reached", tokenNum);
			}

			appendAndAdvance("else");
			// else if [b] then
			//      ^
			if (currToken() == "if")
			{
				appendAndAdvance(" if (");

				// else if [b] then
				//          ^
				parseExpr({ ParsedType::boolean });

				// else if [b] then
				//             ^
				if (currToken() == "then")
				{
					appendAndAdvance(")");

					endOfLine();
					parseBlock(extraRules);
					continue;
				}
				throw SyntaxException("expected 'then'", tokenNum);
			}

			endOfLine();
			elseReached = true;

			parseBlock(extraRules);
			continue;
		}
		return;
	}
}

// extra rules for parsing statement after a 'repeat' block
void parseAfterRepeat(int scope, std::set<bool (*)()>& extraRules)
{
	// 'repeat' is the equivalent of 'do while' in C++
	if (currStatementScope == scope)
	{
		const std::string& token = currToken();
		// either 'repeat ... while <condition>' or 'repeat ... until <condition>' are valid
		if (token == "while" || token == "until")
		{
			appendAndAdvance("while (" + std::string(token == "until" ? "!(" : ""));

			parseExpr({ ParsedType::boolean });
			uncommittedTrans += token == "until" ? "));" : ");";
			endOfLine();
			return;
		}
	}
	throw SyntaxException("expected 'while' or 'until' condition after 'repeat' block", tokenNum);
}

// parse a programming structure such as a loop, if statement, or function declaration and return
// whether a structure was found (and output values to parse function parameters if needed)
bool parseStructure(bool (*&additionalRule)(), void (*&parseAfter)(int, std::set<bool (*)()>&))
{
	const std::string* token = &currToken();

	if (*token == "if")
	{
		appendAndAdvance("if (");

		// if [b] then
		//     ^
		parseExpr({ ParsedType::boolean });

		// if [b] then
		//        ^
		if (currToken() == "then")
		{
			appendAndAdvance(")");
			parseAfter = parseAfterIf;
			return true;
		}
		throw SyntaxException("expected 'then'", tokenNum);
	}

	if (*token == "while" || *token == "until")
	{
		appendAndAdvance("while (" + std::string(*token == "until" ? "!(" : ""));

		// while [b] do
		//        ^
		parseExpr({ ParsedType::boolean });

		// while [b] do
		//           ^
		if (currToken() == "do")
		{
			appendAndAdvance(*token == "until" ? "))" : ")");
			additionalRule = extraParseInsideLoop;
			return true;
		}
		throw SyntaxException("expected 'do'", tokenNum);
	}

	if (*token == "for")
	{
		appendAndAdvance("for (");

		const std::string& forVar = currToken();
		// for i <- [n] (down)? to [n] do
		//     ^
		if (parseVarName(VarParseMode::forVar))
		{
			// for i <- [n] (down)? to [n] do
			//       ^
			if (currToken() == "<-")
			{
				appendAndAdvance(" = ");

				// for i <- [n] (down)? to [n] do
				//           ^
				parseExpr({ ParsedType::number });
				uncommittedTrans += "; _" + forVar;

				// for i <- [n] (down)? to [n] do
				//              ^
				bool down = false;
				if (currToken() == "down")
				{
					down = true;
					tokenNum++;
				}
				if (currToken() == "to")
				{
					appendAndAdvance(down ? " >= " : " <= ");

					// for i <- [n] (down)? to [n] do
					//                          ^
					parseExpr({ ParsedType::number });

					// for i <- [n] (down)? to [n] do
					//                             ^
					if (currToken() == "do")
					{
						appendAndAdvance("; _" + forVar + (down ? " -= 1)" : " += 1)"));
						additionalRule = extraParseInsideLoop;
						return true;
					}
					throw SyntaxException("expected 'do'", tokenNum);
				}
				throw SyntaxException("expected 'to' or 'down to'", tokenNum);
			}
			throw SyntaxException("expected '<-'", tokenNum);
		}

		// for each e in [x] do
		//     ^
		if (currToken() == "each")
		{
			tokenNum++;
			// for each e in [x] do
			//          ^
			if (parseVarName(VarParseMode::forEachVar))
			{
				// for each e in [x] do
				//            ^
				if (currToken() == "in")
				{
					appendAndAdvance(" : ");
					static std::set<ParsedType> valid = {
						ParsedType::string, ParsedType::list,
						ParsedType::map, ParsedType::any
					};

					// for each e in [x] do
					//                ^
					parseExpr({ ParsedType::string, ParsedType::list, ParsedType::map });

					// for each e in [x] do
					//                   ^
					if (currToken() == "do")
					{
						appendAndAdvance(")");
						additionalRule = extraParseInsideLoop;
						return true;
					}
					throw SyntaxException("expected 'do'", tokenNum);
				}
				throw SyntaxException("expected 'in'", tokenNum);
			}
			throw SyntaxException("expected declaration of 'for each' loop iteration variable", tokenNum);
		}
		throw SyntaxException("invalid 'for' loop statement", tokenNum);
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
			throw SyntaxException("nested function illegal", tokenNum);
		}

		appendAndAdvance("var ");
		const std::string& funcName = currToken();

		// function [name]({params})
		//           ^
		if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
		{
			appendAndAdvance("f_" + funcName);

			// function [name]({params})
			//                ^
			if (currToken() == "(")
			{
				appendAndAdvance("(");
				inFunction = true;

				// function [name]({params})
				//                  ^
				int numParams = parseCommaSep([] { parseVarName(VarParseMode::functionParam); }, ")");

				// function [name]({params})
				//                         ^
				if (currToken() == ")")
				{
					appendAndAdvance(")");
					SudohFunction func = { funcName, numParams };
					functionsDefined.insert(func);
					newFunctions.push_back(func);

					additionalRule = extraParseFunction;
					parseAfter = [](auto, auto) { commitLine(); inFunction = false; };
					return true;
				}
				throw SyntaxException("expected closing parenthesis", tokenNum);
			}
			throw SyntaxException("expected argument list after function declaration", tokenNum);
		}
		throw SyntaxException("invalid function name", tokenNum);
	}

	return false;
}

// parse an assignment statement
bool parseAssignment()
{
	const std::string& name = currToken();

	// [var] <- [expr]
	//  ^
	if (std::regex_match(name, NAME_RE) && keywords.count(name) == 0)
	{
		size_t beginTokenNum = tokenNum;
		parseVar(true);

		// [var] <- [expr]
		//       ^
		if (currToken() == "<-")
		{
			// check if assignment is of form [var] <- [var] [operator] [expr] to transpile to C++
			// compound assignment statement e.g. [var] += [expr]
			if (tokenNum + (tokenNum - beginTokenNum) + 1 <= tokens.size() &&
				std::equal(tokens.begin() + beginTokenNum, tokens.begin() + tokenNum, tokens.begin() + tokenNum + 1))
			{
				size_t temp = tokenNum;
				tokenNum += tokenNum - beginTokenNum + 1;
				const std::string& op = currToken();
				// only translate to compound assignment statements for arithmetic operations
				if (op == "+" || op == "-" || op == "*" || op == "/" || op == "mod")
				{
					appendAndAdvance(" " + (op == "mod" ? "%" : op) + "= ");
				}
				else
				{
					tokenNum = temp;
					appendAndAdvance(" = ");
				}
			}
			else
			{
				appendAndAdvance(" = ");
			}

			// [var] <- [expr]
			//           ^
			parseExpr();
			return true;
		}
	}
	return false;
}

// parse a function call
bool parseFuncCall()
{
	const std::string& funcName = currToken();

	// [name]({params})
	//  ^
	if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
	{
		size_t funcCallTokenNum = tokenNum;
		tokenNum++;
		// [name]({params})
		//       ^
		if (currToken() == "(")
		{
			appendAndAdvance("f_" + funcName + "(");

			// [name]({params})
			//         ^
			int numParams = parseCommaSep(parseExpr, ")");

			// [name]({params})
			//                ^
			if (currToken() == ")")
			{
				appendAndAdvance(")");
				functionsUsed.push_back({ { funcName, numParams }, funcCallTokenNum });
				return true;
			}
			throw SyntaxException("expected closing parenthesis for function call", tokenNum);
		}
		tokenNum--;
		return false;
	}
	return false;
}

// parse an expression that indicates a variable
bool parseVar(bool lvalue)
{
	bool exists = varExists(currToken());
	if (parseVarName(lvalue ? VarParseMode::mayBeNew : VarParseMode::mustExist))
	{
		// also accept list, string, or map indexed values as variables
		while (currToken() == "[")
		{
			if (!exists)
			{
				throw SyntaxException("cannot index into undeclared variable", tokenNum);
			}
			// translate to var[x] for attempted assignment and var.at(x) for attempted access
			appendAndAdvance(lvalue ? "[" : ".at(");

			// value inside of brackets must be an expression
			parseExpr();

			if (currToken() == "]")
			{
				appendAndAdvance(lvalue ? "]" : ")");
				continue;
			}
			throw SyntaxException("expected closing bracket", tokenNum);
		}
		return true;
	}
	return false;
}

//  +-------------------------+
//  |   Expressiong parsing   |
//  |   functions             |
//  +-------------------------+

typedef std::map<ParsedType, std::set<ParsedType>> Operations;
static const std::set<ParsedType> ALL = {
		ParsedType::number, ParsedType::boolean, ParsedType::string,
		ParsedType::list, ParsedType::map, ParsedType::null
};
// helper function for parseExprBinary function which checks integrity of binary operation by checking
// if operation between two specified types is allowed
void checkBinary(const std::string translatedBinOp, ParsedType& leftType,
	const Operations& allowedOps, void (*rightFunction)(ParsedType&))
{
	size_t initTokenNum = tokenNum;

	const std::string& sudohBinOp = currToken();
	appendAndAdvance(" " + translatedBinOp + " ");

	maybeMultiline();

	// find type of right term in binary expression and determine whether binary statement of given
	// operation between these two types is legal
	ParsedType rightType;
	rightFunction(rightType);

	if (rightType == ParsedType::any)
	{
		return;
	}
	for (const auto& e : allowedOps)
	{
		if ((leftType == ParsedType::any || leftType == e.first) && e.second.count(rightType) != 0)
		{
			return;
		}
	}

	tokenNum = initTokenNum;
	throw SyntaxException("binary operator '" + sudohBinOp + "' cannot be applied to types '" +
		typeToString(leftType) + "' and '" + typeToString(rightType) + "'", tokenNum);
}

void parseArithmetic(ParsedType& type)
{
	parseTerm(type);
	const std::string* token = &currToken();
	while (*token == "+" || *token == "-" || *token == "*" || *token == "/" || *token == "mod")
	{
		if (*token == "+")
		{
			checkBinary(*token, type, {
				{ ParsedType::number, { ParsedType::number } },
				{ ParsedType::string, ALL },
				{ ParsedType::list, ALL }
				}, parseTerm);
		}
		else
		{
			checkBinary(*token == "mod" ? "%" : *token, type, {
				{ ParsedType::number, { ParsedType::number } }
				}, parseTerm);
		}
		token = &currToken();
	}
}

void parseComparison(ParsedType& type)
{
	parseArithmetic(type);
	const std::string* token = &currToken();
	while (*token == "=" || *token == "!=" || *token == "<" || *token == "<=" || *token == ">" || *token == ">=")
	{
		// idea- need higher precedence for comparison than others (to parse 1+2=2+2 correctly for instance)
		checkBinary(*token == "=" ? "==" : *token, type, {
			{ ParsedType::number, { ParsedType::number } },
			{ ParsedType::boolean, { ParsedType::boolean } },
			{ ParsedType::string, { ParsedType::string } },
			{ ParsedType::list, { ParsedType::list } },
			{ ParsedType::map, { ParsedType::map } }
			}, parseArithmetic);
		type = ParsedType::boolean;
		token = &currToken();
	}
}

// parse binary expression
void parseExpr(ParsedType& type)
{
	parseComparison(type);
	const std::string* token = &currToken();
	while (*token == "and" || *token == "or")
	{
		checkBinary(*token == "and" ? "&&" : "||", type, {
			{ ParsedType::boolean, { ParsedType::boolean } }
			}, parseComparison);
		token = &currToken();
	}
}

void parseExpr()
{
	ParsedType discard;
	parseExpr(discard);
}

// parse expression and throw exception if expression does not match one of allowed types
void parseExpr(const std::vector<ParsedType> allowed)
{
	size_t initTokenNum = tokenNum;

	ParsedType type;
	parseExpr(type);
	if (type != ParsedType::any && std::find(allowed.begin(), allowed.end(), type) == allowed.end())
	{
		std::string msg = "expected expression of type (" + typeToString(allowed[0]);
		for (int i = 1; i < allowed.size(); i++)
		{
			msg += " | " + typeToString(allowed[i]);
		}
		msg += ")";

		tokenNum = initTokenNum;
		throw SyntaxException(msg, tokenNum);
	}
}

// parse a single term in an expression
void parseTerm(ParsedType& t)
{
	const std::string& token = currToken();

	// +-----------------------------------------------------------------------------+
	// |   Check for compound term e.g. parenthesized term or one with unary 'not'   |
	// +-----------------------------------------------------------------------------+
	if (token == "not") // check for unary 'not' expression
	{
		appendAndAdvance("!");
		parseExpr({ ParsedType::boolean });
		uncommittedTrans += ")";
	}
	else if (token == "(") // check for parenthesized expression
	{
		appendAndAdvance("(");
		parseExpr();
		if (currToken() == ")")
		{
			appendAndAdvance(")");
		}
		else
		{
			throw SyntaxException("expected closing parenthesis", tokenNum);
		}
	}
	// +---------------------------------------------------------------+
	// |   Not a compound term at this point; check for normal value   |
	// +---------------------------------------------------------------+
	else if (token == "true" || token == "false") // check for boolean
	{
		appendAndAdvance("var(" + token + ")");
		t = ParsedType::boolean;
	}
	else if (std::regex_match(token, NUMBER_RE)) // check for number
	{
		appendAndAdvance("var(" + token + ")");
		t = ParsedType::number;
	}
	else if (std::regex_match(token, STRING_RE)) // check for string
	{
		appendAndAdvance("var(std::string(" + token + "))");
		t = ParsedType::string;
	}
	else if (token == "null") // check for null value
	{
		appendAndAdvance("null");
		t = ParsedType::null;
	}
	else if (token == "[") // check for list
	{
		appendAndAdvance("var(List{ ");
		maybeMultiline();
		parseCommaSep(parseExpr, "]");

		maybeMultiline();
		if (currToken() == "]")
		{
			appendAndAdvance(" })");
			t = ParsedType::list;
		}
		else
		{
			throw SyntaxException("expected closing ']'", tokenNum);
		}
	}
	else if (token == "{") // check for map
	{
		appendAndAdvance("var(Map{ ");
		maybeMultiline();
		parseCommaSep(parseMapEntry, "}");
		maybeMultiline();
		if (currToken() == "}")
		{
			appendAndAdvance(" })");
			t = ParsedType::map;
		}
		else
		{
			throw SyntaxException("expected closing '}'", tokenNum);
		}
	}
	else if (parseFuncCall() || parseVar(false)) // check if this is valid variable-type expression indicating 'any' type
	{
		t = ParsedType::any;
	}
	else
	{
		throw SyntaxException("expected expression term", tokenNum);
	}
}

// parses a comma-separated list of items and returns number of items found
int parseCommaSep(void (*parseItem)(), const std::string stop)
{
	int numItems = 0;
	
	if (currToken() == stop)
	{
		return numItems;
	}
	parseItem();
	numItems++;

	// loop to handle further comma-separated items
	while (currToken() == ",")
	{
		appendAndAdvance(", ");
		maybeMultiline();

		if (currToken() == stop)
		{
			return numItems;
		}
		parseItem();
		numItems++;
	}

	return numItems;
}
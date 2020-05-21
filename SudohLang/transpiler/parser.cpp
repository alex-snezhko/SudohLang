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
int tokenNum;

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

// forward declaration of functions when needed
void commitLine();
bool parseNextLine(std::set<bool (*)()>&);
void parseBlock(std::set<bool (*)()>&);
bool parseStructure(bool (*&)(), void (*&)(int, std::set<bool (*)()>&));
bool parseAssignment();
bool parseFuncCall();
bool parseVar(bool);
bool parseExpr(ParsedType&);
bool parseExpr();
void parseExpr(std::vector<ParsedType>);
bool parseTerm(ParsedType&);
void parseExprBinary(ParsedType&);
int parseCommaSep(bool (*)());

// returns string of the token that the parser is currently on
const std::string& nextToken()
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
	// count end of input as being is scope 0
	if (*token == END)
	{
		return 0;
	}
	return scope;
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
		tokens = tokenize(contents);
		tokenNum = 0;
		inFunction = false;

		if (nextTokenScope() != 0)
		{
			throw SyntaxException("base scope must not be indented");
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

		// combine all transpiled content;
		// structure - #include, function forward declarations, function definitions, main
		transpiled = "#include \"sudoh.h\"\n\n";
		if (newFunctions.size() != 0)
		{
			for (const SudohFunction& e : newFunctions)
			{
				transpiled += "Variable _" + e.name + "(";
				for (int i = 0; i < e.numParams; i++)
				{
					transpiled += std::string("Variable") + (i < e.numParams - 1 ? ", " : "");
				}
				transpiled += ");\n";
			}

			transpiled += "\n" + transpiledFunctions;
		}
		transpiled += transpiledMain;
		return true;
	}
	// will catch a syntax error and print the error
	catch (SyntaxException& e)
	{
		size_t fileCharIdx = tokens[tokenNum].fileCharNum;

		// get the entire line that the current token is on
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

		// print the error message and indicate parse failure
		std::cout << "Syntax error on line " << tokens[tokenNum].lineNum << ": " << e.what() << "\n\t" << line << "\n\t";
		for (int i = 0; i < fileCharIdx - beginLine; i++)
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
	const std::string& token = nextToken();
	if (token != "\n" && token != END)
	{
		throw SyntaxException("expected end of line");
	}
	commitLine();
}

// parses one single line in the Sudoh code and commits a transpiled version it if is well-formed
// returns false when end token is reached as flag to stop searching for new lines
bool parseNextLine(std::set<bool (*)()>& extraRules)
{
	if (nextToken() == END)
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
	throw SyntaxException("invalid line");
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
		throw SyntaxException("empty block not allowed");
	}

	// add new scope for new variables to be placed into
	varsInScopeN.push_back(std::set<std::string>());

	// parse each line in the block
	while (currStatementScope == blockScope && parseNextLine(extraRules))
	{
		currStatementScope = nextTokenScope();
	}

	// special case to add after function definition
	if (inFunction && blockScope == 1)
	{
		uncommittedTrans = "\treturn null;";
		commitLine();
	}

	// block must be exactly one scope higher than initial scope
	if (currStatementScope > blockScope)
	{
		throw SyntaxException("illegal attempt to increase indentation level");
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
	for (const std::set<std::string>& e : varsInScopeN)
	{
		if (e.count(name) != 0)
		{
			return true;
		}
	}
	return false;
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
			injectionScope = currStatementScope + 1;
			varsToInject.push_back(name);
			appendAndAdvance("Variable _" + name);
			break;
		case VarParseMode::functionParam:
			if (std::find(varsToInject.begin(), varsToInject.end(), name) != varsToInject.end())
			{
				throw SyntaxException("function cannot have multiple parameters with same name");
			}
			injectionScope = currStatementScope + 1;
			varsToInject.push_back(name);
			appendAndAdvance("Variable _" + name);
			break;
		}
		return true;
	}
	return false;
}

// parse a single entry in a possibly comma-seperated map
bool parseMapEntry()
{
	// map entries must be in the form of <expression> <- <expression>

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

// extra rules for parsing statements inside of a loop
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

// extra rules for parsing statements inside of a function
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

// extra rules for parsing statements after an 'if' block
void parseAfterIf(int scope, std::set<bool (*)()>& extraRules)
{
	// allow appearances of 'else' or 'else if' blocks; no 'else if' may appear after an 'else'
	bool elseReached = false;
	while (currStatementScope == scope)
	{
		if (nextToken() == "else")
		{
			appendAndAdvance("else");
			// else if [b] then
			//      ^
			if (nextToken() == "if")
			{
				if (elseReached)
				{
					throw SyntaxException("'else' block cannot be followed by 'else if' block");
				}

				appendAndAdvance(" if (");

				// else if [b] then
				//          ^
				parseExpr({ ParsedType::boolean });

				// else if [b] then
				//             ^
				if (nextToken() == "then")
				{
					appendAndAdvance(")");

					endOfLine();
					parseBlock(extraRules);
					continue;
				}
				throw SyntaxException("expected 'then'");
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

// extra rules for parsing statement after a 'repeat' block
void parseAfterRepeat(int scope, std::set<bool (*)()>& extraRules)
{
	// 'repeat' is the equivalent of 'do while' in C++
	if (currStatementScope == scope)
	{
		const std::string& token = nextToken();
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
	throw SyntaxException("expected 'while' or 'until' condition after 'repeat' block");
}

// parse a programming structure such as a loop, if statement, or function declaration and return
// whether a structure was found (and output values to parse function parameters if needed)
bool parseStructure(bool (*&additionalRule)(), void (*&parseAfter)(int, std::set<bool (*)()>&))
{
	const std::string* token = &nextToken();

	if (*token == "if")
	{
		appendAndAdvance("if (");

		// if [b] then
		//     ^
		parseExpr({ ParsedType::boolean });

		// if [b] then
		//        ^
		if (nextToken() == "then")
		{
			appendAndAdvance(")");
			parseAfter = parseAfterIf;
			return true;
		}
		throw SyntaxException("expected 'then'");
	}

	if (*token == "while" || *token == "until")
	{
		appendAndAdvance("while (" + std::string(*token == "until" ? "!(" : ""));
		
		// while [b] do
		//        ^
		parseExpr({ ParsedType::boolean });

		// while [b] do
		//           ^
		if (nextToken() == "do")
		{
			appendAndAdvance(*token == "until" ? "))" : ")");
			additionalRule = extraParseInsideLoop;
			return true;
		}
		throw SyntaxException("expected 'do'");
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
				parseExpr({ ParsedType::number });
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
					parseExpr({ ParsedType::number });

					// for i <- [n] (down)? to [n] do
					//                             ^
					if (nextToken() == "do")
					{
						appendAndAdvance("; _" + forVar + (down ? " -= 1)" : " += 1)"));
						additionalRule = extraParseInsideLoop;
						return true;
					}
					throw SyntaxException("expected 'do'");
				}
				throw SyntaxException("expected 'to' or 'down to'");
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
					parseExpr({ ParsedType::string, ParsedType::list, ParsedType::map });

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

		// function [name]({params})
		//           ^
		if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
		{
			appendAndAdvance("_" + funcName);

			// function [name]({params})
			//                ^
			if (nextToken() == "(")
			{
				appendAndAdvance("(");
				inFunction = true;

				// function [name]({params})
				//                  ^
				int numParams = parseCommaSep([] { return parseVarName(VarParseMode::functionParam); });

				// function [name]({params})
				//                         ^
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

// parse an assignment statement
bool parseAssignment()
{
	const std::string& name = nextToken();

	// [var] <- [expr]
	//  ^
	if (std::regex_match(name, NAME_RE) && keywords.count(name) == 0)
	{
		int beginTokenNum = tokenNum;
		parseVar(true);

		// [var] <- [expr]
		//       ^
		if (nextToken() == "<-")
		{
			// check if assignment is of form [var] <- [var] [operator] [expr] to transpile to C++
			// compound assignment statement e.g. [var] += [expr]
			if (std::equal(tokens.begin() + beginTokenNum, tokens.begin() + tokenNum, tokens.begin() + tokenNum + 1))
			{
				int temp = tokenNum;
				tokenNum += tokenNum - beginTokenNum + 1;
				const std::string& op = nextToken();
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

// parse a function call
bool parseFuncCall()
{
	const std::string& funcName = nextToken();

	// [name]({params})
	//  ^
	if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
	{
		tokenNum++;
		// [name]({params})
		//       ^
		if (nextToken() == "(")
		{
			appendAndAdvance("_" + funcName + "(");

			// [name]({params})
			//         ^
			int numParams = parseCommaSep([] { return parseExpr(); });

			// [name]({params})
			//                ^
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

// parse an expression that indicates a variable
bool parseVar(bool lvalue)
{
	bool exists = varExists(nextToken());
	if (parseVarName(lvalue ? VarParseMode::mayBeNew : VarParseMode::mustExist))
	{
		// also accept list, string, or map indexed values as variables
		while (nextToken() == "[")
		{
			if (!exists)
			{
				throw SyntaxException("cannot get map value of undeclared variable");
			}
			// translate to var[x] for attempted assignment and var.at(x) for attempted access
			appendAndAdvance(lvalue ? "[" : ".at(");

			// value inside of brackets must be an expression
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

// parse expression and output success status as well as expression type
bool parseExpr(ParsedType& t)
{
	if (parseTerm(t))
	{
		parseExprBinary(t);
		return true;
	}
	return false;
}

bool parseExpr()
{
	ParsedType discard;
	return parseExpr(discard);
}

// parse expression and throw exception if expression does not match one of allowed types
void parseExpr(const std::vector<ParsedType> allowed)
{
	int initTokenNum = tokenNum;

	ParsedType type;
	if (!parseExpr(type) || (type != ParsedType::any &&
		std::find(allowed.begin(), allowed.end(), type) == allowed.end()))
	{
		std::string msg = "expected expression of type (" + typeToString(allowed[0]);
		for (int i = 1; i < allowed.size(); i++)
		{
			msg += " | " + typeToString(allowed[i]);
		}
		msg += ")";

		tokenNum = initTokenNum;
		throw SyntaxException(msg);
	}
}

// parse a single term in an expression
bool parseTerm(ParsedType& t)
{
	const std::string& token = nextToken();

	// +-----------------------------------------------------------------------------+
	// |   Check for compound term e.g. parenthesized term or one with unary 'not'   |
	// +-----------------------------------------------------------------------------+

	// check for unary 'not' expression
	if (token == "not")
	{
		appendAndAdvance("!");
		parseExpr({ ParsedType::boolean });
		return true;
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
		throw SyntaxException("expected expression");
	}

	// +---------------------------------------------------------------+
	// |   Not a compound term at this point; check for normal value   |
	// +---------------------------------------------------------------+

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

		// TODO allow multiline map declarations
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

// helper function for parseExprBinary function which checks integrity of binary operation by checking
// if operation between two specified types is allowed
void checkBinary(const std::string translatedBinOp, ParsedType& leftType, const Operations& allowedOps)
{
	const std::string& sudohBinOp = nextToken();
	appendAndAdvance(" " + translatedBinOp + " ");

	if (nextToken() == "\n" && nextTokenScope() <= currStatementScope)
	{
		throw SyntaxException("subsequent lines of multiline arithmetic expression or compound "
			"condition must be indented above level of first line");
	}

	// find type of right term in binary expression and determine whether binary statement of given
	// operation between these two types is legal
	ParsedType rightType;
	if (parseTerm(rightType))
	{
		if (leftType == ParsedType::any || rightType == ParsedType::any)
		{
			parseExprBinary(leftType);
			return;
		}

		for (const auto& e : allowedOps) // TODO will accept something like 'random() mod true' when shouldnt
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

// parse binary expression
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

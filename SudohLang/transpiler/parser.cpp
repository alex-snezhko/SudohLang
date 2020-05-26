#include "parser.h"
#include "syntax_ex.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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

// skips to next relevant token (not whitespace) and returns the scope that it is on
int Parser::skipToNextRelevant()
{
	int scope = currStatementScope;
	
	const std::string* token = &tokens.currToken();
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
		tokens.advance();
		token = &tokens.currToken();
	}
	// special case for beginning of parse when scope is set to -1
	// count end of input as being is scope 0
	if (scope == -1 || *token == TokenIterator::END)
	{
		return 0;
	}
	return scope;
}

// check that following line of multiline statement is indented higher than first line indentation
void Parser::maybeMultiline()
{
	if (tokens.currToken() == "\n" && skipToNextRelevant() <= currStatementScope)
	{
		throw SyntaxException("indentation of following lines of multiline statement "
			"must be greater than indentation of first line");
	}
}

// main function which allows parsing of a Sudoh source file. reads, tokenizes, and begins
// the parse/transpilation of the source file
void Parser::parse(const std::string fileName, bool main)
{
	std::ifstream file;
	file.open(fileName + ".sud");
	if (!file)
	{
		std::cout << "File '" + fileName + ".sud' could not be found. Aborting compilation\n";
		exit(1);
		return;
	}

	// read contents from file into buffer
	std::stringstream contentsStream;
	contentsStream << file.rdbuf();

	// add space at end to help tokenize later
	std::string contents = contentsStream.str() + " ";
	file.close();

	try
	{
		tokens.tokenize(contents);

		// begin transpiled file with #include "<file>"
		trans.includeFile(fileName);

		if (skipToNextRelevant() != 0)
		{
			throw SyntaxException("base scope must not be indented");
		}
		// if there are any included files in this source file, parse them now
		if (tokens.currToken() == "including")
		{
			tokens.advance();
			parseCommaSep(&Parser::parseIncludeFile, "\n");
		}

		// initialize currStatementScope to -1 for parse because global Sudoh code will be treated as a block
		// for parseBlock(), which searches for code in (current scope + 1). This allows search in scope 0
		currStatementScope = -1;
		trans.appendToBuffer("int main()");
		trans.commitLine(inFunction, currStatementScope);

		// parse global block; extraRules will contain a set of extra parse rules allowed when necessary
		// (such as allowing 'break', 'continue' when inside a loop); no extra rules initially
		std::vector<bool (Parser::*)()> extraRules = {};
		parseBlock(extraRules);

		// ensure validity of all function calls attempted
		names.checkFuncCallsValid();
	}
	catch (SyntaxException& e) // will catch a syntax error and print the error
	{
		size_t fileCharNum = tokens.getTokens()[tokens.getTokenNum()].fileCharNum;

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
		std::cout << "Syntax error on line " << tokens.getTokens()[tokens.getTokenNum()].lineNum <<
			" of file '" + fileName + ".sud': " << e.what() << "\n\t" << line << "\n\t";
		for (int i = 0; i < fileCharNum - beginLine; i++)
		{
			std::cout << " ";
		}
		std::cout << "^\nAborting compilation.\n";
		exit(1);
	}

	// gather all functions defined in this .sud file and place them into a header file
	std::string transpiledHeader = "#include \"sudoh.h\"\n\n";
	for (auto& e : names.getFunctionsDefined())
	{
		transpiledHeader += "var f_" + e.name + "(";
		for (int i = 0; i < e.numParams; i++)
		{
			transpiledHeader += std::string("var") + (i < e.numParams - 1 ? ", " : "");
		}
		transpiledHeader += ");\n";
	}

	std::ofstream out;
	out.open(fileName + ".h");
	out << transpiledHeader;
	out.close();

	out.open(fileName + ".cpp");
	out << trans.fullTranspiled(main);
	out.close();
}

// convenience function which advances the token iterator and appends new content to transpiled buffer
void Parser::appendAndAdvance(const std::string append)
{
	tokens.advance();
	trans.appendToBuffer(append);
}

// assert that the next token is the end of a line, and commit the line
void Parser::endOfLine()
{
	const std::string& token = tokens.currToken();
	if (token != "\n" && token != TokenIterator::END)
	{
		throw SyntaxException("expected end of line");
	}
	trans.commitLine(inFunction, currStatementScope);
}

// parses one single line in the Sudoh code and commits a transpiled version it if is well-formed
// returns false when end token is reached as flag to stop searching for new lines
bool Parser::parseNextLine(std::vector<bool(Parser::*)()>& extraRules)
{
	if (tokens.currToken() == TokenIterator::END)
	{
		return false;
	}

	// first check if this line is a standalone function call or assignment statement
	if (parseFuncCall() || parseAssignment())
	{
		trans.appendToBuffer(";");
		endOfLine();
		return true;
	}

	// check if this line is a structure (loop declaration, if statement, function declaration)
	void (Parser::*parseAfter)(int, std::vector<bool (Parser::*)()>&) = nullptr; // content to parse after structure block e.g. 'else' after 'if' block
	bool (Parser::*extraRule)() = nullptr; // extra rule for current structure e.g. allow 'break' in loop
	if (parseStructure(extraRule, parseAfter))
	{
		endOfLine();

		int scope = currStatementScope;
		// if extra parsing rule specified then parse inner block with extra rule
		if (extraRule && std::find(extraRules.begin(), extraRules.end(), extraRule) == extraRules.end())
		{
			extraRules.push_back(extraRule);
			parseBlock(extraRules);
			extraRules.pop_back();
		}
		else
		{
			parseBlock(extraRules);
		}

		// if extra parse after block specified then do it
		if (parseAfter)
		{
			(this->*parseAfter)(scope, extraRules);
		}
		return true;
	}

	// if there are any extra rules specified at this point then check them
	for (auto& e : extraRules)
	{
		if ((this->*e)())
		{
			endOfLine();
			return true;
		}
	}

	// valid lines will have returned from function at this point
	throw SyntaxException("invalid line");
}

// parse a block one scope level higher than current scope
void Parser::parseBlock(std::vector<bool (Parser::*)()>& extraRules)
{
	trans.appendToBuffer("{");
	trans.commitLine(inFunction, currStatementScope);

	int blockScope = currStatementScope + 1;
	currStatementScope = skipToNextRelevant();
	if (currStatementScope < blockScope)
	{
		throw SyntaxException("empty block not allowed");
	}

	names.advanceScope();

	// parse each line in the block
	while (currStatementScope == blockScope && parseNextLine(extraRules))
	{
		currStatementScope = skipToNextRelevant();
	}

	// block must be exactly one scope higher than initial scope
	if (currStatementScope > blockScope)
	{
		throw SyntaxException("illegal attempt to increase indentation level");
	}

	// special case to add after function definition
	if (inFunction && blockScope == 1)
	{
		trans.appendToBuffer("\treturn null;");
		trans.commitLine(inFunction, currStatementScope);
	}

	// add closing curly brace to indicate C++ end of block
	int temp = currStatementScope;
	currStatementScope = blockScope - 1;
	trans.appendToBuffer("}");
	trans.commitLine(inFunction, currStatementScope);
	currStatementScope = temp;

	names.endScope();
}

// parse an assignment statement
bool Parser::parseAssignment()
{
	// [var] <- [expr]
	//  ^
	if (NameManager::validName(tokens.currToken()))
	{
		size_t beginTokenNum = tokens.getTokenNum();
		parseVar(true);

		// [var] <- [expr]
		//       ^
		if (tokens.currToken() == "<-")
		{
			// check if assignment is of form [var] <- [var] [operator] [expr] to transpile to C++
			// compound assignment statement e.g. [var] += [expr]

			// number of tokens in left side of assignment
			size_t num = tokens.getTokenNum() - beginTokenNum;

			auto& tList = tokens.getTokens();
			size_t tNum = tokens.getTokenNum();
			if (tNum + num + 1 <= tList.size() &&
				std::equal(tList.begin() + beginTokenNum, tList.begin() + tNum, tList.begin() + tNum + 1))
			{
				size_t temp = tokens.getTokenNum();
				tokens.setTokenNum(tNum + num + 1);
				const std::string& op = tokens.currToken();
				// only translate to compound assignment statements for arithmetic operations
				if (op == "+" || op == "-" || op == "*" || op == "/" || op == "mod")
				{
					appendAndAdvance(" " + (op == "mod" ? "%" : op) + "= ");
				}
				else
				{
					tokens.setTokenNum(temp);
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
		tokens.setTokenNum(beginTokenNum);
	}
	return false;
}

// parse a function call
bool Parser::parseFuncCall()
{
	const std::string& funcName = tokens.currToken();

	// [name]({params})
	//  ^
	if (NameManager::validName(funcName))
	{
		size_t funcCallTokenNum = tokens.getTokenNum();
		tokens.advance();
		// [name]({params})
		//       ^
		if (tokens.currToken() == "(")
		{
			appendAndAdvance("f_" + funcName + "(");

			// [name]({params})
			//         ^
			int numParams = parseCommaSep(&Parser::parseExpr, ")");

			// [name]({params})
			//                ^
			if (tokens.currToken() == ")")
			{
				appendAndAdvance(")");
				names.addFunctionCall(funcName, numParams, funcCallTokenNum);
				return true;
			}
			throw SyntaxException("expected closing parenthesis for function call");
		}
		tokens.setTokenNum(funcCallTokenNum);
		return false;
	}
	return false;
}

// parse an expression that indicates a variable
bool Parser::parseVar(bool lvalue)
{
	const std::string& name = tokens.currToken();
	if (NameManager::validName(name))
	{
		bool exists = names.varExists(name, inFunction);
		parseVarName(lvalue ? VarParseMode::mayBeNew : VarParseMode::mustExist);

		// also accept list, string, or map indexed values as variables
		while (tokens.currToken() == "[")
		{
			if (!exists)
			{
				throw SyntaxException("cannot index into undeclared variable");
			}
			// translate to var[x] for attempted assignment and var.at(x) for attempted access
			appendAndAdvance(lvalue ? "[" : ".at(");

			// value inside of brackets must be an expression
			parseExpr();

			if (tokens.currToken() == "]")
			{
				appendAndAdvance(lvalue ? "]" : ")");
				continue;
			}
			throw SyntaxException("expected closing bracket");
		}
		return true;
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
bool Parser::parseVarName(VarParseMode mode)
{
	const std::string& name = tokens.currToken();

	// do not accept keyword as valid variable name
	if (NameManager::validName(name))
	{
		switch (mode)
		{
		case VarParseMode::mayBeNew:
			if (names.varExists(name, inFunction))
			{
				appendAndAdvance("_" + name);
				break;
			}
			names.addVar(name);
			appendAndAdvance("var _" + name);
			break;
		case VarParseMode::mustExist:
			if (names.varExists(name, inFunction))
			{
				appendAndAdvance("_" + name);
				break;
			}
			throw SyntaxException("use of undeclared variable " + name);

		case VarParseMode::forVar:
			if (names.varExists(name, inFunction))
			{
				appendAndAdvance("_" + name);
				break;
			}
			names.addVarToNextScope(name);
			appendAndAdvance("var _" + name);
			break;
		case VarParseMode::forEachVar:
			if (names.varExists(name, inFunction))
			{
				throw SyntaxException("'for each' iteration variable must be a new variable");
			}
			names.addVarToNextScope(name);
			appendAndAdvance("var _" + name);
			break;
		case VarParseMode::functionParam:
			names.addVarToNextScope(name);
			appendAndAdvance("var _" + name);
			break;
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
void Parser::checkBinary(const std::string translatedBinOp, ParsedType& leftType,
	const Operations& allowedOps, void (Parser::*rightFunction)(ParsedType&))
{
	size_t initTokenNum = tokens.getTokenNum();

	const std::string& sudohBinOp = tokens.currToken();
	appendAndAdvance(" " + translatedBinOp + " ");

	maybeMultiline();

	// find type of right term in binary expression and determine whether binary statement of given
	// operation between these two types is legal
	ParsedType rightType;
	(this->*rightFunction)(rightType);

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

	tokens.setTokenNum(initTokenNum);
	throw SyntaxException("binary operator '" + sudohBinOp + "' cannot be applied to types '" +
		typeToString(leftType) + "' and '" + typeToString(rightType) + "'");
}

void Parser::parseArithmetic(ParsedType& type)
{
	parseTerm(type);
	const std::string* token = &tokens.currToken();
	while (*token == "+" || *token == "-" || *token == "*" || *token == "/" || *token == "mod")
	{
		if (*token == "+")
		{
			checkBinary(*token, type, {
				{ ParsedType::number, { ParsedType::number } },
				{ ParsedType::string, ALL },
				{ ParsedType::list, ALL },
				{ ParsedType::any, ALL }
				}, &Parser::parseTerm);
		}
		else
		{
			checkBinary(*token == "mod" ? "%" : *token, type, {
				{ ParsedType::number, { ParsedType::number } },
				{ ParsedType::any, { ParsedType::number }}
				}, &Parser::parseTerm);
		}
		token = &tokens.currToken();
	}
}

void Parser::parseComparison(ParsedType& type)
{
	parseArithmetic(type);
	const std::string* token = &tokens.currToken();
	while (*token == "=" || *token == "!=" || *token == "<" || *token == "<=" || *token == ">" || *token == ">=")
	{
		if (*token == "=" || *token == "!=")
		{
			checkBinary(*token == "=" ? "==" : *token, type, {
				{ ParsedType::number, { ParsedType::number, ParsedType::null } },
				{ ParsedType::boolean, { ParsedType::boolean, ParsedType::null } },
				{ ParsedType::string, { ParsedType::string, ParsedType::null } },
				{ ParsedType::list, { ParsedType::list, ParsedType::null } },
				{ ParsedType::map, { ParsedType::map, ParsedType::null } },
				{ ParsedType::any, ALL }
				}, &Parser::parseArithmetic);
		}
		else
		{
			checkBinary(*token, type, {
				{ ParsedType::number, { ParsedType::number, ParsedType::null } },
				{ ParsedType::string, { ParsedType::string, ParsedType::null } },
				{ ParsedType::any, ALL }
				}, &Parser::parseArithmetic);
		}
		
		type = ParsedType::boolean;
		token = &tokens.currToken();
	}
}

// parse binary expression
void Parser::parseExpr(ParsedType& type)
{
	parseComparison(type);
	const std::string* token = &tokens.currToken();
	while (*token == "and" || *token == "or")
	{
		checkBinary(*token == "and" ? "&&" : "||", type, {
			{ ParsedType::boolean, { ParsedType::boolean } },
			{ ParsedType::any, { ParsedType::boolean } }
			}, &Parser::parseComparison);
		token = &tokens.currToken();
	}
}

void Parser::parseExpr()
{
	ParsedType discard;
	parseExpr(discard);
}

// parse expression and throw exception if expression does not match one of allowed types
void Parser::parseExpr(const std::vector<ParsedType> allowed)
{
	size_t initTokenNum = tokens.getTokenNum();

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

		tokens.setTokenNum(initTokenNum);
		throw SyntaxException(msg);
	}
}

// parse a single term in an expression
void Parser::parseTerm(ParsedType& t)
{
	static const std::regex NUMBER_RE = std::regex("-?[0-9]+(\.[0-9]+)?");
	static const std::regex STRING_RE = std::regex("\".*\"");

	const std::string& token = tokens.currToken();

	// +-----------------------------------------------------------------------------+
	// |   Check for compound term e.g. parenthesized term or one with unary 'not'   |
	// +-----------------------------------------------------------------------------+
	if (token == "not") // check for unary 'not' expression
	{
		appendAndAdvance("!");
		parseExpr({ ParsedType::boolean });
		trans.appendToBuffer(")");
	}
	else if (token == "(") // check for parenthesized expression
	{
		appendAndAdvance("(");
		parseExpr(t);
		if (tokens.currToken() == ")")
		{
			appendAndAdvance(")");
		}
		else
		{
			throw SyntaxException("expected closing parenthesis");
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
		parseCommaSep(&Parser::parseExpr, "]");

		maybeMultiline();
		if (tokens.currToken() == "]")
		{
			appendAndAdvance(" })");
			t = ParsedType::list;
		}
		else
		{
			throw SyntaxException("expected closing ']'");
		}
	}
	else if (token == "{") // check for map
	{
		appendAndAdvance("var(Map{ ");
		maybeMultiline();
		parseCommaSep(&Parser::parseMapEntry, "}");
		maybeMultiline();
		if (tokens.currToken() == "}")
		{
			appendAndAdvance(" })");
			t = ParsedType::map;
		}
		else
		{
			throw SyntaxException("expected closing '}'");
		}
	}
	else if (parseFuncCall() || parseVar(false)) // check if this is valid variable-type expression indicating 'any' type
	{
		t = ParsedType::any;
	}
	else
	{
		throw SyntaxException("expected expression term");
	}
}

// +----------------------------------------------+
// |   Functions relevant to parsing structures   |
// |   (if statements, loops, functions)          |
// +----------------------------------------------+

// extra rules for parsing statements inside of a loop
bool Parser::extraParseInsideLoop()
{
	const std::string& t = tokens.currToken();
	if (t == "break" || t == "continue")
	{
		appendAndAdvance(t + ";");
		return true;
	}
	return false;
}

// extra rules for parsing statements inside of a function
bool Parser::extraParseInsideFunction()
{
	if (tokens.currToken() == "return")
	{
		appendAndAdvance("return ");
		if (tokens.currToken() != "\n")
		{
			parseExpr();
		}
		else
		{
			trans.appendToBuffer("null");
		}
		trans.appendToBuffer(";");
		return true;
	}
	return false;
}

// extra rules for parsing statements after an 'if' block
void Parser::parseAfterIf(int scope, std::vector<bool (Parser::*)()>& extraRules)
{
	// allow appearances of 'else' or 'else if' blocks; no 'else if' may appear after an 'else'
	bool elseReached = false;
	while (currStatementScope == scope)
	{
		if (tokens.currToken() == "else")
		{
			if (elseReached)
			{
				throw SyntaxException("'else' block already reached");
			}

			appendAndAdvance("else");
			// else if [b] then
			//      ^
			if (tokens.currToken() == "if")
			{
				appendAndAdvance(" if (");

				// else if [b] then
				//          ^
				parseExpr({ ParsedType::boolean });

				// else if [b] then
				//             ^
				if (tokens.currToken() == "then")
				{
					appendAndAdvance(")");

					endOfLine();
					parseBlock(extraRules);
					continue;
				}
				throw SyntaxException("expected 'then'");
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
void Parser::parseAfterRepeat(int scope, std::vector<bool (Parser::*)()>& extraRules)
{
	// 'repeat' is the equivalent of 'do while' in C++
	if (currStatementScope == scope)
	{
		const std::string& token = tokens.currToken();
		// either 'repeat ... while <condition>' or 'repeat ... until <condition>' are valid
		if (token == "while" || token == "until")
		{
			appendAndAdvance("while (" + std::string(token == "until" ? "!(" : ""));

			parseExpr({ ParsedType::boolean });
			trans.appendToBuffer(token == "until" ? "));" : ");");
			endOfLine();
			return;
		}
	}
	throw SyntaxException("expected 'while' or 'until' condition after 'repeat' block");
}

void Parser::afterFunction(int scope, std::vector<bool(Parser::*)()>& extraRules)
{
	// add empty line to separate functions
	trans.commitLine(inFunction, currStatementScope);
	inFunction = false;
}

// parse a programming structure such as a loop, if statement, or function declaration and return
// whether a structure was found (and output values to parse function parameters if needed)
bool Parser::parseStructure(bool (Parser::*& additionalRule)(), void (Parser::*& parseAfter)(int, std::vector<bool (Parser::*)()>&))
{
	const std::string* token = &tokens.currToken();

	if (*token == "if")
	{
		appendAndAdvance("if (");

		// if [b] then
		//     ^
		parseExpr({ ParsedType::boolean });

		// if [b] then
		//        ^
		if (tokens.currToken() == "then")
		{
			appendAndAdvance(")");
			parseAfter = &Parser::parseAfterIf;
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
		if (tokens.currToken() == "do")
		{
			appendAndAdvance(*token == "until" ? "))" : ")");
			additionalRule = &Parser::extraParseInsideLoop;
			return true;
		}
		throw SyntaxException("expected 'do'");
	}

	if (*token == "for")
	{
		appendAndAdvance("for (");

		// for i <- [n] (down)? to [n] do
		//     ^
		const std::string& forVar = tokens.currToken();
		if (NameManager::validName(forVar))
		{
			parseVarName(VarParseMode::forVar);

			// for i <- [n] (down)? to [n] do
			//       ^
			if (tokens.currToken() == "<-")
			{
				appendAndAdvance(" = ");

				// for i <- [n] (down)? to [n] do
				//           ^
				parseExpr({ ParsedType::number });
				trans.appendToBuffer("; _" + forVar);

				// for i <- [n] (down)? to [n] do
				//              ^
				bool down = false;
				if (tokens.currToken() == "down")
				{
					down = true;
					tokens.advance();
				}
				if (tokens.currToken() == "to")
				{
					appendAndAdvance(down ? " >= " : " <= ");

					// for i <- [n] (down)? to [n] do
					//                          ^
					parseExpr({ ParsedType::number });

					// for i <- [n] (down)? to [n] do
					//                             ^
					if (tokens.currToken() == "do")
					{
						appendAndAdvance("; _" + forVar + (down ? " -= 1)" : " += 1)"));
						additionalRule = &Parser::extraParseInsideLoop;
						return true;
					}
					throw SyntaxException("expected 'do'");
				}
				throw SyntaxException("expected 'to' or 'down to'");
			}
			throw SyntaxException("expected '<-'");
		}

		// for each e in [x] do
		//     ^
		if (tokens.currToken() == "each")
		{
			tokens.advance();
			// for each e in [x] do
			//          ^
			if (NameManager::validName(tokens.currToken()))
			{
				parseVarName(VarParseMode::forEachVar);

				// for each e in [x] do
				//            ^
				if (tokens.currToken() == "in")
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
					if (tokens.currToken() == "do")
					{
						appendAndAdvance(")");
						additionalRule = &Parser::extraParseInsideLoop;
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
		additionalRule = &Parser::extraParseInsideLoop;
		parseAfter = &Parser::parseAfterRepeat;
		return true;
	}

	if (*token == "function")
	{
		if (inFunction)
		{
			throw SyntaxException("nested function illegal");
		}

		appendAndAdvance("var ");
		const std::string& funcName = tokens.currToken();

		// function [name]({params})
		//           ^
		if (NameManager::validName(funcName))
		{
			appendAndAdvance("f_" + funcName);

			// function [name]({params})
			//                ^
			if (tokens.currToken() == "(")
			{
				appendAndAdvance("(");
				inFunction = true;

				// function [name]({params})
				//                  ^
				int numParams = parseCommaSep(&Parser::parseFunctionParameter, ")");

				// function [name]({params})
				//                         ^
				if (tokens.currToken() == ")")
				{
					appendAndAdvance(")");
					names.addFunction(funcName, numParams);

					additionalRule = &Parser::extraParseInsideFunction;
					parseAfter = &Parser::afterFunction;
					return true;
				}
				throw SyntaxException("expected closing parenthesis");
			}
			throw SyntaxException("expected argument list after function declaration");
		}
		throw SyntaxException("invalid function name");
	}

	return false;
}

// +----------------------------------------------+
// |   Helper functions for parsing elements of   |
// |   possible comma-separated list              |
// +----------------------------------------------+

void Parser::parseFunctionParameter()
{
	parseVarName(VarParseMode::functionParam);
}

void Parser::parseIncludeFile()
{
	const std::string& inclFileName = tokens.currToken();
	if (!std::regex_match(inclFileName, NameManager::NAME_RE))
	{
		throw SyntaxException("invalid file name");
	}

	Parser p;
	p.parse(inclFileName, false);
	auto& funcs = p.names.getFunctionsDefined();
	for (auto& e : funcs)
	{
		names.addFunction(e.name, e.numParams);
	}

	trans.includeFile(inclFileName);
	tokens.advance();
}

// parse a single entry in a possibly comma-seperated map
void Parser::parseMapEntry()
{
	// map entries must be in the form of <expression> <- <expression>

	trans.appendToBuffer("{ ");

	parseExpr();
	if (tokens.currToken() != "<-")
	{
		throw SyntaxException("map entry must be of form <key> <- <value>");
	}
	appendAndAdvance(", ");
	parseExpr();

	trans.appendToBuffer(" }");
}

// parses a comma-separated list of items and returns number of items found
int Parser::parseCommaSep(void (Parser::*parseItem)(), const std::string stop)
{
	int numItems = 0;

	if (tokens.currToken() == stop)
	{
		return numItems;
	}
	(this->*parseItem)();
	numItems++;

	// loop to handle further comma-separated items
	while (tokens.currToken() == ",")
	{
		appendAndAdvance(", ");
		maybeMultiline();

		(this->*parseItem)();
		numItems++;
	}

	return numItems;
}

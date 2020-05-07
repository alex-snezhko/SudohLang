#pragma once
#include <string>
#include <vector>
#include <regex>
#include <set>

enum class ParsedType { number, boolean, string, list, map, null, any };

class ParseException : public std::exception
{
	std::string msg;
public:
	ParseException(std::string str) : msg(str) {}
	const char* what() const throw()
	{
		return msg.c_str();
	}
};

const std::set<std::string> keywords = {
	"if", "then", "else", "do", "not", "true", "false", "repeat",
	"while", "for", "<-", "return", "break", "continue", "mod" // TODO
};

const std::regex NAME_RE = std::regex("[_a-zA-Z][_a-zA-Z0-9]*");
const std::regex NUMBER_RE = std::regex("[0-9]+(\.[0-9]+)?");
const std::regex STRING_RE = std::regex("\".*\"");

std::set<std::string> allVarsInScope;
std::vector<std::set<std::string>> varsInScopeN;
std::set<std::string> functionNames;
size_t currScopeLevel;

std::vector<std::string> tokens;
size_t tokenNum;
ParsedType discard;

bool acceptVarName(const std::string& name, bool mustExist = false)
{
	if (std::regex_match(name, NAME_RE))
	{
		if (keywords.count(name) != 0)
		{
			return false;
		}

		bool exists = false;
		if (allVarsInScope.count(name) != 0)
		{
			exists = true;
		}

		if (!exists)
		{
			if (mustExist)
			{
				throw ParseException("use of undeclared variable " + name);
			}
			varsInScopeN[currScopeLevel].insert(name);
			allVarsInScope.insert(name);
		}

		return true;
	}
	return false;
}

bool parseNextLine(std::set<bool (*)()>&);
void parseNestedBlock(std::set<bool (*)()>&);
bool parseStructure(void (*&)(size_t, std::set<bool (*)()>&), bool (*&)());
bool parseFuncDef();
bool parseAssignment();
bool parseLvalue();
bool parseVar();
bool parseExpr(ParsedType& t = discard);
bool parseTerm(ParsedType& t);
void parseExprBinary(const ParsedType& left);
bool parseVal(ParsedType& t);
void parseCommaSep(bool (*checkFunc)());
void skipToNextLine();

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
	default:
		return "null";
	}
}

const std::string END = "";
const std::string& nextToken(bool advance = true, bool skipWhitespace = true)
{
	size_t init = tokenNum;
	if (skipWhitespace)
	{
		while (tokenNum < tokens.size() && (tokens[tokenNum] == "\n" || tokens[tokenNum] == "\t"))
		{
			tokenNum++;
		}
	}
	if (tokenNum == tokens.size())
	{
		return END;
	}

	const std::string& ret = tokens[tokenNum];
	tokenNum = advance ? tokenNum + 1 : init;
	return ret;
}

// returns scope level of next line
void skipToNextLine()
{
	const std::string* token = &nextToken(false, false);

	if (*token == "\n")
	{
		tokenNum++;
		size_t scope;
		do
		{
			scope = 0;

			token = &nextToken(true, false);
			while (*token == "\t")
			{
				scope++;
				token = &nextToken(true, false);
			}

		} while (*token == "\n" || token->substr(0, 2) == "//");
		tokenNum--;

		currScopeLevel = scope;
	}
}

//  +-----------------------+
//  |   Parsing functions   |
//  +-----------------------+

bool parse(std::vector<std::string> vec)
{
	tokens = vec;
	tokenNum = 0;
	try
	{
		skipToNextLine();

		varsInScopeN.push_back(std::set<std::string>());

		std::set<bool (*)()> extraRules = { parseFuncDef };
		while (parseNextLine(extraRules)) {}

		allVarsInScope.erase(varsInScopeN[0].begin(), varsInScopeN[0].end());
		varsInScopeN.pop_back();
	}
	catch (ParseException& e)
	{
		std::cout << e.what();
		return false;
	}

	return true;
}

void assertEndOfLine()
{
	const std::string& token = nextToken(false, false);
	if (token != "\n" && token != END)
	{
		throw ParseException("each statement/program element must be on a new line");
	}
}

bool parseNextLine(std::set<bool (*)()>& extraRules)
{
	const std::string* token = &nextToken(false, false);
	if (*token == END)
	{
		return false;
	}

	if (parseAssignment())
	{
		assertEndOfLine();

		size_t formerScope = currScopeLevel;
		skipToNextLine();

		if (currScopeLevel > formerScope)
		{
			throw ParseException("illegal attempt to increase indentation level");
		}
		return true;
	}

	void (*parseAfter)(size_t, std::set<bool (*)()>&) = nullptr;
	bool (*extraRule)() = nullptr;
	if (parseStructure(parseAfter, extraRule))
	{
		size_t scope = currScopeLevel;
		if (extraRule && extraRules.count(extraRule) == 0)
		{
			extraRules.insert(extraRule);
			parseNestedBlock(extraRules);
			extraRules.erase(extraRule);
		}
		else
		{
			parseNestedBlock(extraRules);
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
			return true;
		}
	}

	throw ParseException("invalid line");
}



void parseNestedBlock(std::set<bool (*)()>& extraRules)
{
	size_t blockScope = currScopeLevel + 1;
	skipToNextLine();
	if (currScopeLevel < blockScope)
	{
		throw ParseException("empty block not allowed");
	}
	if (currScopeLevel > blockScope)
	{
		throw ParseException("block may not be indented more than one level");
	}

	varsInScopeN.push_back(std::set<std::string>());

	while (currScopeLevel == blockScope && parseNextLine(extraRules)) {} // TODO will not get statements after indented block

	auto& curr = varsInScopeN.back();
	allVarsInScope.erase(curr.begin(), curr.end());
	varsInScopeN.pop_back();
}

void parseAfterIf(size_t scope, std::set<bool (*)()>& extraRules)
{
	bool elseReached = false;
	while (currScopeLevel == scope)
	{
		if (nextToken(false) == "else")
		{
			tokenNum++;
			if (nextToken(false) == "if")
			{
				tokenNum++;
				if (elseReached)
				{
					throw ParseException("'else' block cannot be followed by 'else if' block");
				}
				parseNestedBlock(extraRules);
				continue;
			}
			if (elseReached)
			{
				throw ParseException("multiple 'else' blocks not allowed");
			}
			elseReached = true;
			parseNestedBlock(extraRules);
			continue;
		}
		return;
	}
}

bool extraParseInsideLoop()
{
	const std::string& t = nextToken(false);
	if (t == "break" || t == "continue")
	{
		tokenNum++;
		return true;
	}
	return false;
}

bool parseStructure(void (*&parseAfter)(size_t, std::set<bool (*)()>&), bool (*&additionalRule)())
{
	size_t init = tokenNum;
	const std::string* token = &nextToken();
	if (*token == "if")
	{
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken() == "then")
			{
				size_t ifScope = currScopeLevel;

				parseAfter = parseAfterIf;
				return true;
			}
			throw ParseException("expected 'then'");
		}
		throw ParseException("expected condition");
	}

	if (*token == "while" || *token == "until")
	{
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken() == "do")
			{
				additionalRule = extraParseInsideLoop;
				return true;
			}
			throw ParseException("expected 'do'");
		}
		throw ParseException("expected condition");
	}

	if (*token == "for")
	{
		// for i <- n1 (down)? to n2 do
		//     ^
		if (acceptVarName(nextToken()))
		{
			// for i <- n1 (down)? to n2 do
			//       ^
			if (nextToken() == "<-")
			{
				// for i <- n1 (down)? to n2 do
				//          ^
				ParsedType type;
				if (parseExpr(type) && (type == ParsedType::number || type == ParsedType::any))
				{
					// for i <- n1 (down)? to n2 do
					//              ^
					token = &nextToken();
					if (*token == "to" || (*token == "down" && nextToken() == "to"))
					{
						// for i <- n1 (down)? to n2 do
						//                        ^
						if (parseExpr(type) && (type == ParsedType::number || type == ParsedType::any))
						{
							// for i <- n1 (down)? to n2 do
							//                           ^
							if (nextToken() == "do")
							{
								additionalRule = extraParseInsideLoop;
								return true;
							}
							throw ParseException("expected 'do'");
						}
						throw ParseException("final value of 'for' loop should be numeric expression");
					}
					throw ParseException("expected 'to' or 'down to'");
				}
				throw ParseException("value of 'for' loop variable should be numeric expression");
			}
			throw ParseException("expected initial value assignment to variable in 'for'");
		}
		throw ParseException("expected variable name after 'for'");
	}

	if (*token == "repeat")
	{

	}

	tokenNum = init;
	return false;
}

bool parseFuncDef()
{
	if (nextToken(false) == "function")
	{
		tokenNum++;
		if (acceptVarName(nextToken()))
		{
			if (nextToken() == "(")
			{
				parseCommaSep([] { return acceptVarName(nextToken()); });

				if (nextToken() == ")")
				{
					return true;
				}
				throw ParseException("closing parenthesis expected");
			}
			throw ParseException("expected argument list after function declaration");
		}
		throw ParseException("invalid function name");
	}
	return false;
}

bool parseAssignment()
{
	size_t init = tokenNum;
	if (parseLvalue())
	{
		if (nextToken() == "<-")
		{
			if (parseExpr())
			{
				return true;
			}
			throw ParseException("expected expression");
		}
		throw ParseException("invalid assignment expression");
	}

	return tokenNum = init, false;
}

bool parseLvalue()
{
	size_t init = tokenNum;
	if (acceptVarName(nextToken()))
	{
		while (nextToken(false) == "[")
		{
			tokenNum++;
			if (parseExpr())
			{
				if (nextToken() == "]")
				{
					continue;
				}
				throw ParseException("expected closing bracket");
			}
			throw ParseException("expected expression inside bracket");
		}

		return true;
	}

	return tokenNum = init, false;
}

bool parseVar()
{
	size_t init = tokenNum;
	const std::string* token = &nextToken();
	if (acceptVarName(*token, true))
	{
		token = &nextToken(false);
		if (*token == "(")
		{
			tokenNum++;
			parseCommaSep([] { return parseExpr(); });

			if (nextToken() == ")")
			{
				return true;
			}
			throw ParseException("expected closing parenthesis");
		}

		while (*token == "[")
		{
			tokenNum++;
			if (parseExpr())
			{
				if (nextToken() == "]")
				{
					continue;
				}
				throw ParseException("expected closing bracket");
			}
			throw ParseException("expected expression inside bracket");
		}

		return true;
	}

	return tokenNum = init, false;
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
	size_t init = tokenNum;
	ParsedType type;
	const std::string& token = nextToken();

	// check for unary prefix operators such as -, not
	if (token == "-")
	{
		if (parseExpr(type))
		{
			if (type == ParsedType::number || type == ParsedType::any)
			{
				return t = type, true;
			}
			throw ParseException("cannot apply '-' unary operator to non-numeric item");
		}
		throw ParseException("expected expression after '-'");
	}
	if (token == "not")
	{
		if (parseExpr(type))
		{
			if (type == ParsedType::boolean || type == ParsedType::any)
			{
				return t = type, true;
			}
			throw ParseException("cannot apply 'not' operator to non-boolean item");
		}
		throw ParseException("expected expression after 'not'");
	}

	// check for parenthesized expression
	if (token == "(")
	{
		if (parseExpr(type))
		{
			if (nextToken() == ")")
			{
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
	size_t init = tokenNum;
	const std::string& token = nextToken();

	// check for boolean
	if (token == "true" || token == "false")
	{
		return t = ParsedType::boolean, true;
	}

	// check for number
	if (std::regex_match(token, NUMBER_RE))
	{
		return t = ParsedType::number, true;
	}

	// check for string
	if (std::regex_match(token, STRING_RE))
	{
		return t = ParsedType::string, true;
	}

	// check for list
	if (token == "[")
	{
		parseCommaSep([] { return parseExpr(); });
		if (nextToken() == "]")
		{
			return t = ParsedType::list, true;
		}
		throw ParseException("expected end of list");
	}

	// check for map
	if (token == "{")
	{
		// flag to keep track of whether another entry is needed (after comma)
		bool lastComma = false;

		parseCommaSep(
			[]
			{
				if (!parseExpr())
				{
					throw ParseException("expected expression as map key");
				}
				if (nextToken() != "<-")
				{
					throw ParseException("map entry must be of form <key> <- <value>");
				}
				if (!parseExpr())
				{
					throw ParseException("expected expression as map value");
				}
				return true;
			}
		);

		if (nextToken() == "}")
		{
			return t = ParsedType::map, true;
		}
		throw ParseException("expected closing curly brace");
	}

	// check for null value
	if (token == "null")
	{
		return t = ParsedType::null, true;
	}

	// check if this is valid variable-type expression indicating 'any' type
	if (tokenNum = init, parseVar())
	{
		return t = ParsedType::any, true;
	}

	// failed all checks, this is not a value
	return tokenNum = init, false;
}

void parseExprBinary(const ParsedType& leftType)
{
	size_t init = tokenNum;
	const std::string& token = nextToken();
	ParsedType rightType;
	if (token == "+")
	{
		if (!parseTerm(rightType)) { tokenNum = init; return; }

		switch (leftType)
		{
		case ParsedType::number:
			if (rightType == ParsedType::number || rightType == ParsedType::any)
			{
				parseExprBinary(leftType);
				return;
			}
			throw ParseException("'+' operator cannot be applied to number and " + typeToString(rightType));
		case ParsedType::any:
		case ParsedType::string:
		case ParsedType::list:
			parseExprBinary(leftType);
			return;
		default:
			throw ParseException("'+' operator cannot be applied to " + typeToString(leftType));
		}
	}
	else if (token == "-" || token == "*" || token == "/" || token == "mod")
	{
		if (!parseTerm(rightType)) { tokenNum = init; return; }

		if ((leftType == ParsedType::number || leftType == ParsedType::any) &&
			(rightType == ParsedType::number || rightType == ParsedType::any))
		{
			parseExprBinary(leftType);
			return;
		}
		throw ParseException("'" + token + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}
	else if (token == "and" || token == "or")
	{
		if (!parseTerm(rightType)) { tokenNum = init; return; }

		if ((leftType == ParsedType::boolean || leftType == ParsedType::any) &&
			(rightType == ParsedType::boolean || rightType == ParsedType::any))
		{
			parseExprBinary(leftType);
			return;
		}
		throw ParseException("'" + token + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}
	else if (token == "is")
	{
		if (!parseTerm(rightType)) { tokenNum = init; return; }

		if (nextToken(false) == "not")
		{
			tokenNum++;
		}

		if ((leftType == ParsedType::any || rightType == ParsedType::any) ||
			(leftType == rightType))
		{
			parseExprBinary(leftType);
			return;
		}
		throw ParseException("'" + token + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}

	// epsilon also accepted
	tokenNum = init;
	return;
}

void parseCommaSep(bool (*checkFunc)())
{
	size_t init = tokenNum;
	if (checkFunc())
	{
		// loop to handle further comma-separated expressions
		while (true)
		{
			if (nextToken(false) == ",")
			{
				tokenNum++;
				if (checkFunc())
				{
					continue;
				}
				throw ParseException("expected item after comma");
			}
			break;
		}
	}
}

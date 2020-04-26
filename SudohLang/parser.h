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
		bool exists = false;
		if (allVarsInScope.find(name) != allVarsInScope.end())
		{
			exists = true;
		}

		if (mustExist)
		{
			if (exists)
			{
				varsInScopeN[currScopeLevel].insert(name);
				allVarsInScope.insert(name);
			}
		}
		else
		{
			varsInScopeN[currScopeLevel].insert(name);
			allVarsInScope.insert(name);
		}
		return true;
	}
	return false;
}

bool parseAssignment();
bool parseLvalue();
bool parseVar();
bool parseExpr(ParsedType& t = discard);
bool parseTerm(ParsedType& t);
void parseExprBinary(const ParsedType& left);
bool parseVal(ParsedType& t);
// commaSep rule accepts epsilon, so would always return true anyways;
// if there is a syntax error, an exception will be thrown
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
const std::string& nextToken(bool skip = true, bool skipWhitespace = true)
{
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
	return tokens[skip ? tokenNum++ : tokenNum];
}

bool parse(std::vector<std::string> vec)
{
	tokens = vec;
	tokenNum = 0;
	try
	{
		skipToNextLine();
		parseNestedBlock();
		// check that all function names are valid
	}
	catch (ParseException& e)
	{
		std::cout << e.what();
		return false;
	}
}

void parseNextLine()
{
	if (parseAssignment() || structure() || funcDef())
	{
		const std::string& token = nextToken(false, false);
		if (token != "\n" && token != END)
		{
			throw ParseException("each statement/program element must be on a new line");
		}

		size_t formerScope = currScopeLevel;
		skipToNextLine();

		if (currScopeLevel > formerScope)
		{
			throw ParseException("illegal attempt to increase indentation level");
		}
	}
	else
	{
		throw ParseException("invalid line");
	}
}

void parseNestedBlock()
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
	parseNextLine();

	while (currScopeLevel == blockScope)
	{
		parseNextLine();
	}

	allVarsInScope.erase(varsInScopeN[currScopeLevel].begin(), varsInScopeN[currScopeLevel].end());
	varsInScopeN.pop_back();
	currScopeLevel--;
}

bool funcDef()
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
					varsInScopeN.push_back(std::set<std::string>());
					currScopeLevel++;
					return true;
				}
				throw ParseException("closing parenthesis expected");
			}
			throw ParseException("expected argument list after function declaration");
		}
		throw ParseException("invalid function name");
	}
}

bool structure()
{
	int init = tokenNum;
	const std::string* token = &nextToken(true);
	if (*token == "if")
	{
		ParsedType type;
		if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
		{
			if (nextToken() == "then")
			{
				size_t ifScope = currScopeLevel;

				bool elseReached = false;
				do
				{
					// get all lines inside of block
					parseNestedBlock();
					if (currScopeLevel != ifScope)
					{
						return true;
					}
					
					if (nextToken() == "else")
					{
						if (nextToken() == "if")
						{
							if (elseReached)
							{
								throw ParseException("'else' block cannot be followed by 'else if' block");
							}
							continue;
						}
						if (elseReached)
						{
							throw ParseException("multiple 'else' blocks not allowed");
						}
						elseReached = true;
					}
					tokenNum--;
					return true;
				} while (true);

				/*for (int i = (int)varsInScopeN.size() - 1; i >= newScope; i--)
				{
					allVarsInScope.erase(varsInScopeN[i].begin(), varsInScopeN[i].end());
					varsInScopeN.pop_back();
				}*/
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
								parseNestedBlock();
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
}

bool parseAssignment()
{
	int init = tokenNum;
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
	int init = tokenNum;
	const std::string& token = nextToken();
	if (acceptVarName(token))
	{
		while (nextToken() == "[")
		{
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
		tokenNum--;

		return true;
	}

	return tokenNum = init, false;
}

bool parseVar()
{
	int init = tokenNum;
	const std::string& token = nextToken();
	if (acceptVarName(token, true))
	{
		if (nextToken() == "(")
		{
			parseCommaSep([] { return parseExpr(); });

			if (nextToken() == ")")
			{
				return true;
			}
			throw ParseException("expected closing parenthesis");
		}
		tokenNum--;

		while (nextToken() == "[")
		{
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

	tokenNum--;

	return parseVal(t);

	// check if this is a valid literal value;
	// if it is not, this is not a valid expression
	//return val(t) ? exprBinary(t) : tokenNum = init, false;
}

bool parseVal(ParsedType& t)
{
	int init = tokenNum;
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
	if (tokenNum--, parseVar())
	{
		return t = ParsedType::any, true;
	}

	// failed all checks, this is not a value
	return tokenNum = init, false;
}

void parseExprBinary(const ParsedType& leftType)
{
	int init = tokenNum;
	const std::string& token = nextToken();

	if (token == END)
	{
		return;
	}

	ParsedType rightType;
	if (!parseTerm(rightType))
	{
		tokenNum = init;
		return;
	}

	if (token == "+")
	{
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
		if (nextToken() != "not")
		{
			tokenNum--;
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
	int init = tokenNum;
	if (checkFunc())
	{
		// loop to handle further comma-separated expressions
		while (true)
		{
			if (nextToken() == ",")
			{
				if (checkFunc())
				{
					continue;
				}
				throw ParseException("expected item after comma");
			}
			tokenNum--;
			break;
		}
	}
}

// returns scope level of next line
void skipToNextLine()
{
	const std::string* token = &nextToken(false);

	bool keepGoing = true;
	if (*token == "\n")
	{
		size_t scope;
		do
		{
			scope = 0;

			token = &nextToken(false);
			while (*token == "\t")
			{
				scope++;
			}
			keepGoing = *token == "\n" || token->substr(0, 2) != "//";
		} while (keepGoing);

		if (*token == END)
		{
			throw ParseException("done");
		}

		currScopeLevel = scope;
	}
	if (*token == END)
	{
		throw ParseException("done");
	}
	//throw ParseException("expected new line");
}

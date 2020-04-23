#pragma once
#include <string>
#include <vector>
#include <regex>

enum class ParsedType { number, boolean, string, list, map, null, any };

struct ParseException : public std::exception
{
	std::string msg;
	ParseException(std::string str) : msg(str) {}
	const char* what() const throw()
	{
		return msg.c_str();
	}
};

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

const std::regex NAME_RE = std::regex("[_a-zA-Z][_a-zA-Z0-9]*");
const std::regex NUMBER_RE = std::regex("[0-9]+(\.[0-9]+)?"); //"\d+(\.\d+)?"
const std::regex STRING_RE = std::regex("\".*\"");

std::string newName;
std::vector<std::string> namesUsed;

std::vector<std::string> tokens;
size_t tokenNum;
ParsedType discard;

bool assignment();
bool lvalue();
bool var();
bool expr(ParsedType& t = discard);
bool term(ParsedType& t);
bool exprBinary(const ParsedType& left);
bool val(ParsedType& t);
// commaSep rule accepts epsilon, so would always return true anyways;
// if there is a syntax error, an exception will be thrown
void commaSep();

bool parse(std::vector<std::string> vec)
{
	tokens = vec;
	tokenNum = 0;
	try
	{
		return assignment();
	}
	catch (ParseException& e)
	{
		std::cout << e.what();
		return false;
	}
}

const std::string END = "";

const std::string& nextToken()
{
	if (tokenNum >= tokens.size())
	{
		return END;
	}
	return tokens[tokenNum++];
}

bool assignment()
{
	int init = tokenNum;
	if (lvalue())
	{
		if (nextToken() == "<-")
		{
			if (expr())
			{
				return true;
			}
			throw ParseException("expected expression");
		}
		throw ParseException("invalid assignment expression");
	}

	return tokenNum = init, false;
}

bool lvalue()
{
	int init = tokenNum;
	const std::string& token = nextToken();
	if (std::regex_match(token, NAME_RE))
	{
		newName = token;
		while (nextToken() == "[")
		{
			if (expr())
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

bool var()
{
	int init = tokenNum;
	const std::string& token = nextToken();
	if (std::regex_match(token, NAME_RE))
	{
		namesUsed.push_back(token);
		if (nextToken() == "(")
		{
			commaSep();
			if (nextToken() == ")")
			{
				return true;
			}
			throw ParseException("expected closing parenthesis");
		}
		tokenNum--;

		while (nextToken() == "[")
		{
			if (expr())
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

bool expr(ParsedType& t)
{
	if (term(t))
	{
		exprBinary(t);
		return true;
	}
	return false;
}

bool term(ParsedType& t)
{
	int init = tokenNum;
	ParsedType type;
	const std::string& token = nextToken();

	// check for unary prefix operators such as -, not
	if (token == "-")
	{
		if (expr(type))
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
		if (expr(type))
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
		if (expr(type))
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

	return val(t);

	// check if this is a valid literal value;
	// if it is not, this is not a valid expression
	//return val(t) ? exprBinary(t) : tokenNum = init, false;
}

bool val(ParsedType& t)
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
		commaSep();
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
		while (expr())
		{
			// each map entry must be in the form <key> <- <value>
			if (nextToken() == "<-")
			{
				if (expr())
				{
					if (nextToken() == ",")
					{
						lastComma = true;
						continue;
					}
					lastComma = false;
					tokenNum--;
					break;
				}
				throw ParseException("expected expression as map value");
			}
			throw ParseException("map entry must be of form <key> <- <value>");
		}

		if (lastComma)
		{
			throw ParseException("expected expression after comma");
		}
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
	if (tokenNum--, var())
	{
		return t = ParsedType::any, true;
	}

	// failed all checks, this is not a value
	return tokenNum = init, false;
}

bool exprBinary(const ParsedType& leftType)
{
	int init = tokenNum;
	const std::string& token = nextToken();

	if (token == END)
	{
		return true;
	}

	ParsedType rightType;
	if (!expr(rightType))
	{
		return tokenNum = init, false;
	}

	if (token == "+")
	{
		switch (leftType)
		{
		case ParsedType::number:
			if (rightType == ParsedType::number || rightType == ParsedType::any)
			{
				return true;
			}
			throw ParseException("'+' operator cannot be applied to number and " + typeToString(rightType));
		case ParsedType::any:
		case ParsedType::string:
		case ParsedType::list:
			return true;
		default:
			throw ParseException("'+' operator cannot be applied to " + typeToString(leftType));
		}
	}
	else if (token == "-" || token == "*" || token == "/" || token == "mod")
	{
		if ((leftType == ParsedType::number || leftType == ParsedType::any) &&
			(rightType == ParsedType::number || rightType == ParsedType::any))
		{
			return true;
		}
		throw ParseException("'" + token + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}
	else if (token == "and" || token == "or")
	{
		if ((leftType == ParsedType::boolean || leftType == ParsedType::any) &&
			(rightType == ParsedType::boolean || rightType == ParsedType::any))
		{
			return true;
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
			return true;
		}
		throw ParseException("'" + token + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}

	// epsilon also accepted
	return true;
}

void commaSep()
{
	int init = tokenNum;
	if (expr())
	{
		// loop to handle further comma-separated expressions
		while (true)
		{
			if (nextToken() == ",")
			{
				if (expr())
				{
					continue;
				}
				throw ParseException("expected expression after comma");
			}
			tokenNum--;
			break;
		}
	}
}

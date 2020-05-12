#pragma once
#include <string>
#include <vector>
#include <regex>
#include <set>

class SyntaxException : public std::exception
{
	std::string msg;
public:
	SyntaxException(std::string str) : msg(str) {}
	const char* what() const throw()
	{
		return msg.c_str();
	}
};

namespace parser
{
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
		default:
			return "null";
		}
	}

	const std::regex NAME_RE = std::regex("[_a-zA-Z][_a-zA-Z0-9]*");
	const std::regex NUMBER_RE = std::regex("[0-9]+(\.[0-9]+)?");
	const std::regex STRING_RE = std::regex("\".*\"");

	const std::set<std::string> keywords = {
		"if", "then", "else", "do", "not", "true", "false", "null", "repeat", "while",
		"until", "for", "return", "break", "continue", "mod", "function", "and", "or" // TODO
	};

	std::vector<std::string> tokens;
	size_t tokenNum;

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
	bool parseStructure(void (*&)(size_t, std::set<bool (*)()>&), bool (*&)());
	bool parseAssignment();
	bool parseVar();
	ParsedType discard;
	bool parseExpr(ParsedType& = discard);
	bool parseTerm(ParsedType&);
	void parseExprBinary(const ParsedType&);
	bool parseVal(ParsedType&);
	int parseCommaSep(bool (*)());
	void skipToNextLine();

	const std::string END = "";
	// TODO maybe change working with tokens to pointers
	const std::string& nextToken(bool advance, bool skipWhitespace = true)
	{
		size_t init = tokenNum;

		int endScope = currScopeLevel;
		if (skipWhitespace)
		{
			while (tokenNum < tokens.size())
			{
				if (tokens[tokenNum] == "\n")
				{
					endScope = 0;
				}
				else if (tokens[tokenNum] == "\t")
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
			//throw SyntaxException("first line of multiline statement must be indented at or below level of successive lines");
		}

		const std::string& ret = tokens[tokenNum];
		tokenNum = advance ? tokenNum + 1 : init;
		return ret;
	}

	// returns scope level of next line
	void skipToNextLine()
	{
		const std::string* token = &nextToken(false, false);

		int scope = 0;
		if (*token == "\n")
		{
			tokenNum++;
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
		}
		currScopeLevel = scope;
	}

	//  +-----------------------+
	//  |   Parsing functions   |
	//  +-----------------------+

	bool parse(std::vector<std::string> vec, std::string& transpiled)
	{
		tokens = vec;
		inFunction = false;
		try
		{
			skipToNextLine();
			if (currScopeLevel != 0)
			{
				throw SyntaxException("first line of file must not be indented");
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
					throw SyntaxException("attempted use of undeclared function " + e.name);
				}
				if (matching->numParams != e.numParams)
				{
					throw SyntaxException("expected " + std::to_string(matching->numParams) +
						" parameters for function '" + matching->name + "'");
				}
			}

			// finalize transpiled content
			transpiled = "#include \"variable.h\"\n\n";
			if (newFunctions.size() != 0)
			{
				for (const SudohFunction& e : newFunctions)
				{
					transpiled += "Variable " + e.name + "(";
					for (int i = 0; i < e.numParams; i++)
					{
						transpiled += std::string("Variable&") + (i < e.numParams - 1 ? ", " : "");
					}
					transpiled += ")\n";
				}

				transpiled += "\n" + transpiledFunctions + "\n";
			}
			transpiled += transpiledMain;

		}
		catch (SyntaxException& e)
		{
			std::cout << e.what() << std::endl;
			return false;
		}

		return true;
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

	bool parseNextLine(std::set<bool (*)()>& extraRules)
	{
		if (nextToken(false, false) == END)
		{
			return false;
		}

		if (parseAssignment())
		{
			uncommittedTrans += ";";
			return true;
		}

		void (*parseAfter)(size_t, std::set<bool (*)()>&) = nullptr;
		bool (*extraRule)() = nullptr;
		if (parseStructure(parseAfter, extraRule))
		{
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
				return true;
			}
		}

		throw SyntaxException("invalid line");
	}

	void parseBlock(std::set<bool (*)()>& extraRules)
	{
		int initScope = currScopeLevel;

		uncommittedTrans = "{";
		commitLine();

		skipToNextLine();
		if (currScopeLevel <= initScope)
		{
			throw SyntaxException("empty block not allowed");
		}

		varsInScopeN.push_back(std::set<std::string>());

		while (currScopeLevel == initScope + 1 && parseNextLine(extraRules))
		{
			const std::string& token = nextToken(false, false);
			if (token != "\n" && token != END)
			{
				throw SyntaxException("each statement/program element must be on a new line");
			}
			skipToNextLine();
		}

		if (inFunction && initScope == 0)
		{
			uncommittedTrans = "return null;";
			commitLine();
		}

		if (currScopeLevel > initScope)
		{
			throw SyntaxException("illegal attempt to increase indentation level");
		}

		int temp = currScopeLevel;
		currScopeLevel = initScope;
		uncommittedTrans = "}";
		commitLine();
		currScopeLevel = temp;

		varsInScopeN.pop_back();
	}

	void parseAfterIf(size_t scope, std::set<bool (*)()>& extraRules)
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
						throw SyntaxException("'else' block cannot be followed by 'else if' block");
					}

					uncommittedTrans += " if (";
					ParsedType type;
					if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
					{
						if (nextToken(true) == "then")
						{
							uncommittedTrans += ")";
							commitLine();

							parseBlock(extraRules);
							continue;
						}
						throw SyntaxException("expected 'then'");
					}
					throw SyntaxException("expected condition");
				}
				if (elseReached)
				{
					throw SyntaxException("multiple 'else' blocks not allowed");
				}
				elseReached = true;

				commitLine();
				parseBlock(extraRules);
				continue;
			}
			return;
		}
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
				uncommittedTrans += "Variable& " + name;
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
						throw SyntaxException("use of undeclared variable " + name);
					}
					varsInScopeN[currScopeLevel].insert(name);
					uncommittedTrans += "Variable ";
				}
				uncommittedTrans += name;

				return exists ? VarStatus::existingVar : VarStatus::newVar;
			}
		}
		return VarStatus::invalid;
	}

	bool parseMapEntry()
	{
		if (!parseExpr())
		{
			throw SyntaxException("expected expression as map key");
		}
		if (nextToken(true) != "<-")
		{
			throw SyntaxException("map entry must be of form <key> <- <value>");
		}
		if (!parseExpr())
		{
			throw SyntaxException("expected expression as map value");
		}
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

	bool parseStructure(void (*&parseAfter)(size_t, std::set<bool (*)()>&), bool (*&additionalRule)())
	{
		size_t init = tokenNum;
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
				throw SyntaxException("expected 'then'");
			}
			throw SyntaxException("expected condition");
		}

		if (*token == "while" || *token == "until")
		{
			ParsedType type;
			if (parseExpr(type) && (type == ParsedType::boolean || type == ParsedType::any))
			{
				if (nextToken(true) == "do")
				{
					additionalRule = extraParseInsideLoop;
					return true;
				}
				throw SyntaxException("expected 'do'");
			}
			throw SyntaxException("expected condition");
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
						uncommittedTrans += "; " + forVar;
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
									uncommittedTrans += "; " + forVar + "++)";
									additionalRule = extraParseInsideLoop;
									return true;
								}
								throw SyntaxException("expected 'do'");
							}
							throw SyntaxException("final value of 'for' loop should be numeric expression");
						}
						throw SyntaxException("expected 'to' or 'down to'");
					}
					throw SyntaxException("value of 'for' loop variable should be numeric expression");
				}
				throw SyntaxException("expected initial value assignment to variable in 'for'");
			}
			throw SyntaxException("expected variable name after 'for'");
		}

		if (*token == "repeat")
		{

		}

		if (*token == "function")
		{
			if (inFunction)
			{
				throw SyntaxException("nested function illegal");
			}

			const std::string& funcName = nextToken(true);
			if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
			{
				uncommittedTrans += "Variable " + funcName + "(";
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
					throw SyntaxException("closing parenthesis expected");
				}
				throw SyntaxException("expected argument list after function declaration");
			}
			throw SyntaxException("invalid function name");
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
				if (status == VarStatus::newVar)
				{
					throw SyntaxException("cannot access map value of undefined variable");
				}

				tokenNum++;
				if (parseExpr())
				{
					if (nextToken(true) == "]")
					{
						continue;
					}
					throw SyntaxException("expected ']'");
				}
				throw SyntaxException("expected expression inside bracket");
			}

			if (nextToken(true) == "<-")
			{
				uncommittedTrans += " = ";
				if (parseExpr())
				{
					return true;
				}
				throw SyntaxException("expected expression");
			}
			throw SyntaxException("invalid assignment expression");
		}

		return false;
	}

	bool parseFuncCall()
	{
		const std::string& funcName = nextToken(false);
		if (std::regex_match(funcName, NAME_RE) && keywords.count(funcName) == 0)
		{
			if (nextToken(false) == "(")
			{
				tokenNum++;
				uncommittedTrans += "(";
				int numParams = parseCommaSep([] { return parseExpr(); });

				if (nextToken(true) == ")")
				{
					functionsUsed.insert({ funcName, numParams });
					uncommittedTrans += ")";
					return true;
				}
				throw SyntaxException("expected closing parenthesis for function call");
			}
		}
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
					throw SyntaxException("expected closing bracket");
				}
				throw SyntaxException("expected expression inside bracket");
			}

			return true;
		}
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
		const std::string& token = nextToken(true);

		// check for unary prefix operators such as -, not
		if (token == "-")
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
		}
		if (token == "not")
		{
			uncommittedTrans += "!";
			if (parseExpr(type))
			{
				if (type == ParsedType::boolean || type == ParsedType::any)
				{
					return t = type, true;
				}
				throw SyntaxException("cannot apply 'not' operator to non-boolean item");
			}
			throw SyntaxException("expected expression after 'not'");
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
		size_t init = tokenNum;
		std::string initStr = uncommittedTrans;

		const std::string& token = nextToken(true);

		// check for boolean
		if (token == "true" || token == "false")
		{
			uncommittedTrans += token;
			return t = ParsedType::boolean, true;
		}

		// check for number
		if (std::regex_match(token, NUMBER_RE))
		{
			uncommittedTrans += token;
			return t = ParsedType::number, true;
		}

		// check for string
		if (std::regex_match(token, STRING_RE))
		{
			uncommittedTrans += token;
			return t = ParsedType::string, true;
		}

		// check for list
		if (token == "[")
		{
			uncommittedTrans += token;
			parseCommaSep([] { return parseExpr(); });
			if (nextToken(true) == "]")
			{
				return t = ParsedType::list, true;
			}
			throw SyntaxException("expected end of list");
		}

		// check for map
		if (token == "{")
		{
			uncommittedTrans += token;
			// flag to keep track of whether another entry is needed (after comma)
			bool lastComma = false;

			parseCommaSep(parseMapEntry);

			if (nextToken(true) == "}")
			{
				uncommittedTrans += "}";
				return t = ParsedType::map, true;
			}
			throw SyntaxException("expected closing curly brace");
		}

		// check for null value
		if (token == "null")
		{
			uncommittedTrans += token;
			return t = ParsedType::null, true;
		}

		// check if this is valid variable-type expression indicating 'any' type
		if (tokenNum = init, parseVar())
		{
			return t = ParsedType::any, true;
		}

		// failed all checks, this is not a value
		uncommittedTrans = initStr;
		return tokenNum = init, false;
	}

	void checkValidBinary(const std::string binaryOp, const std::string transBinOp,
		const ParsedType& leftType, const std::map<ParsedType, std::set<ParsedType>>& allowedOps)
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

		throw SyntaxException("'" + binaryOp + "' cannot be applied to types " +
			typeToString(leftType) + " and " + typeToString(rightType));
	}

	void parseExprBinary(const ParsedType& leftType)
	{
		size_t init = tokenNum;

		typedef std::map<ParsedType, std::set<ParsedType>> OpMap;

		static const std::set<ParsedType> all = {
			ParsedType::number, ParsedType::boolean, ParsedType::string,
			ParsedType::list, ParsedType::map, ParsedType::null
		};

		static const OpMap plus = {
			{ ParsedType::number, { ParsedType::number } },
			{ ParsedType::string, all },
			{ ParsedType::list, all }
		};
		static const OpMap arithmetic = { { ParsedType::number, { ParsedType::number } } };
		static const OpMap boolOps = { { ParsedType::boolean, { ParsedType::boolean } } };
		static const OpMap equalOps = {
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
			checkValidBinary(token, token, leftType, plus);
		}
		else if (token == "-" || token == "*" || token == "/" || token == "mod")
		{
			checkValidBinary(token, token == "mod" ? "%" : token, leftType, arithmetic);
		}
		else if (token == "and" || token == "or")
		{
			checkValidBinary(token, token == "and" ? "&&" : "||", leftType, boolOps);
		}
		else if (token == "=" || token == "!=")
		{
			checkValidBinary(token, token == "=" ? "==" : "!=", leftType, equalOps);
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
				throw SyntaxException("expected item after comma");
			}
		}
		
		return numItems;
	}
}

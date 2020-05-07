#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <regex>
#include "parser.h"
//#include <boost/spirit/include/qi.hpp>
//#include <boost/phoenix/stl/container/container.hpp>

#define NAME_RE "[a-zA-Z_]+[a-zA-Z0-9_]*"
#define TYPE_RE "(number|boolean|string|list|map)"
#define INT_RE "([0-9]+)"
#define FLOAT_RE "([0-9]*.[0-9]+)"
#define STRING_RE "(\".*\")"
#define VAL_RE ""
#define LIST_RE "([" VAL_RE "(," VAL_RE ")*])"
#define BOOL_RE "((not )? (" VAL_RE " (<=|<|is|>|>=) " VAL_RE ")|)"

#define FUNCCALL_RE NAME_RE " \\( (" NAME_RE " (, " NAME_RE ")*)? \\)"
#define IF_RE "if " BOOL_RE
#define FUNCDEF_RE "function " NAME_RE " \\( " NAME_RE "( : " TYPE_RE ")?" \
					  "( , " NAME_RE "( : " TYPE_RE ")?)* \\) (returns " TYPE_RE ")?"

//using namespace boost::spirit;
//using namespace boost::phoenix;

std::string transpiled;

std::vector<std::string> tokenize(std::string line)
{
	// symbols which act as both delimiters and tokens themselves
	static const std::vector<std::string> delimTokens = {
		" ", ",", "\"", "\n", "\t",
		"(", ")", "[", "]", "{", "}",
		"<-", "+", "-", "*", "//", "/",
		"<=", "<", ">=", ">", "=", "!="
	};

	std::vector<std::string> tokens;

	// treat line as if there were a delimiter at the end
	// (for algorithm to work correctly)
	line += " ";
	int idxBeginToken = 0;
	bool beginLine = true;
	for (size_t curr = 0; curr < line.length(); curr++)
	{
		// check if the token at index is a delimiter
		const std::string* delim = nullptr;
		for (const std::string& d : delimTokens)
		{
			if (line.compare(curr, d.length(), d) == 0)
			{
				delim = &d;
				break;
			}
		}

		// if token at index was not delimiter then skip
		if (!delim)
		{
			continue;
		}

		// add new token
		if (curr > idxBeginToken)
		{
			tokens.push_back(line.substr(idxBeginToken, curr - idxBeginToken));
		}

		

		// add delimiter as a token as well (if it is not space)
		if (*delim != " ")
		{
			// if delimeter is a string then close the string if it is valid
			if (*delim == "\"")
			{
				// try to find closing quote
				size_t close;
				for (close = curr + 1; close < line.length(); close++)
				{
					if (line[close] == '\"' && line[close - 1] != '\\')
					{
						break;
					}
				}
				// if loop iterated through rest of string but still
				// didnt find closing quote then string is mal-formed
				if (close == line.length())
				{
					throw ParseException("malformed string");
				}

				tokens.push_back(line.substr(curr, close - curr + 1));
				curr = close;
			}
			else if (*delim == "//")
			{
				size_t end = line.find('\n', curr);
				tokens.push_back(line.substr(curr, end));
				curr = end;
			}
			else
			{
				beginLine = *delim == "\n" ? true : (*delim == "\t" && beginLine);

				// do not add tabs after the beginning of a line to the list
				bool tabAfterBegin = *delim == "\t" && !beginLine;
				if (!tabAfterBegin)
				{
					tokens.push_back(*delim);
				}

				curr += delim->length() - 1;
			}
		}

		idxBeginToken = curr + 1;
	}

	return tokens;
}

void analyze(std::ifstream& file)
{
	std::stringstream contents;
	contents << file.rdbuf();

	std::vector<std::string> tokens = tokenize(contents.str());

	bool r = parse(tokens);
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "usage: sudohc <file.sud>\n";
		return 1;
	}
	std::string name = argv[1];
	size_t index = name.find(".sud");
	if (index == std::string::npos || index != name.length() - 4)
	{
		std::cout << "invalid file type\n";
		return 1;
	}

	std::ifstream file;
	file.open(argv[1]);
	if (!file)
	{
		std::cout << "invalid file\n";
		return 1;
	}

	analyze(file);

	transpiled += "}\n";
	file.close();
	return 0;
}

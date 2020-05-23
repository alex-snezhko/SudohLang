#include "sudohc.h"
#include <cctype>

void tokenize(const std::string& line, std::vector<Token>& tokens)
{
	// symbols which act as both delimiters and tokens themselves
	static const std::vector<std::string> delimTokens = {
		" ", ",", "\"", "\n", "\t",
		"(", ")", "[", "]", "{", "}",
		"<-", "+", "-", "*", "//", "/",
		"<=", "<", ">=", ">", "=", "!="
	};

	bool beginLine = true;
	int lineNum = 1;

	size_t idxBeginToken = 0;
	for (size_t i = 0; i < line.length(); i++)
	{
		// check if the token at index is a delimiter
		const std::string* delim = nullptr;
		for (const std::string& d : delimTokens)
		{
			if (line.compare(i, d.length(), d) == 0)
			{
				delim = &d;
				break;
			}
		}

		// if token at index was not delimiter then skip
		if (!delim)
		{
			if (!isalnum(line[i]) && line[i] != '_')
			{
				tokens.push_back({ lineNum, i, std::string(1, line[i]) });
				throw SyntaxException("invalid character", tokens.size() - 1);
			}
			beginLine = false;
			continue;
		}

		// add new token
		if (i > idxBeginToken)
		{
			tokens.push_back({ lineNum, idxBeginToken, line.substr(idxBeginToken, i - idxBeginToken) });
		}

		// add delimiter as a token as well (if it is not space)
		if (*delim != " ")
		{
			// if delimeter is a string then close the string if it is valid
			if (*delim == "\"")
			{
				// try to find closing quote
				size_t close;
				for (close = i + 1; close < line.length(); close++)
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
					tokens.push_back({ lineNum, i, std::string(1, line[i]) });
					throw SyntaxException("malformed string", tokens.size() - 1);
				}

				tokens.push_back({ lineNum, i, line.substr(i, close - i + 1) });
				i = close;
			}
			else if (*delim == "//")
			{
				tokens.push_back({ lineNum, i, "\n" });
				i = line.find('\n', i);
			}
			else if (*delim == "\t")
			{
				// do not add tabs after the beginning of a line to the list
				if (beginLine)
				{
					tokens.push_back({ lineNum, i, "\t" });
				}
			}
			else if (*delim == "\n")
			{
				tokens.push_back({ lineNum++, i, "\n" });
				beginLine = true;
			}
			else
			{
				beginLine = false;
				tokens.push_back({ lineNum, i, *delim });
				i += delim->length() - 1;
			}
		}

		idxBeginToken = i + 1;
	}

	tokens.push_back({ lineNum, line.length() - 1, END });
}
#ifndef TOKEN_ITER_H
#define TOKEN_ITER_H

#include <vector>
#include <string>

// container class for list of tokens in input Sudoh source file
class TokenIterator
{
	// simple struct for containing information about each token in a source file
	struct Token
	{
		// the line in the source file which this token appears on
		int lineNum;
		// the character of the source file which this token appears on
		size_t fileCharNum;
		// the actual token
		const std::string tokenString;

		bool operator==(const Token& other) const
		{
			return tokenString == other.tokenString;
		}
	};

	// list of tokens to be accessed by parsing functions
	std::vector<Token> tokens;
	// current index into tokens list
	size_t tokenNum;

public:
	static const std::string END;

	TokenIterator() : tokenNum(0) {}
	void tokenize(const std::string& contents);

	// returns string of the token that the parser is currently on
	const std::string& currToken();
	void advance();

	const std::vector<Token>& getTokens();
	size_t getTokenNum();
	void setTokenNum(size_t val);
};

#endif

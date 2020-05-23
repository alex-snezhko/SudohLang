#pragma once
#include <string>
#include <vector>

struct Token
{
	int lineNum;
	size_t fileCharNum;
	const std::string tokenString;

	bool operator==(const Token& other) const
	{
		return tokenString == other.tokenString;
	}
};

class SyntaxException : public std::exception
{
	std::string msg;
	size_t tokenNum;
public:
	SyntaxException(std::string str, size_t token) : msg(str), tokenNum(token) {}
	const char* what() const
	{
		return msg.c_str();
	}
	size_t getTokenNum() const
	{
		return tokenNum;
	}
};

void tokenize(const std::string& line, std::vector<Token>& tokens);
const std::string END = "";
bool parse(const std::string& contents, std::string& transpiled);

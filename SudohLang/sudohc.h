#pragma once
#include <string>
#include <vector>

struct Token
{
	int lineNum;
	int fileCharNum;
	std::string token;
};

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

std::vector<Token> tokenize(const std::string& line);
int parse(const std::string& contents, std::string& transpiled);

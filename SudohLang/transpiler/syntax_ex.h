#ifndef SYNTAX_EX_H
#define SYNTAX_EX_H

#include <exception>
#include <string>

class SyntaxException : public std::exception
{
	std::string msg;
	bool thisToken;
	size_t tokenNum;
public:
	// will indicate a SyntaxException at the current token; this is mostly for convenience as
	// most SyntaxExceptions thrown will be at the current token
	SyntaxException(std::string str) : msg(str), thisToken(true), tokenNum(0) {}
	// will indicate a SyntaxException at a different specified token
	SyntaxException(std::string str, size_t token) : msg(str), thisToken(false), tokenNum(token) {}

	bool isThisToken() const
	{
		return thisToken;
	}

	size_t getTokenNum() const
	{
		return tokenNum;
	}

	const char* what() const throw()
	{
		return msg.c_str();
	}
};

#endif

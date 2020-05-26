#pragma once
#include <exception>
#include <string>

class SyntaxException : public std::exception
{
	std::string msg;
public:
	SyntaxException(std::string str) : msg(str) {}
	const char* what() const
	{
		return msg.c_str();
	}
};
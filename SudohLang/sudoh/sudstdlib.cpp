#include "sudoh.h"
#include <iostream>

Variable _print(Variable& str)
{
	std::cout << str.toString();
	return null;
}

Variable _printLine(Variable& str)
{
	_print(str);
	std::cout << std::endl;
	return null;
}

Variable _length(Variable& var)
{
	return var.length();
}

Variable _string(Variable& var)
{
	return var.toString();
}

Variable _number(Variable& var)
{
	//switch (var.type)
	//{
		//TODO
	//}
	return null;
}

Variable _random()
{
	return rand(); // TODO add srand to beginning of transpiled output
}
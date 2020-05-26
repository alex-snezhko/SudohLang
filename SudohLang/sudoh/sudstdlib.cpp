#include "sudoh.h"
#include <iostream>
#include <ctime>

Variable f_print(Variable str)
{
	std::cout << str.toString();
	return null;
}

Variable f_printLine(Variable str)
{
	std::cout << str.toString() << std::endl;
	return null;
}

Variable f_length(Variable var)
{
	switch (var.type)
	{
	case Type::list:
		return (int)var.val.listRef->val.size();
	case Type::map:
		return (int)var.val.mapRef->val.size();
	case Type::string:
		return (int)var.val.stringVal.length();
	}
	runtimeException("cannot take length of type " + var.typeString());
}

Variable f_string(Variable var)
{
	return var.toString();
}

Variable f_integer(Variable var)
{
	switch (var.type)
	{
	case Type::number:
		break;
	}
	return null;
}

Variable f_random()
{
	static bool seedSet = false;
	if (!seedSet)
	{
		srand(time(nullptr));
		seedSet = true;
	}
	return rand();
}

Variable f_remove(Variable list, Variable index)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'remove' must be a list");
	}

	if (index.type != Type::number || index.val.numVal.isInt)
	{
		runtimeException("second parameter of 'remove' must be an integer number");
	}

	int i = index.val.numVal.intVal;
	List& l = list.val.listRef->val;
	l.erase(l.begin() + i);
	return null;
}

Variable f_append(Variable list, Variable value)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'append' must be a list");
	}

	list.val.listRef->val.push_back(value);
	return null;
}

Variable f_insert(Variable list, Variable index, Variable value)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'insert' must be a list");
	}

	if (index.type != Type::number || index.val.numVal.isInt)
	{
		runtimeException("second parameter of 'index' must be an integer number");
	}

	int i = index.val.numVal.intVal;
	List& l = list.val.listRef->val;
	l.insert(l.begin() + i, value);
	return null;
}

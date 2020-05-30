#include "sudohstdlib.h"
#include "runtime_ex.h"
#include <iostream>
#include <ctime>
#include <cmath>

inline bool isInt(double d) // TODO old; replace
{
	return abs(round(d) - d) < 0.00000001;
}

double floatVal(std::string which, std::string procedure, const Variable& var)
{
	if (var.type != Type::number)
	{
		runtimeException(which + " parameter of procedure '" + procedure + "' must be of type 'number'");
	}
	return var.val.numVal;
}

template <typename T>
T assertType(std::string which, std::string procedure, bool (*condition)(T&))
{
	T val;
	if (!condition(val))
	{
		runtimeException(which + " parameter of procedure ");
	}
	return val;
} // TODO

// returns user input as a string
Variable f_input()
{
	std::string inp;
	std::cin >> inp;
	return inp;
}

// prints a variable to standard output
Variable f_print(Variable str)
{
	std::cout << str.toString();
	return Variable();
}

// prints a variable on a new line to standard output
Variable f_printLine(Variable str)
{
	std::cout << str.toString() << std::endl;
	return Variable();
}

// return the length of a container
Variable f_length(Variable var)
{
	switch (var.type)
	{
	case Type::list:
		return (double)var.val.listRef->size();
	case Type::map:
		return (double)var.val.mapRef->size();
	case Type::string:
		return (double)var.val.stringVal.length();
	}
	runtimeException("cannot take length of type " + var.typeString());
}

// returns a string representation of a variable
Variable f_string(Variable var)
{
	return var.toString();
}

// converts a floating point number to an integer
Variable f_integer(Variable var)
{
	if (var.type != Type::number)
	{
		runtimeException("parameter of 'integer' must be a number");
	}
	if (isInt(var.val.numVal))
	{
		return var;
	}
	return floor(var.val.numVal);
}

Variable f_number(Variable str)
{
	if (str.type != Type::string)
	{
		runtimeException("parameter of 'number' must be a string");
	}

	const std::string& s = str.val.stringVal;

	try
	{
		return std::stod(s);
	}
	catch (std::invalid_argument&)
	{
		return Variable();
	}
}

// returns the ascii character represented by num
Variable f_ascii(Variable num)
{
	if (num.type != Type::number || !isInt(num.val.numVal))
	{
		runtimeException("parameter of 'ascii' must be an integer");
	}

	double d = num.val.numVal;
	if (d < 0.0 || d >= CHAR_MAX)
	{
		runtimeException("parameter of 'ascii' outside of range of ascii character codes");
	}
	return std::string(1, (char)d);
}

// returns a random integer
Variable f_random(Variable range)
{
	if (range.type != Type::number || !isInt(range.val.numVal))
	{
		runtimeException("parameter of 'random' must be an integer");
	}

	double d = range.val.numVal;
	if (d < 1.0 || d >= SIZE_MAX)
	{
		runtimeException("parameter of 'random' must be an integer in the range [1, " + std::to_string(SIZE_MAX) + ")");
	}

	static bool seedSet = false;
	if (!seedSet)
	{
		srand(time(nullptr));
		seedSet = true;
	}
	return (double)(rand() % (size_t)d);
}

// removes an element from a list or map
Variable f_remove(Variable var, Variable index)
{
	if (var.type == Type::list)
	{
		Variable::List& l = *var.val.listRef;
		if (index.type != Type::number || !isInt(index.val.numVal))
		{
			runtimeException("second parameter of 'remove' on container of type 'list' must be an integer number");
		}

		double d = index.val.numVal;
		if (d < 0.0 || d >= l.size())
		{
			runtimeException("second parameter of 'remove' is out of bounds of list");
		}

		l.erase(l.begin() + (size_t)d);
		return Variable();
	}
	if (var.type == Type::map)
	{
		Variable::Map& m = *var.val.mapRef;
		auto e = m.find(index);
		if (e == m.end())
		{
			runtimeException("element attempted to be removed from map does not exist in the map");
		}
		m.erase(e);
		return Variable();
	}

	runtimeException("illegal call to 'remove' on type " + var.typeString() +
		"'. An element may only be removed from a 'list' or 'map'");
}

// removes the last element from a list
Variable f_removeLast(Variable list)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'remove' must be a list");
	}

	list.val.listRef->pop_back();
	return Variable();
}

// appends a new element to a list
Variable f_append(Variable list, Variable value)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'append' must be a list");
	}

	list.val.listRef->push_back(value);
	return Variable();
}

// inserts an element into a list at specified index
Variable f_insert(Variable list, Variable index, Variable value)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'insert' must be a list");
	}

	if (index.type != Type::number || isInt(index.val.numVal))
	{
		runtimeException("second parameter of 'insert' must be an integer number");
	}

	Variable::List& l = *list.val.listRef;
	double d = index.val.numVal;

	if (d < 0.0 || d >= l.size())
	{
		runtimeException("second parameter of 'insert' is out of bounds of list");
	}

	l.insert(l.begin() + (size_t)d, value);
	return Variable();
}

Variable f_substring(Variable str, Variable begin, Variable end)
{
	if (str.type != Type::string)
	{
		runtimeException("first parameter of 'substring' must be a string");
	}

	if (begin.type != Type::number || !isInt(begin.val.numVal))
	{
		runtimeException("second parameter of 'substring' must be an integer");
	}
	const std::string& s = str.val.stringVal;
	double d1 = begin.val.numVal;
	if (d1 < 0.0 || d1 >= s.length())
	{
		runtimeException("second parameter of 'substring' outside of string bounds");
	}
	size_t b = (size_t)d1;

	if (end.type != Type::number || !isInt(end.val.numVal))
	{
		runtimeException("third parameter of 'substring' must be an integer");
	}
	double d2 = end.val.numVal;
	if (d2 < 0.0 || d2 >= s.length())
	{
		runtimeException("third parameter of 'substring' outside of string bounds");
	}
	size_t e = (size_t)d2;

	return s.substr(b, e - b);
}

Variable f_type(Variable var)
{
	return var.typeString();
}

Variable f_pow(Variable num, Variable power)
{
	return pow(floatVal("first", "pow", num), floatVal("second", "pow", power));
}

Variable f_cos(Variable num)
{
	return cos(floatVal("first", "cos", num));
}

Variable f_sin(Variable num)
{
	return sin(floatVal("first", "sin", num));
}

Variable f_tan(Variable num)
{
	return tan(floatVal("first", "tan", num));
}

Variable f_acos(Variable num)
{
	return acos(floatVal("first", "acos", num));
}

Variable f_asin(Variable num)
{
	return asin(floatVal("first", "asin", num));
}

Variable f_atan(Variable num)
{
	return atan(floatVal("first", "atan", num));
}

Variable f_atan2(Variable num1, Variable num2)
{
	return atan2(floatVal("first", "atan2", num1), floatVal("second", "atan2", num2));
}

Variable f_log(Variable num, Variable base)
{
	return log(floatVal("first", "log", num)) / log(floatVal("second", "log", base));
}

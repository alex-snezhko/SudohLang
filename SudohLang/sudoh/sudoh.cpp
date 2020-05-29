#include "sudoh.h"
#include "runtime_ex.h"
#include <iostream>
#include <ctime>
#include <cmath>

double floatVal(std::string which, std::string procedure, const Variable& var)
{
	if (var.type != Type::number)
	{
		runtimeException(which + " parameter of procedure '" + procedure + "' must be of type 'number'");
	}
	return var.val.numVal.val;
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
	return null;
}

// prints a variable on a new line to standard output
Variable f_printLine(Variable str)
{
	std::cout << str.toString() << std::endl;
	return null;
}

// return the length of a container
Variable f_length(Variable var)
{
	switch (var.type)
	{
	case Type::list:
		return (double)var.val.listRef->val.size();
	case Type::map:
		return (double)var.val.mapRef->val.size();
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
	if (var.val.numVal.isInt)
	{
		return var;
	}
	return floor(var.val.numVal.val);
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
		return null;
	}
}

// returns the ascii character represented by num
Variable f_ascii(Variable num)
{
	if (num.type != Type::number || !num.val.numVal.isInt)
	{
		runtimeException("parameter of 'ascii' must be an integer");
	}

	double d = num.val.numVal.val;
	if (d < 0.0 || d >= CHAR_MAX)
	{
		runtimeException("parameter of 'ascii' outside of range of ascii character codes");
	}
	return std::string(1, (char)num.val.numVal.val);
}

// returns a random integer
Variable f_random(Variable range)
{
	if (range.type != Type::number || !range.val.numVal.isInt)
	{
		runtimeException("parameter of 'random' must be an integer");
	}

	double d = range.val.numVal.val;
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
	return (double)(rand() % (size_t)range.val.numVal.val);
}

// removes an element from a list or map
Variable f_remove(Variable var, Variable index)
{
	if (var.type == Type::list)
	{
		Variable::List& l = var.val.listRef->val;
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("second parameter of 'remove' on container of type 'list' must be an integer number");
		}

		double d = index.val.numVal.val;
		if (d < 0.0 || d >= l.size())
		{
			runtimeException("second parameter of 'remove' is out of bounds of list");
		}

		l.erase(l.begin() + (size_t)index.val.numVal.val);
		return null;
	}
	if (var.type == Type::map)
	{
		Variable::Map& m = var.val.mapRef->val;
		auto e = m.find(index);
		if (e == m.end())
		{
			runtimeException("element attempted to be removed from map does not exist in the map");
		}
		m.erase(e);
		return null;
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

	Variable::List& l = list.val.listRef->val;
	l.pop_back();
	return null;
}

// appends a new element to a list
Variable f_append(Variable list, Variable value)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'append' must be a list");
	}

	list.val.listRef->val.push_back(value);
	return null;
}

// inserts an element into a list at specified index
Variable f_insert(Variable list, Variable index, Variable value)
{
	if (list.type != Type::list)
	{
		runtimeException("first parameter of 'insert' must be a list");
	}

	if (index.type != Type::number || index.val.numVal.isInt)
	{
		runtimeException("second parameter of 'insert' must be an integer number");
	}

	Variable::List& l = list.val.listRef->val;
	double d = index.val.numVal.val;

	if (d < 0.0 || d >= l.size())
	{
		runtimeException("second parameter of 'insert' is out of bounds of list");
	}

	l.insert(l.begin() + (size_t)index.val.numVal.val, value);
	return null;
}

Variable f_substring(Variable str, Variable begin, Variable end)
{
	if (str.type != Type::string)
	{
		runtimeException("first parameter of 'substring' must be a string");
	}

	if (begin.type != Type::number || !begin.val.numVal.isInt)
	{
		runtimeException("second parameter of 'substring' must be an integer");
	}
	const std::string& s = str.val.stringVal;
	double d1 = begin.val.numVal.val;
	if (d1 < 0.0 || d1 >= s.length())
	{
		runtimeException("second parameter of 'substring' outside of string bounds");
	}
	size_t b = (size_t)d1;

	if (end.type != Type::number || !end.val.numVal.isInt)
	{
		runtimeException("third parameter of 'substring' must be an integer");
	}
	double d2 = end.val.numVal.val;
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

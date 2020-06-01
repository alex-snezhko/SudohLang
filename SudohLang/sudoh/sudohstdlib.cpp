#include "sudohstdlib.h"
#include "runtime_ex.h"
#include <iostream>
#include <ctime>
#include <cmath>
#include <memory>

// asserts that a variable is of the right type and returns its value
template <typename T>
T assertTypeGeneric(const std::string& which, const std::string& procedure, const std::string& reqType,
	const Variable& var, bool (*condition)(const Variable&, T&))
{
	T val;
	if (!condition(var, val))
	{
		runtimeException("expected parameter '" + which + "' of procedure '" +
			procedure + "' to be of type '" + reqType + "'");
	}
	return val;
}

// specialized version of 'assertType' which verifies positive integer
size_t assertPositiveInteger(const std::string& which, const std::string& procedure, const Variable& var)
{
	size_t val;
	if (!Variable::indexCheck(var, val))
	{
		runtimeException("expected parameter '" + which + "' of procedure '" + procedure +
			"' to be an integer in the range [0, 2^" + std::to_string(sizeof(size_t) * 8) + " - 1)");
	}
	return val;
}

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
Variable f_integer(Variable num)
{
	double d = assertTypeGeneric("num", "integer", "number", num, Variable::numCheck);
	return floor(d);
}

Variable f_number(Variable str)
{
	const std::string& s = assertTypeGeneric("str", "number", "string", str, Variable::stringCheck);

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
Variable f_ascii(Variable code)
{
	size_t n = assertPositiveInteger("code", "ascii", code);

	if (n >= CHAR_MAX)
	{
		runtimeException("parameter of 'ascii' outside of range of ascii character codes");
	}
	return std::string(1, (char)n);
}

// returns a random integer
Variable f_random(Variable range)
{
	size_t n = assertPositiveInteger("range", "random", range);

	static bool seedSet = false;
	if (!seedSet)
	{
		srand(time(nullptr));
		seedSet = true;
	}
	return (double)(rand() % n);
}

// removes an element from a list or map
Variable f_remove(Variable var, Variable element)
{
	if (var.type == Type::list)
	{
		Variable::List& l = *var.val.listRef;

		size_t n = assertPositiveInteger("element", "remove", element);
		if (n >= l.size())
		{
			runtimeException("parameter 'index' of 'remove' is out of bounds of list");
		}

		l.erase(l.begin() + n);
		return Variable();
	}
	else if (var.type == Type::map)
	{
		Variable::Map& m = *var.val.mapRef;
		auto e = m.find(element);
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
	Variable::List& l = *assertTypeGeneric("list", "removeLast", "list", list, Variable::listCheck);
	l.pop_back();
	return Variable();
}

// appends a new element to a list
Variable f_append(Variable list, Variable value)
{
	Variable::List& l = *assertTypeGeneric("list", "append", "list", list, Variable::listCheck);
	l.push_back(value);
	return Variable();
}

// inserts an element into a list at specified index
Variable f_insert(Variable list, Variable index, Variable value)
{
	Variable::List& l = *assertTypeGeneric("list", "insert", "list", list, Variable::listCheck);
	size_t n = assertPositiveInteger("index", "insert", index);

	if (n >= l.size())
	{
		runtimeException("parameter 'index' of 'insert' is out of bounds of list");
	}

	l.insert(l.begin() + n, value);
	return Variable();
}

Variable f_range(Variable indexable, Variable begin, Variable end)
{
	size_t b = assertPositiveInteger("begin", "substring", begin);
	size_t e = assertPositiveInteger("end", "substring", end);

	if (indexable.type == Type::string)
	{
		const std::string& s = indexable.val.stringVal;

		if (b >= s.length())
		{
			runtimeException("parameter 'begin' of 'range' outside of string bounds");
		}
		if (e >= s.length())
		{
			runtimeException("parameter 'end' of 'range' outside of string bounds");
		}

		return std::string(s.begin() + b, s.end() + e);
	}
	else if (indexable.type == Type::list)
	{
		const Variable::List& l = *indexable.val.listRef;

		if (b >= l.size())
		{
			runtimeException("parameter 'begin' of 'range' outside of list bounds");
		}
		if (e >= l.size())
		{
			runtimeException("parameter 'end' of 'range' outside of list bounds");
		}

		return std::make_shared<Variable::List>(Variable::List(l.begin() + b, l.begin() + e));
	}

	runtimeException("cannot take range of type " + indexable.typeString());
}

Variable f_type(Variable var)
{
	return var.typeString();
}

Variable f_pow(Variable num, Variable power)
{
	double n = assertTypeGeneric("num", "pow", "number", num, Variable::numCheck);
	double p = assertTypeGeneric("power", "pow", "number", num, Variable::numCheck);
	return pow(n, p);
}

Variable f_cos(Variable angle)
{
	double n = assertTypeGeneric("angle", "cos", "number", angle, Variable::numCheck);
	return cos(n);
}

Variable f_sin(Variable angle)
{
	double n = assertTypeGeneric("angle", "sin", "number", angle, Variable::numCheck);
	return sin(n);
}

Variable f_tan(Variable angle)
{
	double n = assertTypeGeneric("angle", "tan", "number", angle, Variable::numCheck);
	return tan(n);
}

Variable f_acos(Variable val)
{
	double n = assertTypeGeneric("angle", "acos", "number", val, Variable::numCheck);
	return acos(n);
}

Variable f_asin(Variable val)
{
	double n = assertTypeGeneric("val", "asin", "number", val, Variable::numCheck);
	return asin(n);
}

Variable f_atan(Variable val)
{
	double n = assertTypeGeneric("val", "atan", "number", val, Variable::numCheck);
	return atan(n);
}

Variable f_atan2(Variable y, Variable x)
{
	double ny = assertTypeGeneric("y", "atan2", "number", y, Variable::numCheck);
	double nx = assertTypeGeneric("x", "atan2", "number", x, Variable::numCheck);
	return atan2(ny, nx);
}

Variable f_log(Variable num, Variable base)
{
	double n = assertTypeGeneric("num", "log", "number", num, Variable::numCheck);
	double b = assertTypeGeneric("base", "log", "number", base, Variable::numCheck);
	return log(n) / log(b);
}

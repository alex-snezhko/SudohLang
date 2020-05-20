#include "sudoh.h"

// +---------------------------+
// |   Number implementation   |
// +---------------------------+

Number::Number(int i) : intVal(i), isInt(true) {}
Number::Number(double f) : floatVal(f), isInt(false) {}

double Number::getFloatVal() const { return isInt ? (double)intVal : floatVal; }

Number Number::operator+(const Number& other) const
{
	return isInt && other.isInt ?
		Number(intVal + other.intVal) :
		Number(getFloatVal() + other.getFloatVal());
}

Number Number::operator-(const Number& other) const
{
	return isInt && other.isInt ?
		Number(intVal - other.intVal) :
		Number(getFloatVal() - other.getFloatVal());
}

Number Number::operator*(const Number& other) const
{
	return isInt && other.isInt ?
		Number(intVal * other.intVal) :
		Number(getFloatVal() * other.getFloatVal());
}

Number Number::operator/(const Number& other) const
{
	if (other.getFloatVal() == 0)
	{
		runtimeException("division by zero");
	}
	return isInt && other.isInt && (intVal % other.intVal == 0) ?
		Number(intVal / other.intVal) :
		Number(getFloatVal() / other.getFloatVal());
}

Number Number::operator%(const Number& other) const
{
	if (other.getFloatVal() == 0)
	{
		runtimeException("division by zero");
	}
	return isInt && other.isInt ?
		Number(intVal % other.intVal) :
		Number(fmod(getFloatVal(), other.getFloatVal()));
}

bool Number::operator==(const Number& other) const
{
	return isInt && other.isInt ?
		intVal == other.intVal :
		getFloatVal() == other.getFloatVal();
}

bool Number::operator!=(const Number& other) const
{
	return !operator==(other);
}

bool Number::operator<(const Number& other) const
{
	return isInt && other.isInt ?
		intVal < other.intVal :
		getFloatVal() < other.getFloatVal();
}

bool Number::operator<=(const Number& other) const
{
	return isInt && other.isInt ?
		intVal <= other.intVal :
		getFloatVal() <= other.getFloatVal();
}

class String
{
	std::string str;
	List chars;

	String(std::string s) : str(s), chars(s.length())
	{
		for (int i = 0; i < s.length(); i++)
		{
			chars[i] = s[i];
		}
	}

	int size() const
	{
		return str.size();
	}

	Variable& operator[](const Variable& index)
	{
		//int i = index.val.numVal.intVal;
		//Variable& t = chars[i];
	}
};
